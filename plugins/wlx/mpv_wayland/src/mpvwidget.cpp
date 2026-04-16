#include "mpvwidget.h"
#include <QOpenGLContext>
#include <QWindow>
#include <QDebug>
#include <clocale>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QCoreApplication>

MpvWidget::MpvWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_mpv(nullptr)
    , m_mpvGL(nullptr)
    , m_glReady(false)
{
    setWindowFlags(Qt::Widget);
    setAttribute(Qt::WA_NativeWindow);
    setMouseTracking(true); // Needed so we get hover events for the OSC
    setFocusPolicy(Qt::WheelFocus); // Accepts keyboard inputs including wheel focus

    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->installEventFilter(this);
    }

    // mpv requires LC_NUMERIC=C or it will refuse to initialize
    setlocale(LC_NUMERIC, "C");

    m_mpv = mpv_create();
    if (!m_mpv) {
        qCritical() << "mpv_wayland: mpv_create() failed";
        return;
    }

    // Use the Render API — no native window embedding
    mpv_set_option_string(m_mpv, "vo", "libmpv");
    mpv_set_option_string(m_mpv, "keep-open", "yes");
    mpv_set_option_string(m_mpv, "hwdec", "no");
    // Enable the On Screen Controller
    mpv_set_option_string(m_mpv, "osc", "yes");
    // Suppress terminal output from mpv
    mpv_set_option_string(m_mpv, "terminal", "no");
    mpv_set_option_string(m_mpv, "msg-level", "all=no");
    
    // Enable default keybindings so our mapped keypresses actually do something
    mpv_set_option_string(m_mpv, "input-default-bindings", "yes");
    // Enable input to the OSC
    mpv_set_option_string(m_mpv, "input-vo-keyboard", "yes");

    int err = mpv_initialize(m_mpv);
    if (err < 0) {
        qCritical() << "mpv_wayland: mpv_initialize() failed:" << err;
        mpv_terminate_destroy(m_mpv);
        m_mpv = nullptr;
        return;
    }
}

MpvWidget::~MpvWidget()
{
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->removeEventFilter(this);
    }

    makeCurrent();
    if (m_mpvGL) {
        mpv_render_context_free(m_mpvGL);
        m_mpvGL = nullptr;
    }
    doneCurrent();
    if (m_mpv) {
        mpv_terminate_destroy(m_mpv);
        m_mpv = nullptr;
    }
}

void MpvWidget::on_update(void *ctx)
{
    MpvWidget *widget = static_cast<MpvWidget*>(ctx);
    // Thread-safe: callback comes from mpv's background thread, post to Qt main loop
    QMetaObject::invokeMethod(widget, "onMpvUpdate", Qt::QueuedConnection);
}

void *MpvWidget::get_proc_address(void *ctx, const char *name)
{
    Q_UNUSED(ctx);
    QOpenGLContext *glctx = QOpenGLContext::currentContext();
    if (!glctx) return nullptr;
    return reinterpret_cast<void*>(glctx->getProcAddress(name));
}

void MpvWidget::initializeGL()
{
    if (!m_mpv) {
        qCritical() << "mpv_wayland: initializeGL called but m_mpv is null";
        return;
    }

    mpv_opengl_init_params gl_init_params = {
        get_proc_address,
        nullptr
    };

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, (void *)MPV_RENDER_API_TYPE_OPENGL},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    int err = mpv_render_context_create(&m_mpvGL, m_mpv, params);
    if (err < 0) {
        qCritical() << "mpv_wayland: mpv_render_context_create() failed:" << err;
        return;
    }

    mpv_render_context_set_update_callback(m_mpvGL, on_update, this);
    m_glReady = true;

    // If a file was queued before GL was ready, load it now
    if (!m_pendingFile.isEmpty()) {
        QMetaObject::invokeMethod(this, "doLoadFile", Qt::QueuedConnection);
    }
}

void MpvWidget::paintGL()
{
    if (!m_mpvGL) return;

    // Use physical pixels for Wayland HiDPI scaling
    qreal dpr = devicePixelRatioF();
    int w = static_cast<int>(width() * dpr);
    int h = static_cast<int>(height() * dpr);
    int fbo = defaultFramebufferObject();

    mpv_opengl_fbo mpfbo = {
        fbo,
        w,
        h,
        0
    };

    int flip_y = 1;

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    mpv_render_context_render(m_mpvGL, params);
}

void MpvWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

bool MpvWidget::loadFile(const QString &fileName)
{
    if (!m_mpv) return false;

    m_pendingFile = fileName;

    if (m_glReady) {
        // GL is already ready, load immediately
        doLoadFile();
        return true;
    }

    // GL not ready yet — file will be loaded in initializeGL via deferred call
    return true;
}

void MpvWidget::doLoadFile()
{
    if (!m_mpv || m_pendingFile.isEmpty()) return;

    QByteArray utf8 = m_pendingFile.toUtf8();
    const char *args[] = {"loadfile", utf8.constData(), nullptr};
    int err = mpv_command(m_mpv, args);

    if (err < 0) {
        qCritical() << "mpv_wayland: loadfile failed:" << err << m_pendingFile;
    }
}

void MpvWidget::closeFile()
{
    if (!m_mpv) return;
    const char *args[] = {"stop", nullptr};
    mpv_command(m_mpv, args);
    m_pendingFile.clear();
}

void MpvWidget::onMpvUpdate()
{
    update();
}

void MpvWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_mpv) return;
    int x = event->x() * devicePixelRatioF();
    int y = event->y() * devicePixelRatioF();
    QString cmd = QString("mouse %1 %2").arg(x).arg(y);
    mpv_command_string(m_mpv, cmd.toUtf8().constData());
}

void MpvWidget::mousePressEvent(QMouseEvent *event)
{
    setFocus(); // Explicitly request focus when clicked so we can receive keyboard events
    grabKeyboard(); // Force grab all keyboard input (prevents Double Commander from stealing it)

    if (!m_mpv) return;
    if (event->button() == Qt::LeftButton) {
        mpv_command_string(m_mpv, "keydown MOUSE_BTN0");
    } else if (event->button() == Qt::RightButton) {
        mpv_command_string(m_mpv, "keydown MOUSE_BTN2");
    }
}

void MpvWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_mpv) return;
    if (event->button() == Qt::LeftButton) {
        mpv_command_string(m_mpv, "keyup MOUSE_BTN0");
    } else if (event->button() == Qt::RightButton) {
        mpv_command_string(m_mpv, "keyup MOUSE_BTN2");
    }
}

void MpvWidget::wheelEvent(QWheelEvent *event)
{
    if (!m_mpv) return;
    if (event->angleDelta().y() > 0) {
        mpv_command_string(m_mpv, "keypress WHEEL_UP");
    } else {
        mpv_command_string(m_mpv, "keypress WHEEL_DOWN");
    }
}

void MpvWidget::leaveEvent(QEvent *event)
{
    if (!m_mpv) return;
    // Release keyboard grab if the user moves their mouse away from the video
    releaseKeyboard();
    
    // Move mouse out of frame to hide OSC
    mpv_command_string(m_mpv, "mouse -100 -100");
}

QString MpvWidget::mapQtKeyToMpvKey(QKeyEvent *event)
{
    switch(event->key()) {
        case Qt::Key_Space: return "SPACE";
        case Qt::Key_Left: return "LEFT";
        case Qt::Key_Right: return "RIGHT";
        case Qt::Key_Up: return "UP";
        case Qt::Key_Down: return "DOWN";
        case Qt::Key_Enter:
        case Qt::Key_Return: return "ENTER";
        case Qt::Key_Escape: return "ESC";
        case Qt::Key_Backspace: return "BS";
        case Qt::Key_PageUp: return "PGUP";
        case Qt::Key_PageDown: return "PGDWN";
        case Qt::Key_Home: return "HOME";
        case Qt::Key_End: return "END";
        case Qt::Key_Tab: return "TAB";
    }
    // Return the text character if available
    QString text = event->text();
    if (!text.isEmpty()) {
        return text;
    }
    return "";
}

void MpvWidget::keyPressEvent(QKeyEvent *event)
{
    if (!m_mpv) return;
    QString mpvKey = mapQtKeyToMpvKey(event);
    if (!mpvKey.isEmpty()) {
        QString cmd = QString("keypress %1").arg(mpvKey); // Send keypress directly instead of keydown to avoid stuck keys
        mpv_command_string(m_mpv, cmd.toUtf8().constData());
    }
}

void MpvWidget::keyReleaseEvent(QKeyEvent *event)
{
    // Ignore key release since we sent a full keypress above.
    // If you need exact down/up tracking, this can be reverted to keyup.
    Q_UNUSED(event);
}

bool MpvWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        // If the mouse is hovering over the video, steal all keyboard events globally
        if (underMouse() || hasFocus()) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (event->type() == QEvent::KeyPress) {
                keyPressEvent(keyEvent);
            } else {
                keyReleaseEvent(keyEvent);
            }
            return true; // Eat the event so DC doesn't get it
        }
    }
    return QOpenGLWidget::eventFilter(obj, event);
}

