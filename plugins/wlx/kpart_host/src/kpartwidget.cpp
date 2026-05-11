#include "kpartwidget.h"
#include <QMimeDatabase>
#include <KParts/ReadOnlyPart>
#include <KParts/PartLoader>
#include <KPluginMetaData>
#include <QUrl>
#include <QEvent>
#include <QCoreApplication>
#include <QApplication>
#include <QTimer>
#include <QChildEvent>

KPartWidget::KPartWidget(QWidget *parent)
    : QWidget(parent)
    , m_part(nullptr)
    , m_loadGeneration(0)
{
    // Prevent this widget from ever accepting keyboard focus.
    // Without this, Okular/Calligra cause focus to land here,
    // and arrow key events get consumed instead of reaching DC's file list.
    setFocusPolicy(Qt::NoFocus);

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // Install global event filter to intercept focus-stealing by KParts
    // at the application level, regardless of which widget they target.
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->installEventFilter(this);
    }
}

KPartWidget::~KPartWidget()
{
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->removeEventFilter(this);
    }
    if (m_part) {
        m_part->closeUrl();
        delete m_part;
    }
}

void KPartWidget::returnFocusToDC()
{
    QWidget *target = this->parentWidget();
    if (target) {
        target->setFocus(Qt::OtherFocusReason);
    }
    this->clearFocus();
    if (m_part && m_part->widget()) {
        m_part->widget()->clearFocus();
    }
}

void KPartWidget::installFocusGuard()
{
    if (!m_part || !m_part->widget()) return;

    QWidget *focusTarget = this->parentWidget();

    m_part->widget()->installEventFilter(this);

    // Set NoFocus policy and focus proxy on the root part widget and all
    // children. The focus proxy redirects any programmatic setFocus() calls
    // to the parent widget, and NoFocus prevents Tab/click focus acquisition.
    m_part->widget()->setAttribute(Qt::WA_NativeWindow, false);
    m_part->widget()->setFocusPolicy(Qt::NoFocus);
    if (focusTarget) {
        m_part->widget()->setFocusProxy(focusTarget);
    }

    for (QWidget *child : m_part->widget()->findChildren<QWidget*>()) {
        child->setAttribute(Qt::WA_NativeWindow, false);
        child->setFocusPolicy(Qt::NoFocus);
        child->installEventFilter(this);
        if (focusTarget) {
            child->setFocusProxy(focusTarget);
        }
    }
}

bool KPartWidget::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::ChildAdded: {
        // Okular/Calligra spawn widgets asynchronously (e.g. PageView).
        // Apply focus guards to each new child in our KPart's subtree.
        QChildEvent *ce = static_cast<QChildEvent*>(event);
        if (ce->child() && ce->child()->isWidgetType()) {
            QWidget *childWidget = static_cast<QWidget*>(ce->child());
            if (m_part && m_part->widget() &&
                (watched == m_part->widget() || m_part->widget()->isAncestorOf(static_cast<QWidget*>(watched)))) {
                childWidget->setAttribute(Qt::WA_NativeWindow, false);
                childWidget->setFocusPolicy(Qt::NoFocus);
                childWidget->installEventFilter(this);
                QWidget *focusTarget = this->parentWidget();
                if (focusTarget) {
                    childWidget->setFocusProxy(focusTarget);
                }
            }
        }
        break;
    }

    case QEvent::FocusIn: {
        // Global safety net: if focus lands on KPartWidget itself or any
        // widget inside the KPart's subtree, immediately restore focus to
        // DC's file list (saved before loading). This catches late async
        // focus steals by Okular during PDF/EPUB page rendering that occur
        // after the completed signal has already fired.
        if (watched->isWidgetType()) {
            QWidget *w = static_cast<QWidget*>(watched);

            bool isOurs = (w == this);
            if (!isOurs && m_part && m_part->widget()) {
                isOurs = (w == m_part->widget() || m_part->widget()->isAncestorOf(w));
            }

            if (isOurs && m_savedFocusWidget && m_savedFocusWidget != w) {
                m_savedFocusWidget->setFocus(Qt::OtherFocusReason);
            }
        }
        break;
    }

    default:
        break;
    }

    return QWidget::eventFilter(watched, event);
}

bool KPartWidget::loadFile(const QString &fileName)
{
    // Save which widget currently has focus (DC's file list) so we can
    // restore it after the KPart inevitably steals focus.
    m_savedFocusWidget = QApplication::focusWidget();

    // Increment generation to invalidate any queued callbacks from the
    // previous part before we tear it down.
    m_loadGeneration++;

    if (m_part) {
        returnFocusToDC();

        m_part->closeUrl();
        m_layout->removeWidget(m_part->widget());
        delete m_part;
        m_part = nullptr;
    }

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(fileName);

    // If the file is detected as a generic ZIP but has a more specific extension
    // (like .docx, .odt, etc.), prioritize the extension-based MIME type.
    if (mime.name() == QLatin1String("application/zip") || mime.isDefault()) {
        QMimeType extMime = db.mimeTypeForFile(fileName, QMimeDatabase::MatchExtension);
        if (!extMime.isDefault() && extMime.name() != mime.name()) {
            mime = extMime;
        }
    }

    QUrl url = QUrl::fromLocalFile(fileName);

    // Find all parts available for this MIME type
    QVector<KPluginMetaData> parts = KParts::PartLoader::partsForMimeType(mime.name());

    KPluginMetaData selectedPart;

    // First pass: look for specialized renderers (not archives, not terminal)
    for (const auto &metaData : parts) {
        QString pluginId = metaData.pluginId();
        if (pluginId.contains(QLatin1String("konsole"), Qt::CaseInsensitive) ||
            pluginId.contains(QLatin1String("arkpart"), Qt::CaseInsensitive) ||
            pluginId.contains(QLatin1String("kioarchive"), Qt::CaseInsensitive)) {
            continue;
        }
        selectedPart = metaData;
        break;
    }

    // Second pass: if no specialized renderer found, allow archive explorers as fallback
    if (!selectedPart.isValid()) {
        for (const auto &metaData : parts) {
            QString pluginId = metaData.pluginId();
            if (pluginId.contains(QLatin1String("konsole"), Qt::CaseInsensitive)) {
                continue;
            }
            if (pluginId.contains(QLatin1String("arkpart"), Qt::CaseInsensitive) ||
                pluginId.contains(QLatin1String("kioarchive"), Qt::CaseInsensitive)) {
                selectedPart = metaData;
                break;
            }
        }
    }

    if (selectedPart.isValid()) {
        m_pendingUrl = url;
        m_selectedPart = selectedPart;

        // Defer instantiation by 50ms so Double Commander can finish handling
        // the user's MouseRelease event on the file list. Without this delay,
        // complex KParts spin up Wayland grabs so fast that DC misses the
        // release and gets stuck in a phantom-drag mode.
        QTimer::singleShot(50, this, [this, gen = m_loadGeneration]() {
            if (gen == m_loadGeneration) {
                instantiatePart();
            }
        });

        return true;
    }

    return false;
}

void KPartWidget::instantiatePart()
{
    auto result = KParts::PartLoader::instantiatePart<KParts::ReadOnlyPart>(m_selectedPart, this, this);
    if (result) {
        m_part = result.plugin;

        m_layout->addWidget(m_part->widget());

        installFocusGuard();
        connect(m_part, &KParts::ReadOnlyPart::completed, this, [this]() {
            installFocusGuard();
            if (m_savedFocusWidget) {
                m_savedFocusWidget->setFocus(Qt::OtherFocusReason);
            }
        });
        connect(m_part, &KParts::ReadOnlyPart::completedWithPendingAction, this, [this]() {
            installFocusGuard();
            if (m_savedFocusWidget) {
                m_savedFocusWidget->setFocus(Qt::OtherFocusReason);
            }
        });

        m_part->openUrl(m_pendingUrl);

        // Immediately restore focus after opening (catches synchronous focus steals)
        if (m_savedFocusWidget) {
            m_savedFocusWidget->setFocus(Qt::OtherFocusReason);
        }
    }
}
