#include <QtWidgets>

#include <dlfcn.h>
#include <libintl.h>
#include <locale.h>

#include "wlxplugin.h"

#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

class ImageViewer : public QWidget
{
		Q_OBJECT
	public:
		explicit ImageViewer(QWidget *parent = nullptr, bool isQuickView = false):
			QWidget(parent), m_rotation(0), m_zoomFactor(1.0),
			m_isFlipH(false), m_isFlipV(false), m_isFit(true), m_isStatic(false)
		{
			auto *mainLayout = new QVBoxLayout(this);
			mainLayout->setContentsMargins(0, 0, 0, 0);
			mainLayout->setSpacing(0);

			QToolBar *toolbar = new QToolBar(this);
			toolbar->addAction(QIcon::fromTheme("zoom-in"), _("Zoom In"), [this]()
			{
				scaleImage(1.1);
			});
			toolbar->addAction(QIcon::fromTheme("zoom-out"), _("Zoom Out"), [this]()
			{
				scaleImage(0.9);
			});
			toolbar->addAction(QIcon::fromTheme("zoom-original"), _("Original Size"), [this]()
			{
				m_isFit = false;
				m_zoomFactor = 1.0;
				updateZoom();
			});
			toolbar->addAction(QIcon::fromTheme("zoom-fit-best"), _("Fit"), [this]()
			{
				m_isFit = true;
				fitToParent();
			});
			toolbar->addSeparator();
			QAction *copyAct = toolbar->addAction(QIcon::fromTheme("edit-copy"), _("Copy to Clipboard"));
			connect(copyAct, &QAction::triggered, this, &ImageViewer::copyToClipboard);
			toolbar->addAction(QIcon::fromTheme("object-rotate-left"), _("Rotate"), [this]()
			{
				rotate(-90);
			});
			toolbar->addAction(QIcon::fromTheme("object-rotate-right"), _("Rotate Clockwise"), [this]()
			{
				rotate(90);
			});
			toolbar->addAction(QIcon::fromTheme("object-flip-horizontal"), _("Flip Horizontally"), [this]()
			{
				m_isFlipH = !m_isFlipH;
				updateZoom();
			});
			toolbar->addAction(QIcon::fromTheme("object-flip-vertical"), _("Flip Vertically"), [this]()
			{
				m_isFlipV = !m_isFlipV;
				updateZoom();
			});
			m_actPlayPause = toolbar->addAction(style()->standardIcon(QStyle::SP_MediaPause), _("Stop Animation"));
			connect(m_actPlayPause, &QAction::triggered, this, &ImageViewer::togglePlayPause);
			toolbar->addSeparator();
			m_lblSize = new QLabel("0x0", this);

			toolbar->addWidget(m_lblSize);
			mainLayout->addWidget(toolbar);

			if (isQuickView)
				toolbar->hide();

			m_scrollArea = new QScrollArea(this);
			m_scrollArea->setAlignment(Qt::AlignCenter);
			m_scrollArea->setWidgetResizable(false);

			m_mamkimCanvas = new QLabel;
			m_mamkimCanvas->setAlignment(Qt::AlignCenter);

			m_movie = new QMovie(this);

			connect(m_movie, &QMovie::frameChanged, this, [this](int)
			{
				updateZoom();
			});

			m_scrollArea->setWidget(m_mamkimCanvas);
			mainLayout->addWidget(m_scrollArea);
		}

		bool loadFile(const QString &filename)
		{
			QImageReader reader(filename);

			if (!reader.canRead())
				return false;

			m_movie->stop();

			m_rotation = 0;
			m_isFlipH = false;
			m_isFlipV = false;
			m_isStatic = false;
			m_mamkimCanvas->setMovie(nullptr);

			if (reader.supportsAnimation() && reader.imageCount() > 1)
			{
				m_movie->setFileName(filename);

				if (!m_movie->isValid())
					return false;

				m_movie->start();
				m_mamkimCanvas->setMovie(m_movie);
				m_orgSize = m_movie->currentPixmap().size();
			}
			else
			{
				m_isStatic = true;
				m_staticPixmap = QPixmap(filename);

				if (m_staticPixmap.isNull())
					return false;

				m_mamkimCanvas->setMovie(nullptr);
				m_orgSize = m_staticPixmap.size();
			}

			m_isFit = true;
			updatePlayPauseStatus();

			QTimer::singleShot(50, this, [this]()
			{
				fitToParent();
			});

			return true;
		}

		void copyToClipboard()
		{
			if (m_isStatic && !m_staticPixmap.isNull())
				QApplication::clipboard()->setPixmap(m_staticPixmap);
			else if (m_movie->isValid())
			{
				QSize oldSize = m_movie->scaledSize();
				m_movie->setScaledSize(QSize());
				QApplication::clipboard()->setPixmap(m_movie->currentPixmap());
				m_movie->setScaledSize(oldSize);
			}
		}

	protected:
		void resizeEvent(QResizeEvent *event) override
		{
			QWidget::resizeEvent(event);

			if (m_isFit)
				fitToParent();
		}

		void wheelEvent(QWheelEvent *event) override
		{
			if (event->modifiers() & Qt::ControlModifier)
			{
				scaleImage(event->angleDelta().y() > 0 ? 1.1 : 0.9);
				event->accept();
			}
			else
				QWidget::wheelEvent(event);
		}

	private:
		void togglePlayPause()
		{
			if (!m_isStatic)
			{
				m_movie->setPaused(m_movie->state() == QMovie::Running);
				updatePlayPauseStatus();
			}
		}

		void updatePlayPauseStatus()
		{
			m_actPlayPause->setVisible(!m_isStatic);

			if (!m_isStatic)
			{
				if (m_movie->state() == QMovie::Running)
				{
					m_actPlayPause->setText(_("Stop Animation"));
					m_actPlayPause->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
				}
				else
				{
					m_actPlayPause->setText(_("Play Animation"));
					m_actPlayPause->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
				}
			}
		}

		void rotate(int delta)
		{
			m_isFit = false;
			m_rotation = (m_rotation + delta) % 360;

			if (m_rotation < 0)
				m_rotation += 360;

			updateZoom();
		}

		void scaleImage(double factor)
		{
			m_isFit = false;
			m_zoomFactor = qBound(0.01, m_zoomFactor * factor, 20.0);
			updateZoom();
		}

		void fitToParent()
		{
			if (m_orgSize.isEmpty())
				return;

			QSize viewSize = m_scrollArea->viewport()->size();

			if (viewSize.isEmpty())
				return;

			QSize targetSize = m_orgSize;

			if (m_rotation == 90 || m_rotation == 270)
				targetSize.transpose();

			if (targetSize.width() <= viewSize.width() && targetSize.height() <= viewSize.height())
				m_zoomFactor = 1.0;
			else
			{
				double w = (double)viewSize.width() / targetSize.width();
				double h = (double)viewSize.height() / targetSize.height();
				m_zoomFactor = qMin(w, h);
			}

			updateZoom();
		}

		void updateZoom()
		{
			if (m_orgSize.isEmpty())
				return;

			QSize scaledSize = m_orgSize * m_zoomFactor;

			QTransform trans;
			trans.scale(m_isFlipH ? -1 : 1, m_isFlipV ? -1 : 1);
			trans.rotate(m_rotation);

			if (m_isStatic)
			{
				QPixmap pix = m_staticPixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
				m_mamkimCanvas->setPixmap(pix.transformed(trans, Qt::SmoothTransformation));
			}
			else
			{
				m_movie->setScaledSize(scaledSize);
				QPixmap frame = m_movie->currentPixmap().transformed(trans, Qt::SmoothTransformation);
				m_mamkimCanvas->setPixmap(frame);
			}

			QSize displaySize = scaledSize;

			if (m_rotation == 90 || m_rotation == 270)
				displaySize.transpose();

			m_mamkimCanvas->setFixedSize(displaySize);

			QString text = QString("%1x%2").arg(m_orgSize.width()).arg(m_orgSize.height());

			if (m_zoomFactor != 1)
				text = QString::asprintf("%dx%d (%dx%d %.0f%%)", m_orgSize.width(), m_orgSize.height(),
				                         scaledSize.width(), scaledSize.height(), m_zoomFactor * 100);

			m_lblSize->setText(text);
		}

		int m_rotation;
		QSize m_orgSize;
		QMovie *m_movie;
		double m_zoomFactor;
		QPixmap m_staticPixmap;
		QAction *m_actPlayPause;
		QScrollArea *m_scrollArea;
		QLabel *m_mamkimCanvas, *m_lblSize;
		bool m_isFlipH, m_isFlipV, m_isFit, m_isStatic;
};

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	bool isQuickView = false;
	QWidget *parent = (QWidget*)ParentWin;

	while (parent -> parentWidget() != Q_NULLPTR)
		parent = parent->parentWidget();

	if (parent->windowRole() != "TfrmViewer")
		isQuickView = true;

	ImageViewer *view = new ImageViewer((QWidget*)ParentWin, isQuickView);

	if (!view->loadFile(FileToLoad))
	{
		delete view;
		return nullptr;
	}

	view->show();

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	ImageViewer *view = (ImageViewer*)PluginWin;

	if (view->loadFile(FileToLoad))
		return LISTPLUGIN_OK;

	return LISTPLUGIN_ERROR;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	ImageViewer *view = (ImageViewer*)ListWin;

	delete view;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	ImageViewer *view = (ImageViewer*)ListWin;

	switch (Command)
	{
	case lc_copy :
		view->copyToClipboard();
		break;

	default :
		return LISTPLUGIN_ERROR;
	}

	return LISTPLUGIN_OK;
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	static char plg_path[PATH_MAX];
	const char* loc_dir = "langs";

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(plg_path, &dlinfo) != 0)
	{
		strncpy(plg_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(plg_path, '/');

		if (pos)
			strcpy(pos + 1, loc_dir);

		setlocale(LC_ALL, "");
		bindtextdomain(GETTEXT_PACKAGE, plg_path);
		textdomain(GETTEXT_PACKAGE);
	}
}

#include "plugin.moc"
