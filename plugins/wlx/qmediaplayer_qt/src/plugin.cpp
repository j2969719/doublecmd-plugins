#include <QtWidgets>
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QMimeDatabase>
#include <QMediaMetaData>

#if QT_VERSION >= 0x060000
#include <QAudioOutput>
#else
#include <QMediaPlaylist>
#include <QVideoSurfaceFormat>
#include <QAbstractVideoSurface>
#endif

#include "wlxplugin.h"

#define DURATION_EMPTY "-:--:--/-:--:--"
#define MY_TIME_FORMAT "u:%02u:%02u"
#define MY_TIME_IS_VALID(t) t != -1
#define MY_TIME_ARGS(t) \
        MY_TIME_IS_VALID (t) ? \
        (uint) (t / (1000 * 60 * 60)) : 99, \
        MY_TIME_IS_VALID (t) ? \
        (uint) ((t / (1000 * 60)) % 60) : 99, \
        MY_TIME_IS_VALID (t) ? \
        (uint) ((t / 1000) % 60) : 99

Q_DECLARE_METATYPE(QMediaPlayer *)
QMimeDatabase db;

int gVolume = 80;
bool gMute = false;
bool gLoop = true;
QString gCfgPath;

static void update_mutebtn(QPushButton *bmute)
{
	if (!gMute)
		bmute->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaVolume));
	else
		bmute->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaVolumeMuted));
}

static void update_loopbtn(QPushButton *bloop)
{
	if (!gLoop)
		bloop->setText("once");
	else
		bloop->setText("loop");
}

#if QT_VERSION >= 0x060000
HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QVariant vplayer;
	QMimeType type = db.mimeTypeForFile(QString(FileToLoad));

	if (!type.name().contains("audio") && !type.name().contains("video"))
		return nullptr;

	QFrame *view = new QFrame((QWidget*)ParentWin);

	QMediaPlayer *player = new QMediaPlayer((QObject*)view);

	QObject::connect(player, &QMediaPlayer::errorChanged, [player, view]()
	{
		if (player->error() != QMediaPlayer::NoError)
			QMessageBox::critical(view, "", player->errorString());
	});

	QAudioOutput *audio = new QAudioOutput((QObject*)view);
	player->setAudioOutput(audio);

	QPushButton *bloop = new QPushButton(view);
	update_loopbtn(bloop);

	QObject::connect(bloop, &QPushButton::clicked, [bloop, player]()
	{
		gLoop = !gLoop;
		player->setLoops(gLoop ? QMediaPlayer::Infinite : QMediaPlayer::Once);
		update_loopbtn(bloop);
	});

	player->setLoops(gLoop ? QMediaPlayer::Infinite : QMediaPlayer::Once);

	QVBoxLayout *main = new QVBoxLayout(view);

	QVideoWidget *video = new QVideoWidget(view);
	player->setVideoOutput(video);
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
		spos->setProperty("duration", x);
	});

	QLabel *ltime = new QLabel(DURATION_EMPTY, view);

	QObject::connect(player, &QMediaPlayer::positionChanged, [spos, ltime](qint64 x)
	{
		spos->setValue(x / 1000);
		qint64 duration = spos->property("duration").value<qint64>();
		ltime->setText(QString::asprintf("%" MY_TIME_FORMAT "/%" MY_TIME_FORMAT, MY_TIME_ARGS(x), MY_TIME_ARGS(duration)));
	});

	main->addWidget(spos);

	QHBoxLayout *controls = new QHBoxLayout;

	QPushButton *bplay = new QPushButton(view);
	bplay->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
	QObject::connect(bplay, SIGNAL(clicked()), player, SLOT(play()));

	QPushButton *bpause = new QPushButton(view);
	bpause->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPause));
	QObject::connect(bpause, SIGNAL(clicked()), player, SLOT(pause()));


	QPushButton *binfo = new QPushButton(view);
	binfo->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));
	binfo->setFocusPolicy(Qt::NoFocus);

	QObject::connect(binfo, &QPushButton::clicked, [player, view]()
	{
		QString info;
		QList<QMediaMetaData::Key> keys = player->metaData().keys();

		for (qsizetype i = 0; i < keys.size(); ++i)
		{
			QString str;
			str.append(player->metaData().metaDataKeyToString(keys.at(i)).replace(QRegularExpression("([a-z])([A-Z])"), "\\1 \\2"));
			str.append(": ");
			str.append(player->metaData().stringValue(keys.at(i)));
			str.append("\n");
			info.append(str);

		}
		if (!info.isEmpty())
			QMessageBox::information(view, "", info);
		else
			QMessageBox::information(view, "", "no suitable info available");
	});

	QPushButton *bmute = new QPushButton(view);
	bmute->setFocusPolicy(Qt::NoFocus);
	audio->setMuted(gMute);
	update_mutebtn(bmute);

	QObject::connect(bmute, &QPushButton::clicked, [bmute, audio]()
	{
		gMute = !audio->isMuted();
		audio->setMuted(gMute);
		update_mutebtn(bmute);
	});

	QSlider *svolume = new QSlider(Qt::Horizontal, view);
	svolume->setRange(1, 100);

	QObject::connect(svolume, &QSlider::valueChanged, [svolume, audio]()
	{
		gVolume = svolume->value();
		audio->setVolume(QAudio::convertVolume(gVolume / qreal(100), QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale));
	});

	svolume->setValue(gVolume);

	controls->addWidget(bplay);
	controls->addWidget(bpause);
	controls->addSpacing(10);
	controls->addWidget(binfo);
	controls->addSpacing(5);
	controls->addWidget(ltime);
	controls->addSpacing(5);
	controls->addWidget(bloop);
	controls->addStretch(1);
	controls->addWidget(bmute);
	controls->addWidget(svolume);
	main->addLayout(controls);

	vplayer.setValue(player);
	view->setProperty("player", vplayer);
	view->show();
	player->setSource(QUrl::fromLocalFile(FileToLoad));
	player->play();

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QWidget *view = (QWidget*)PluginWin;
	QMediaPlayer *player = view->property("player").value<QMediaPlayer *>();

	QMimeType type = db.mimeTypeForFile(QString(FileToLoad));

	if (type.name().contains("audio") || type.name().contains("video"))
	{
		player->setSource(QUrl::fromLocalFile(FileToLoad));
		player->play();
		return LISTPLUGIN_OK;
	}

	return LISTPLUGIN_ERROR;
}
#else
HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QVariant vplayer;
	QMimeType type = db.mimeTypeForFile(QString(FileToLoad));

	if (!type.name().contains("audio") && !type.name().contains("video"))
		return nullptr;

	QFrame *view = new QFrame((QWidget*)ParentWin);
	view->setFrameStyle(QFrame::NoFrame);

	QMediaPlayer *player = new QMediaPlayer((QObject*)view);
	QMediaPlaylist *playlist = new QMediaPlaylist((QObject*)player);

	QPushButton *bloop = new QPushButton(view);
	update_loopbtn(bloop);

	QObject::connect(bloop, &QPushButton::clicked, [bloop, player, playlist]()
	{
		gLoop = !gLoop;
		playlist->setPlaybackMode(gLoop ? QMediaPlaylist::Loop : QMediaPlaylist::CurrentItemOnce);
		update_loopbtn(bloop);
	});

	playlist->setPlaybackMode(gLoop ? QMediaPlaylist::Loop : QMediaPlaylist::CurrentItemOnce);
	player->setPlaylist(playlist);

	QObject::connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), [view, player, bloop](QMediaPlayer::Error error)
	{
		gLoop = false;
		player->playlist()->setPlaybackMode(QMediaPlaylist::CurrentItemOnce);
		update_loopbtn(bloop);
		QMessageBox::critical(view, "", player->errorString());
	});

	QVBoxLayout *main = new QVBoxLayout(view);

	QVideoWidget *video = new QVideoWidget(view);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	player->setVideoOutput(video->videoSurface());
#else
	player->setVideoOutput(video);
#endif

	video->setAspectRatioMode(Qt::KeepAspectRatio);
	video->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	QPalette pal;
	pal.setColor(QPalette::Window, Qt::black);
	video->setPalette(pal);
	video->setAutoFillBackground(true);
	main->addWidget(video);

	QLabel *linfo = new QLabel(view);
	linfo->setAlignment(Qt::AlignCenter);
	linfo->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	main->addWidget(linfo);

	QLabel *lnum = new QLabel(view);
	lnum->hide();

	QObject::connect(playlist, &QMediaPlaylist::currentIndexChanged, [player, linfo, lnum](int x)
	{
		linfo->setText("");
		lnum->setText(QString("%1/%2").arg(x + 1).arg(player->playlist()->mediaCount()));
		player->playlist()->setPlaybackMode(QMediaPlaylist::Loop);
	});

	QSlider *spos = new QSlider(Qt::Horizontal, view);
	spos->setFocusPolicy(Qt::NoFocus);
	spos->setRange(0, player->duration() / 1000);

	QObject::connect(spos, &QSlider::sliderMoved, [player](int x)
	{
		player->setPosition(x * 1000);
	});

	QObject::connect(player, &QMediaPlayer::durationChanged, [spos](qint64 x)
	{
		spos->setMaximum(x / 1000);
		spos->setProperty("duration", x);
	});

	QLabel *ltime = new QLabel(DURATION_EMPTY, view);

	QObject::connect(player, &QMediaPlayer::positionChanged, [spos, ltime](qint64 x)
	{
		spos->setValue(x / 1000);
		qint64 duration = spos->property("duration").value<qint64>();
		ltime->setText(QString::asprintf("%" MY_TIME_FORMAT "/%" MY_TIME_FORMAT, MY_TIME_ARGS(x), MY_TIME_ARGS(duration)));
	});

	main->addWidget(spos);

	QHBoxLayout *controls = new QHBoxLayout;

	QPushButton *bplay = new QPushButton(view);
	bplay->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
	QObject::connect(bplay, &QPushButton::clicked, player, &QMediaPlayer::play);
	bplay->setFocusPolicy(Qt::NoFocus);

	QPushButton *bpause = new QPushButton(view);
	bpause->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPause));
	QObject::connect(bpause, &QPushButton::clicked, player, &QMediaPlayer::pause);
	bpause->setFocusPolicy(Qt::NoFocus);

	QPushButton *binfo = new QPushButton(view);
	binfo->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));
	binfo->setFocusPolicy(Qt::NoFocus);

	QObject::connect(binfo, &QPushButton::clicked, [playlist, view]()
	{
		QString info;
		QStringList keys =
		{
			"Title",
			"SubTitle",
			"Author",
			"LeadPerformer",
			"ContributingArtist",
			"Director",
			"Writer",
			"Composer",
			"AlbumArtist",
			"AlbumTitle",
			"TrackNumber",
			"ChapterNumber",
			"Comment",
			"Description",
			"Category",
			"Genre",
			"Year",
			"Date",
			"Language",
			"Publisher",
			"Copyright",
			"Conductor",
			"ParentalRating",
			"RatingOrganization",
			"Keywords",
			"MediaType",
			"Resolution",
			"VideoCodec",
			"AudioCodec",
		};

		for (int i = 0; i < keys.size(); ++i)
			if (playlist->mediaObject()->metaData(keys[i]).isValid())
			{
				QString str;
				str.append(QString(keys[i]).replace(QRegularExpression("([a-z])([A-Z])"), "\\1 \\2"));
				str.append(": ");

				if (playlist->mediaObject()->metaData(keys[i]).canConvert(QMetaType::QString))
					str.append(playlist->mediaObject()->metaData(keys[i]).toString());
				else if (playlist->mediaObject()->metaData(keys[i]).canConvert(QMetaType::QSize))
				{
					QSize size = playlist->mediaObject()->metaData(keys[i]).toSize();
					str.append(QString("%1x%2").arg(size.width()).arg(size.height()));
				}

				str.append("\n");
				info.append(str);
			}

		if (!info.isEmpty())
			QMessageBox::information(view, "", info);
		else
			QMessageBox::information(view, "", "no suitable info available");
	});

	QPushButton *bprev = new QPushButton(view);
	bprev->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowBack));
	QObject::connect(bprev, &QPushButton::clicked, playlist, &QMediaPlaylist::previous);
	bprev->setFocusPolicy(Qt::NoFocus);
	bprev->hide();

	QPushButton *bnext = new QPushButton(view);
	bnext->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowForward));
	QObject::connect(bnext, &QPushButton::clicked, playlist, &QMediaPlaylist::next);
	bnext->setFocusPolicy(Qt::NoFocus);
	bnext->hide();

	QObject::connect(playlist, &QMediaPlaylist::loaded, [bnext, bprev, lnum]()
	{
		bprev->show();
		bnext->show();
		lnum->show();
	});

	QObject::connect(playlist, &QMediaPlaylist::mediaRemoved, [linfo, player, bnext, bprev, lnum](int start, int end)
	{
		bprev->hide();
		bnext->hide();
		lnum->hide();
		player->playlist()->setPlaybackMode(QMediaPlaylist::Loop);
		linfo->setText("");
	});

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	QObject::connect(player, QOverload<const QString &, const QVariant &>::of(&QMediaObject::metaDataChanged), [video, linfo](const QString & key, const QVariant & value)
	{
		if (key == "CoverArtImage")
		{
			QImage image = value.value<QImage>().convertToFormat(QImage::Format_ARGB32);
			QVideoSurfaceFormat format(image.size(), QVideoFrame::Format_ARGB32);
			video->videoSurface()->start(format);
			video->videoSurface()->present(image);
		}
		else if (key == "ContributingArtist")
			linfo->setText(value.value<QString>());
		else if (key == "Title")
		{
			QString text = linfo->text();

			if (text.length() > 256 || text.contains("\t—\t"))
				text.clear();

			if (!text.isEmpty())
				text.append("\t—\t");

			text.append(value.value<QString>());
			linfo->setText(text);
		}
	});
#endif

	QPushButton *bmute = new QPushButton(view);
	bmute->setFocusPolicy(Qt::NoFocus);
	player->setMuted(gMute);
	update_mutebtn(bmute);


	QObject::connect(bmute, &QPushButton::clicked, [bmute, player]()
	{
		gMute = !player->isMuted();
		player->setMuted(gMute);
		update_mutebtn(bmute);
	});

	QSlider *svolume = new QSlider(Qt::Horizontal, view);
	svolume->setRange(1, 100);
	svolume->setValue(gVolume);
	player->setVolume(gVolume);

	QObject::connect(svolume, &QSlider::valueChanged, [svolume, player]()
	{
		gVolume = svolume->value();
		player->setVolume(gVolume);
	});

	svolume->setFocusPolicy(Qt::NoFocus);

	controls->addWidget(bplay);
	controls->addWidget(bpause);
	controls->addSpacing(10);
	controls->addWidget(binfo);
	controls->addSpacing(10);
	controls->addWidget(bprev);
	controls->addWidget(bnext);
	controls->addSpacing(10);
	controls->addWidget(lnum);
	controls->addSpacing(5);
	controls->addWidget(ltime);
	controls->addSpacing(5);
	controls->addWidget(bloop);
	controls->addStretch(1);
	controls->addWidget(bmute);
	controls->addWidget(svolume);
	main->addLayout(controls);

	if (type.name() == "audio/x-mpegurl")
		playlist->load(QUrl::fromLocalFile(FileToLoad));
	else
		playlist->addMedia(QUrl::fromLocalFile(FileToLoad));

	vplayer.setValue(player);
	view->setProperty("player", vplayer);
	view->show();

	player->play();

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QFrame *view = (QFrame*)PluginWin;
	QMediaPlayer *player = view->property("player").value<QMediaPlayer *>();

	QMimeType type = db.mimeTypeForFile(QString(FileToLoad));

	if (type.name().contains("audio") || type.name().contains("video"))
	{
		player->playlist()->clear();

		if (type.name() == "audio/x-mpegurl")
			player->playlist()->load(QUrl::fromLocalFile(FileToLoad));
		else
			player->playlist()->addMedia(QUrl::fromLocalFile(FileToLoad));

		player->play();
		return LISTPLUGIN_OK;
	}

	return LISTPLUGIN_ERROR;
}
#endif


void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QFrame *view = (QFrame*)ListWin;
	QMediaPlayer *player = view->property("player").value<QMediaPlayer *>();

	QSettings settings(gCfgPath, QSettings::IniFormat);
	settings.setValue(PLUGNAME "/volume", gVolume);
	settings.setValue(PLUGNAME "/mute", gMute);
	settings.setValue(PLUGNAME "/loop", gLoop);

	delete player;
	delete view;
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	QFileInfo defini(QString::fromStdString(dps->DefaultIniName));
	gCfgPath = defini.absolutePath() + "/j2969719.ini";
	QSettings settings(gCfgPath, QSettings::IniFormat);

	if (!settings.contains(PLUGNAME "/volume"))
		settings.setValue(PLUGNAME "/volume", gVolume);
	else
		gVolume = settings.value(PLUGNAME "/volume").toInt();

	if (!settings.contains(PLUGNAME "/mute"))
		settings.setValue(PLUGNAME "/mute", gMute);
	else
		gMute = settings.value(PLUGNAME "/mute").toBool();

	if (!settings.contains(PLUGNAME "/loop"))
		settings.setValue(PLUGNAME "/loop", gLoop);
	else
		gLoop = settings.value(PLUGNAME "/loop").toBool();
}
