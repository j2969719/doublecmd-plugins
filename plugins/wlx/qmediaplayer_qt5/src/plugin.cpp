#include <QtWidgets>
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QMimeDatabase>
#include <QMediaMetaData>
#include <QVideoSurfaceFormat>
#include <QAbstractVideoSurface>
#include "wlxplugin.h"

Q_DECLARE_METATYPE(QMediaPlayer *)
QMimeDatabase db;

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QVariant vplayer;
	QMimeType type = db.mimeTypeForFile(QString(FileToLoad));

	if (!type.name().contains("audio") && !type.name().contains("video"))
		return NULL;

	QFrame *view = new QFrame((QWidget*)ParentWin);

	QMediaPlayer *player = new QMediaPlayer((QObject*)view);
	player->setMedia(QUrl::fromLocalFile(FileToLoad));
	player->play();

	QVBoxLayout *main = new QVBoxLayout(view);

	QVideoWidget *video = new QVideoWidget(view);
	player->setVideoOutput(video->videoSurface());
	main->addWidget(video);

	QSlider *spos = new QSlider(Qt::Horizontal, view);
	spos->setRange(0, player->duration() / 1000);

	QObject::connect(spos, &QSlider::sliderMoved, [player](int x)
	{
		player->setPosition(x * 1000);
	});

	QObject::connect(player, &QMediaPlayer::durationChanged, [spos](qint64 x)
	{
		spos->setMaximum(x / 1000);
	});

	QObject::connect(player, &QMediaPlayer::positionChanged, [spos](qint64 x)
	{
		spos->setValue(x / 1000);
	});

	main->addWidget(spos);

	QHBoxLayout *controls = new QHBoxLayout;

	QPushButton *bplay = new QPushButton(view);
	bplay->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
	QObject::connect(bplay, SIGNAL(clicked()), player, SLOT(play()));

	QPushButton *bpause = new QPushButton(view);
	bpause->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPause));
	QObject::connect(bpause, SIGNAL(clicked()), player, SLOT(pause()));

	QObject::connect(player, QOverload<const QString &, const QVariant &>::of(&QMediaObject::metaDataChanged), [video](const QString & key, const QVariant & value)
	{
		if (key == "CoverArtImage")
		{
			QImage image = value.value<QImage>().convertToFormat(QImage::Format_ARGB32);
			QVideoSurfaceFormat format(image.size(), QVideoFrame::Format_ARGB32);
			video->videoSurface()->start(format);
			video->videoSurface()->present(image);
		}
	});


	QSlider *svolume = new QSlider(Qt::Horizontal, view);
	svolume->setRange(0, 100);
	QObject::connect(svolume, SIGNAL(sliderMoved(int)), player, SLOT(setVolume(int)));
	svolume->setValue(80);

	controls->addWidget(bplay);
	controls->addWidget(bpause);
	controls->addStretch(1);
	controls->addWidget(svolume);
	main->addLayout(controls);

	vplayer.setValue(player);
	view->setProperty("player", vplayer);
	view->show();

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QWidget *view = (QWidget*)PluginWin;
	QMediaPlayer *player = view->property("player").value<QMediaPlayer *>();

	QMimeType type = db.mimeTypeForFile(QString(FileToLoad));

	if (type.name().contains("audio") || type.name().contains("video"))
	{
		player->setMedia(QUrl::fromLocalFile(FileToLoad));
		player->play();
		return LISTPLUGIN_OK;
	}

	return LISTPLUGIN_ERROR;
}


void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QMediaPlayer *view = (QMediaPlayer*)ListWin;
	QMediaPlayer *player = view->property("player").value<QMediaPlayer *>();

	delete player;
	delete view;
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}
