#include <QStyle>
#include <QtWidgets>
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QMimeDatabase>
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
	player->setVideoOutput(video);

	QHBoxLayout *controls = new QHBoxLayout;
	main->addWidget(video);
	main->addLayout(controls);
	QPushButton *bplay = new QPushButton(view);
	bplay->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
	QObject::connect(bplay, SIGNAL(clicked()), player, SLOT(play()));

	QPushButton *bpause = new QPushButton(view);
	bpause->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPause));
	QObject::connect(bpause, SIGNAL(clicked()), player, SLOT(pause()));

	QSlider *svolume = new QSlider(Qt::Horizontal, view);
	svolume->setRange(0, 100);
	QObject::connect(svolume, SIGNAL(sliderMoved(int)), player, SLOT(setVolume(int)));
	svolume->setValue(80);

	controls->addWidget(bplay);
	controls->addWidget(bpause);
	controls->addStretch(1);
	controls->addWidget(svolume);

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
