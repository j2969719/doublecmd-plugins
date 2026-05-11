#include "mpvwidget.h"
#include <QOpenGLContext>
#include <QWindow>
#include <QDebug>
#include <clocale>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QCoreApplication>
#include <QApplication>
#include <QTimer>
#include <QChildEvent>

MpvWidget::MpvWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_mpv(nullptr)
    , m_mpvGL(nullptr)
    , m_glReady(false)
{
    setWindowFlags(Qt::Widget);
    // LAYER 1: NEVER hold Qt focus. On Wayland, calling setFocus() on a
    // QOpenGLWidget creates a compositor-level subsurface focus lock that
    // cannot be released programmatically. Instead we intercept keyboard
    // events at the application level and forward them to mpv.
    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);

    // Application-level event filter for keyboard forwarding and
    // outside-click detection.
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
    // LAYER 3: Save DC's currently focused widget so we can return to it
    m_savedFocusWidget = QApplication::focusWidget();
    installFocusGuard();

    // Install event filter on top-level window to detect clicks outside
    // our bounds. On Wayland, the GL subsurface doesn't reliably receive
    // FocusOut when the user clicks on DC's main surface (source panel),
    // so we must detect this ourselves.
    if (!m_topLevelFilterInstalled) {
        if (QWidget *tlw = window()) {
            tlw->installEventFilter(this);
            m_topLevelFilterInstalled = true;
        }
    }

    if (!m_mpv) return false;

    m_pendingFile = fileName;

    if (m_glReady) {
        // GL is already ready, load immediately
        doLoadFile();
        // LAYER 3: Restore focus once UI is stable
        QTimer::singleShot(100, this, [this]() { restoreFocusToDC(); });
        return true;
    }

    // GL not ready yet — file will be loaded in initializeGL via deferred call
    // Restore focus via doLoadFile later
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

    // LAYER 3: Restore focus once UI is stable
    QTimer::singleShot(100, this, [this]() { restoreFocusToDC(); });
}

void MpvWidget::installFocusGuard()
{
    const auto children = findChildren<QWidget*>();
    for (QWidget *child : children) {
        child->setFocusPolicy(Qt::NoFocus);
        child->installEventFilter(this);
    }
}

void MpvWidget::restoreFocusToDC()
{
    if (m_savedFocusWidget) {
        m_savedFocusWidget->setFocus(Qt::OtherFocusReason);
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
    // Activate input mode. We do NOT call setFocus() — that would create
    // a Wayland subsurface focus lock. Instead, the application-level
    // event filter in eventFilter() forwards keyboard events to mpv.
    m_isActive = true;

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
    // Not used — keyboard handling is done in eventFilter() to avoid
    // needing Qt focus (which causes Wayland subsurface lock).
    Q_UNUSED(event);
}

void MpvWidget::keyReleaseEvent(QKeyEvent *event)
{
    Q_UNUSED(event);
}

bool MpvWidget::eventFilter(QObject *obj, QEvent *event)
{
    // ── Outside-click detector ──────────────────────────────────────────
    // If the user clicks ANYWHERE outside our widget while we're active,
    // deactivate immediately so DC regains full control.
    if (event->type() == QEvent::MouseButtonPress && m_isActive) {
        auto *me = static_cast<QMouseEvent*>(event);
        QPoint localPos = mapFromGlobal(me->globalPosition().toPoint());
        if (!rect().contains(localPos)) {
            m_isActive = false;
            return false; // let the click through to DC
        }
    }

    // ── Keyboard forwarding ─────────────────────────────────────────────
    // When active, intercept ALL key events at the application level and
    // forward them to mpv. This avoids ever calling setFocus() on the GL
    // widget, which would create a Wayland subsurface focus lock.
    if (event->type() == QEvent::KeyPress && m_isActive) {
        auto *ke = static_cast<QKeyEvent*>(event);

        // ESC: deactivate and return control to DC
        if (ke->key() == Qt::Key_Escape) {
            m_isActive = false;
            return true; // eat the ESC so DC doesn't process it
        }

        // Forward the key to mpv
        if (m_mpv) {
            QString mpvKey = mapQtKeyToMpvKey(ke);
            if (!mpvKey.isEmpty()) {
                QString cmd = QString("keypress %1").arg(mpvKey);
                mpv_command_string(m_mpv, cmd.toUtf8().constData());
                return true; // eat the event so DC doesn't get it
            }
        }
    }

    // ── FocusIn interceptor ──────────────────────────────────────────────
    // Prevent our widget or children from ever holding Qt focus.
    auto *w = qobject_cast<QWidget*>(obj);
    if (event->type() == QEvent::FocusIn && w && (w == this || this->isAncestorOf(w))) {
        QTimer::singleShot(0, this, [this]() { restoreFocusToDC(); });
        return false;
    }

    // ── ChildAdded: guard dynamically-created children ──────────────────
    if (event->type() == QEvent::ChildAdded) {
        auto *ce = static_cast<QChildEvent*>(event);
        if (auto *childWidget = qobject_cast<QWidget*>(ce->child())) {
            childWidget->setFocusPolicy(Qt::NoFocus);
            childWidget->installEventFilter(this);
        }
    }

    return QOpenGLWidget::eventFilter(obj, event);
}
