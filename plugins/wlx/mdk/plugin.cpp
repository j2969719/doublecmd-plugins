/*
 * MDK Wayland WLX plugin for Double Commander
 *
 * Uses QOpenGLWidget for rendering, following the official MDK Qt
 * integration pattern (QMDKWidget). MDK is loaded via dlopen with
 * RTLD_LOCAL to isolate libc++ from DC's libstdc++.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QtWidgets>
#include <QOpenGLWidget>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <dlfcn.h>
#include <cstdio>
#include <cstdint>

#include "sdk/wlxplugin.h"

/* Include official MDK SDK C API headers */
#include <mdk/c/Player.h>
#include <mdk/c/RenderAPI.h>
#include <mdk/c/MediaInfo.h>

/* ── dlopen-based lazy MDK loader ───────────────────────── */

static void *g_mdk_lib = nullptr;
static bool g_mdk_tried = false;

using fn_new    = mdkPlayerAPI* (*)();
using fn_delete = void (*)(mdkPlayerAPI**);
using fn_setopt_str = void (*)(const char*, const char*);
using fn_setopt_ptr = void (*)(const char*, void*);
using fn_gl_destroyed = void (*)();

static fn_new            p_new = nullptr;
static fn_delete         p_delete = nullptr;
static fn_setopt_str     p_setopt_str = nullptr;
static fn_setopt_ptr     p_setopt_ptr = nullptr;
static fn_gl_destroyed   p_gl_destroyed = nullptr;

static bool mdk_load()
{
    if (g_mdk_lib) return true;
    if (g_mdk_tried) return false;
    g_mdk_tried = true;

    g_mdk_lib = dlopen("libmdk.so.0", RTLD_LAZY | RTLD_LOCAL);
    if (!g_mdk_lib)
        g_mdk_lib = dlopen("libmdk.so", RTLD_LAZY | RTLD_LOCAL);
    if (!g_mdk_lib) {
        fprintf(stderr, "[mdk_wlx] dlopen failed: %s\n", dlerror());
        return false;
    }

    p_new           = (fn_new)          dlsym(g_mdk_lib, "mdkPlayerAPI_new");
    p_delete        = (fn_delete)       dlsym(g_mdk_lib, "mdkPlayerAPI_delete");
    p_setopt_str    = (fn_setopt_str)   dlsym(g_mdk_lib, "MDK_setGlobalOptionString");
    p_setopt_ptr    = (fn_setopt_ptr)   dlsym(g_mdk_lib, "MDK_setGlobalOptionPtr");
    p_gl_destroyed  = (fn_gl_destroyed) dlsym(g_mdk_lib, "MDK_foreignGLContextDestroyed");

    if (!p_new || !p_delete) {
        fprintf(stderr, "[mdk_wlx] symbol lookup failed\n");
        dlclose(g_mdk_lib);
        g_mdk_lib = nullptr;
        return false;
    }

    if (p_setopt_str)
        p_setopt_str("logLevel", "Info");

    fprintf(stderr, "[mdk_wlx] MDK loaded OK\n");
    return true;
}

/* ── QOpenGLWidget-based video widget ────────────────────── */

class MdkVideoWidget : public QOpenGLWidget, protected QOpenGLFunctions {
public:
    explicit MdkVideoWidget(QWidget *parent, const QString &file)
        : QOpenGLWidget(parent), m_api(nullptr)
    {
        if (!mdk_load()) return;

        m_api = p_new();
        if (!m_api) {
            fprintf(stderr, "[mdk_wlx] mdkPlayerAPI_new returned null\n");
            return;
        }

        const char *decs[] = {"VAAPI","VDPAU","CUDA","dav1d","FFmpeg", nullptr};
        m_api->setVideoDecoders(m_api->object, decs);

        mdkRenderCallback cb;
        cb.opaque = this;
        cb.cb = [](void*, void* opaque) {
            auto *self = static_cast<MdkVideoWidget*>(opaque);
            QMetaObject::invokeMethod(self, "update", Qt::QueuedConnection);
        };
        m_api->setRenderCallback(m_api->object, cb);

        QByteArray path = file.toUtf8();
        m_api->setMedia(m_api->object, path.constData());
        m_api->setState(m_api->object, MDK_State_Playing);
    }

    ~MdkVideoWidget() override
    {
        if (m_api) {
            makeCurrent();
            m_api->setVideoSurfaceSize(m_api->object, -1, -1, nullptr);
            m_api->setState(m_api->object, MDK_State_Stopped);
            p_delete(&m_api);
            m_api = nullptr;
            doneCurrent();
        }
    }

    mdkPlayerAPI* api() const { return m_api; }

protected:
    void initializeGL() override
    {
        initializeOpenGLFunctions();

        if (!m_api) return;

        memset(&m_glApi, 0, sizeof(m_glApi));
        m_glApi.type = MDK_RenderAPI_OpenGL;
        m_glApi.fbo = -1;
        m_glApi.egl = -1;
        m_glApi.opengl = -1;
        m_glApi.opengles = -1;
        m_glApi.profile = 3;
        m_glApi.opaque = context();

        m_api->setRenderAPI(m_api->object, (mdkRenderAPI*)&m_glApi, nullptr);

        /* Background color for MDK */
        m_api->setBackgroundColor(m_api->object, 0.0f, 0.0f, 0.0f, 1.0f, nullptr);

        connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, [this]() {
            makeCurrent();
            if (m_api)
                m_api->setVideoSurfaceSize(m_api->object, -1, -1, nullptr);
            else if (p_gl_destroyed)
                p_gl_destroyed();
            doneCurrent();
        });
    }

    void resizeGL(int w, int h) override
    {
        if (!m_api) return;
        auto dpr = devicePixelRatioF();
        m_api->setVideoSurfaceSize(m_api->object,
                                    int(w * dpr), int(h * dpr), nullptr);
    }

    void paintGL() override
    {
        /* Clear background to black so we don't see previous windows */
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (!m_api) return;
        m_api->renderVideo(m_api->object, nullptr);
    }

private:
    mdkPlayerAPI *m_api;
    mdkGLRenderAPI m_glApi;
};

/* ── Custom Slider (Jumps to click position) ─────────────── */

class JumpSlider : public QSlider {
    Q_OBJECT
public:
    using QSlider::QSlider;

protected:
    void mousePressEvent(QMouseEvent *ev) override {
        if (ev->button() == Qt::LeftButton && orientation() == Qt::Horizontal) {
            int val = QStyle::sliderValueFromPosition(minimum(), maximum(), ev->pos().x(), width());
            setValue(val);
            
            /* By setting the value first, the handle moves under the cursor.
             * Then passing the event to QSlider causes it to start dragging
             * instead of doing a page-step. */
            QSlider::mousePressEvent(ev);
        } else {
            QSlider::mousePressEvent(ev);
        }
    }
};

/* ── Container with UI Controls ──────────────────────────── */

class MdkPlayerContainer : public QWidget {
    Q_OBJECT
public:
    explicit MdkPlayerContainer(QWidget *parent, const QString &file)
        : QWidget(parent), m_seeking(false)
    {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        m_videoWidget = new MdkVideoWidget(this, file);
        layout->addWidget(m_videoWidget, 1);

        auto *controlsLayout = new QHBoxLayout();
        controlsLayout->setContentsMargins(8, 4, 8, 4);

        m_playPauseBtn = new QPushButton("Pause", this);
        m_loopBtn = new QPushButton("∞ ⟳", this);
        m_loopBtn->setCheckable(true);
        m_slider = new JumpSlider(Qt::Horizontal, this);
        m_timeLabel = new QLabel("00:00 / 00:00", this);

        controlsLayout->addWidget(m_playPauseBtn);
        controlsLayout->addWidget(m_loopBtn);
        controlsLayout->addWidget(m_slider);
        controlsLayout->addWidget(m_timeLabel);

        auto *controlsWidget = new QWidget(this);
        controlsWidget->setLayout(controlsLayout);
        controlsWidget->setStyleSheet("background-color: #1a1a1a; color: #f0f0f0;"
                                      "QPushButton { background: #333; border: none; padding: 4px 12px; }"
                                      "QPushButton:hover { background: #444; }"
                                      "QPushButton:checked { background: #0078d7; font-weight: bold; }");
        layout->addWidget(controlsWidget, 0);

        connect(m_playPauseBtn, &QPushButton::clicked, this, &MdkPlayerContainer::togglePlayPause);
        connect(m_loopBtn, &QPushButton::toggled, this, &MdkPlayerContainer::toggleLoop);
        
        connect(m_slider, &QSlider::sliderPressed, this, [this]() { m_seeking = true; });
        connect(m_slider, &QSlider::sliderReleased, this, [this]() {
            m_seeking = false;
            seekTo(m_slider->value());
        });

        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &MdkPlayerContainer::updateControls);
        m_timer->start(250);
    }

private slots:
    void toggleLoop(bool checked)
    {
        auto *api = m_videoWidget->api();
        if (!api) return;
        /* -1 for infinite loop, 0 for no loop */
        api->setLoop(api->object, checked ? -1 : 0);
    }
    void togglePlayPause()
    {
        auto *api = m_videoWidget->api();
        if (!api) return;

        if (api->state(api->object) == MDK_State_Playing) {
            api->setState(api->object, MDK_State_Paused);
            m_playPauseBtn->setText("Play");
        } else {
            api->setState(api->object, MDK_State_Playing);
            m_playPauseBtn->setText("Pause");
        }
    }

    void seekTo(int posMs)
    {
        auto *api = m_videoWidget->api();
        if (!api) return;
        
        api->seek(api->object, posMs, mdkSeekCallback{});
    }

    QString formatTime(int64_t ms)
    {
        int totalSecs = ms / 1000;
        int mins = totalSecs / 60;
        int secs = totalSecs % 60;
        return QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
    }

    void updateControls()
    {
        auto *api = m_videoWidget->api();
        if (!api) return;

        int64_t pos = api->position(api->object);
        int64_t duration = 0;
        
        const mdkMediaInfo *info = api->mediaInfo(api->object);
        if (info) {
            duration = info->duration;
        }

        m_timeLabel->setText(formatTime(pos) + " / " + formatTime(duration));

        if (!m_seeking && duration > 0) {
            m_slider->setRange(0, duration);
            m_slider->setValue(pos);
        }
    }

private:
    MdkVideoWidget *m_videoWidget;
    QPushButton *m_playPauseBtn;
    QPushButton *m_loopBtn;
    QSlider *m_slider;
    QLabel *m_timeLabel;
    QTimer *m_timer;
    bool m_seeking;
};

#include "plugin.moc"

/* ── WLX exports ─────────────────────────────────────────── */

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char *FileToLoad, int ShowFlags)
{
    fprintf(stderr, "[mdk_wlx] ListLoad: file=%s\n", FileToLoad);

    auto *parent = reinterpret_cast<QWidget*>(ParentWin);
    auto *w = new MdkPlayerContainer(parent, QString::fromUtf8(FileToLoad));

    auto *existingLayout = parent->layout();
    if (!existingLayout) {
        auto *layout = new QVBoxLayout(parent);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(w);
    } else {
        existingLayout->addWidget(w);
    }

    w->show();

    fprintf(stderr, "[mdk_wlx] ListLoad: widget=%p\n", (void*)w);
    return w;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
    fprintf(stderr, "[mdk_wlx] ListCloseWindow\n");
    auto *w = reinterpret_cast<MdkPlayerContainer*>(ListWin);
    delete w;
}

int DCPCALL ListSearchDialog(HWND, int)
{
    return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND, int, int)
{
    return LISTPLUGIN_ERROR;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct*)
{
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
    const char* detect = "EXT=\"MP4\" | EXT=\"MKV\" | EXT=\"AVI\" | EXT=\"WEBM\" | EXT=\"FLV\" | EXT=\"MOV\" | EXT=\"WMV\" | EXT=\"MPEG\" | EXT=\"MPG\" | EXT=\"M4V\" | EXT=\"TS\" | EXT=\"VOB\" | EXT=\"MP3\" | EXT=\"FLAC\" | EXT=\"WAV\" | EXT=\"OGG\" | EXT=\"M4A\" | EXT=\"AAC\" | EXT=\"WMA\"";
    snprintf(DetectString, maxlen, "%s", detect);
}
