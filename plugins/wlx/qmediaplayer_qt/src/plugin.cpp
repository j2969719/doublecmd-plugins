#include <QtWidgets>
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QMimeDatabase>
#include <QMediaMetaData>

#include <dlfcn.h>
#include <libintl.h>
#include <locale.h>

#if QT_VERSION >= 0x060000
#include <QAudioOutput>
#else
#include <QMediaPlaylist>
#endif

#include "wlxplugin.h"

#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

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

QMimeDatabase gDB;
int gVolume = 80;
int gFontSize = 12;
bool gMute = false;
bool gLoop = true;
bool gSaveSettings = true;
QString gCfgPath;

#if QT_VERSION >= 0x060000
static QMap<const char*, QMediaMetaData::Key> gMetaFields
#else
static QMap<const char*, QString> gMetaFields
#endif
{
	{_("Artist"),	   QMediaMetaData::ContributingArtist},
	{_("Title"), 			QMediaMetaData::Title},
	{_("Album"),		   QMediaMetaData::AlbumTitle},
	{_("Track Number"),	  QMediaMetaData::TrackNumber},
	{_("Album Artist"),	  QMediaMetaData::AlbumArtist},
	{_("Author"),		       QMediaMetaData::Author},
	{_("Composer"),		     QMediaMetaData::Composer},
	{_("Lead Performer"),	QMediaMetaData::LeadPerformer},
	{_("Publisher"),	    QMediaMetaData::Publisher},
	{_("Genre"),			QMediaMetaData::Genre},
	{_("Duration"),		     QMediaMetaData::Duration},
	{_("Description"),	  QMediaMetaData::Description},
	{_("Copyright"), 	    QMediaMetaData::Copyright},
	{_("Comment"), 		      QMediaMetaData::Comment},
	{_("Resolution"),	   QMediaMetaData::Resolution},
	{_("Media Type"), 	    QMediaMetaData::MediaType},
	{_("Video Codec"), 	   QMediaMetaData::VideoCodec},
	{_("Video FrameRate"), QMediaMetaData::VideoFrameRate},
	{_("Video BitRate"),	 QMediaMetaData::VideoBitRate},
	{_("Audio Codec"),	   QMediaMetaData::AudioCodec},
	{_("Audio BitRate"),	 QMediaMetaData::AudioBitRate},
	{_("Date"),			 QMediaMetaData::Date},
};


static void setPlayerMedia(QMediaPlayer *player, char* FileToLoad, QString type)
{
#if QT_VERSION >= 0x060000
	player->setSource(QUrl::fromLocalFile(FileToLoad));
#else
	player->playlist()->clear();

	if (type == "audio/x-mpegurl")
		player->playlist()->load(QUrl::fromLocalFile(FileToLoad));
	else
		player->playlist()->addMedia(QUrl::fromLocalFile(FileToLoad));
#endif
}

static void setPlayerPlayPause(QMediaPlayer *player, QPushButton *btnPlay)
{
#if QT_VERSION >= 0x060000
	if (player->playbackState() != QMediaPlayer::PlayingState)
#else
	if (player->state() != QMediaPlayer::PlayingState)
#endif
		player->play();
	else
		player->pause();
}

static void setPlayerVolume(QMediaPlayer *player, QPushButton *btnMute)
{
#if QT_VERSION >= 0x060000
	player->audioOutput()->setMuted(gMute);
	player->audioOutput()->setVolume(QAudio::convertVolume(gVolume / qreal(100),
	                                 QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale));
#else
	player->setMuted(gMute);
	player->setVolume(gVolume);
#endif
	btnMute->setDown(gMute);

	if (!gMute)
	{
		if (gVolume > 70)
			btnMute->setIcon(QIcon::fromTheme("audio-volume-high"));
		else if (gVolume > 30)
			btnMute->setIcon(QIcon::fromTheme("audio-volume-medium"));
		else
			btnMute->setIcon(QIcon::fromTheme("audio-volume-low"));
	}
	else
		btnMute->setIcon(QIcon::fromTheme("audio-volume-muted"));
}

static void setPlayerLoop(QMediaPlayer *player, QPushButton *btnLoop)
{
#if QT_VERSION >= 0x060000
	player->setLoops(gLoop ? QMediaPlayer::Infinite : QMediaPlayer::Once);
#else
	player->playlist()->setPlaybackMode(gLoop ? QMediaPlaylist::Loop : QMediaPlaylist::Sequential);
#endif
	btnLoop->setDown(gLoop);
}


HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QVariant varPlayer;
	QMimeType type = gDB.mimeTypeForFile(QString(FileToLoad));

	if (!type.name().contains("audio") && !type.name().contains("video"))
		return nullptr;

#if QT_VERSION >= 0x060000
	if (type.name() == "audio/x-mpegurl")
		return nullptr;
#endif

	QFrame *view = new QFrame((QWidget*)ParentWin);
	view->setFrameStyle(QFrame::NoFrame);
	view->setFocusPolicy(Qt::NoFocus);

	QMediaPlayer *player = new QMediaPlayer((QObject*)view);

#if QT_VERSION >= 0x060000
	QAudioOutput *audio = new QAudioOutput((QObject*)view);
	player->setAudioOutput(audio);
	player->audioOutput();
#else
	QMediaPlaylist *playlist = new QMediaPlaylist((QObject*)player);
	player->setPlaylist(playlist);
#endif

	QVBoxLayout *main = new QVBoxLayout(view);

	QPalette pal;
	pal.setColor(QPalette::Window, Qt::black);
	pal.setColor(QPalette::Text, Qt::white);
	QFrame *content = new QFrame(view);
	content->setFrameStyle(QFrame::NoFrame);
	content->setAutoFillBackground(true);
	content->setPalette(pal);
	main->addWidget(content);
	QVBoxLayout *box = new QVBoxLayout(content);

	QVideoWidget *video = new QVideoWidget(view);
	video->setAspectRatioMode(Qt::KeepAspectRatio);
	video->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	video->setAutoFillBackground(true);
	video->setPalette(pal);
	player->setVideoOutput(video);
	video->hide();
	box->addWidget(video);

	QLabel *lblThumb = new QLabel(view);
	lblThumb->setAlignment(Qt::AlignCenter);
	lblThumb->setAutoFillBackground(true);
	lblThumb->setPalette(pal);
	lblThumb->hide();
	box->addWidget(lblThumb);

	QLabel *lblMeta = new QLabel(view);
	lblMeta->setAlignment(Qt::AlignCenter);
	lblMeta->setTextInteractionFlags(Qt::TextSelectableByMouse);
	lblMeta->setTextFormat(Qt::RichText);
	lblMeta->setAutoFillBackground(true);
	lblMeta->setPalette(pal);

	QScrollArea *scroll = new QScrollArea(view);
	scroll->setAlignment(Qt::AlignCenter);
	scroll->setWidget(lblMeta);
	scroll->setWidgetResizable(true);
	scroll->setFrameStyle(QFrame::NoFrame);
	scroll->setAutoFillBackground(true);
	scroll->setPalette(pal);
	box->addWidget(scroll);

	QFont font = lblMeta->font();
	font.setPointSize(gFontSize);
	lblMeta->setFont(font);

	QLabel *lblError = new QLabel(view);
	lblError->setAlignment(Qt::AlignCenter);
	lblError->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	lblError->hide();
	main->addWidget(lblError);

#if QT_VERSION >= 0x060000
	QObject::connect(player, &QMediaPlayer::errorChanged, [player, lblError]()
	{
		if (player->error() != QMediaPlayer::NoError)
		{
			player->stop();
			lblError->setText(QString("ERROR: %1").arg(player->errorString()));
			lblError->show();
		}
		else
			lblError->hide();
#else
	QObject::connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), [player, lblError](QMediaPlayer::Error error)
	{
		player->playlist()->setPlaybackMode(QMediaPlaylist::CurrentItemOnce);
		player->stop();
		lblError->setText(QString("ERROR: %1").arg(player->errorString()));
		lblError->show();
	});

	QObject::connect(player, &QMediaPlayer::currentMediaChanged, [lblMeta, lblError, video, player](const QMediaContent & media)
	{
		if (!media.isNull())
		{
			lblMeta->setText("");
			lblError->hide();
			player->playlist()->setPlaybackMode(gLoop ? QMediaPlaylist::Loop : QMediaPlaylist::Sequential);
		}
	});

	QLabel *lblNum = new QLabel(view);
	lblNum->hide();

	QObject::connect(playlist, &QMediaPlaylist::currentIndexChanged, [player, lblMeta, lblNum, lblError](int x)
	{
		if (x != -1)
		{
			lblMeta->setText("");
			lblError->hide();
		}

		lblNum->setText(QString("%1/%2").arg(x + 1).arg(player->playlist()->mediaCount()));
#endif
	});

	QSlider *sldrPos = new QSlider(Qt::Horizontal, view);
	sldrPos->setFocusPolicy(Qt::NoFocus);
	sldrPos->setRange(0, player->duration() / 1000);

	QObject::connect(sldrPos, &QSlider::actionTriggered, [player, sldrPos](int action)
	{
		player->setPosition(sldrPos->sliderPosition() * 1000);
	});

	QObject::connect(player, &QMediaPlayer::durationChanged, [sldrPos](qint64 x)
	{
		sldrPos->setMaximum(x / 1000);
		sldrPos->setProperty("duration", x);
	});

	QObject::connect(player, &QMediaPlayer::mediaStatusChanged, [player]()
	{
		//qDebug() << "mediaStatusChanged: " << player->mediaStatus();
	});

	QObject::connect(player, &QMediaPlayer::seekableChanged, [sldrPos](bool seekable)
	{
		sldrPos->setEnabled(seekable);
	});

	main->addWidget(sldrPos);
	QHBoxLayout *controls = new QHBoxLayout;

	QPushButton *btnPlay = new QPushButton(view);
	btnPlay->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));

	QObject::connect(btnPlay, &QPushButton::clicked, [btnPlay, player]()
	{
		setPlayerPlayPause(player, btnPlay);
	});

#if QT_VERSION >= 0x060000
	QObject::connect(player, &QMediaPlayer::playbackStateChanged, [btnPlay](QMediaPlayer::PlaybackState newState)
#else
	QObject::connect(player, &QMediaPlayer::stateChanged, [btnPlay](QMediaPlayer::State newState)
#endif
	{
		if (newState != QMediaPlayer::PlayingState)
			btnPlay->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
		else
			btnPlay->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPause));
	});

	btnPlay->setFocusPolicy(Qt::NoFocus);

	QPushButton *btnStop = new QPushButton(view);
	btnStop->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaStop));
	//QObject::connect(btnStop, &QPushButton::clicked, player, &QMediaPlayer::stop);

	QObject::connect(btnStop, &QPushButton::clicked, [view, player]()
	{
		player->stop();

		// genius_meme.jpg
		QString file = view->property("file").value<QString>();
		QString type = view->property("type").value<QString>();
		setPlayerMedia(player, (char*)file.toStdString().c_str(), type);
	});

	btnStop->setFocusPolicy(Qt::NoFocus);

	QPushButton *btnInfo = new QPushButton(view);
	btnInfo->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));
	btnInfo->setEnabled(false);
	btnInfo->setFocusPolicy(Qt::NoFocus);

	QLabel *lblTime = new QLabel(DURATION_EMPTY, view);

	QObject::connect(player, &QMediaPlayer::positionChanged, [sldrPos, lblTime](qint64 x)
	{
		sldrPos->setValue(x / 1000);
		qint64 duration = sldrPos->property("duration").value<qint64>();
		lblTime->setText(QString::asprintf("%" MY_TIME_FORMAT "/%" MY_TIME_FORMAT, MY_TIME_ARGS(x), MY_TIME_ARGS(duration)));
	});

	QPushButton *btnLoop = new QPushButton(view);
	btnLoop->setIcon(QIcon::fromTheme("media-playlist-repeat"));
	setPlayerLoop(player, btnLoop);
	btnLoop->setFocusPolicy(Qt::NoFocus);

	QObject::connect(btnLoop, &QPushButton::clicked, [btnLoop, player]()
	{
		gLoop = !gLoop;
		setPlayerLoop(player, btnLoop);
	});

#if QT_VERSION >= 0x060000
	QObject::connect(player, &QMediaPlayer::hasVideoChanged, [lblMeta, btnInfo, lblThumb, scroll, video](bool videoAvailable)
#else
	QObject::connect(player, &QMediaPlayer::videoAvailableChanged, [lblMeta, btnInfo, lblThumb, scroll, video](bool videoAvailable)
#endif
	{
		lblMeta->setVisible(!videoAvailable);
		lblThumb->setVisible(!videoAvailable);
		scroll->setVisible(!videoAvailable);
		btnInfo->setEnabled(videoAvailable);
		video->setVisible(videoAvailable);
	});

	QObject::connect(btnInfo, &QPushButton::clicked, [btnInfo, lblMeta, scroll, video]()
	{
		bool isMeta = lblMeta->isVisible();
		btnInfo->setDown(!isMeta);
		scroll->setVisible(!isMeta);
		lblMeta->setVisible(!isMeta);
		video->setVisible(isMeta);
	});

#if QT_VERSION >= 0x060000
	QObject::connect(player, &QMediaPlayer::metaDataChanged, [player, lblMeta, lblThumb]()
	{
		QString info, value;

		QImage thumb = player->metaData()[QMediaMetaData::ThumbnailImage].value<QImage>();

		if (thumb.isNull())
			thumb = player->metaData()[QMediaMetaData::CoverArtImage].value<QImage>();

		if (!thumb.isNull())
		{
			lblThumb->setPixmap(QPixmap::fromImage(thumb).scaled(256, 256, Qt::KeepAspectRatio));
			lblThumb->show();
		}
		else
			lblThumb->hide();

		QMapIterator<const char*, QMediaMetaData::Key> i(gMetaFields);

		while (i.hasNext())
		{
			i.next();
			QVariant val = player->metaData().value(i.value());

			if (i.value() == QMediaMetaData::VideoBitRate || i.value() == QMediaMetaData::AudioBitRate)
				value = QString::asprintf(_("%'d bps"), val.toInt());
			else if (i.value() == QMediaMetaData::VideoFrameRate)
				value = QString::asprintf("%d", val.toInt());
			else
				value = player->metaData().stringValue(i.value());

			if (!value.isEmpty() && !val.isNull() && val != 0)
			{
				info.append("<b>");
				info.append(QString(i.key()).toHtmlEscaped());
				info.append("</b>");
				info.append(": ");
				info.append(value.toHtmlEscaped());
				info.append("<br>");
			}
		}

		if (info.isEmpty())
		{
			info.append("<b>");
			info.append(QString(_("no suitable info available")).toHtmlEscaped());
			info.append("<br>");
		}

		lblMeta->setText(info);
	});
#else
	QObject::connect(player, QOverload<>::of(&QMediaObject::metaDataChanged), [video, player, lblMeta, lblThumb]()
	{
		QString info;

		QImage thumb = player->metaData(QMediaMetaData::ThumbnailImage).value<QImage>();

		if (thumb.isNull())
			thumb = player->metaData(QMediaMetaData::CoverArtImage).value<QImage>();

		if (!thumb.isNull())
		{
			lblThumb->setPixmap(QPixmap::fromImage(thumb).scaled(256, 256, Qt::KeepAspectRatio));
			lblThumb->show();
		}
		else
			lblThumb->hide();

		QMapIterator<const char*, QString> i(gMetaFields);

		while (i.hasNext())
		{
			i.next();

			QVariant val = player->metaData(i.value());

			if (val.isValid())
			{
				info.append("<b>");
				info.append(QString(i.key()).toHtmlEscaped());
				info.append("</b>");
				info.append(": ");

				if (i.value() == QMediaMetaData::VideoBitRate || i.value() == QMediaMetaData::AudioBitRate)
					info.append(QString::asprintf(_("%'d bps"), val.toInt()));
				else if (val.canConvert(QMetaType::QString))
					info.append(val.toString());
				else if (val.canConvert(QMetaType::QSize))
				{
					QSize size = val.toSize();
					info.append(QString("%1x%2").arg(size.width()).arg(size.height()));
				}

				info.append("<br>");
			}
		}

		if (info.isEmpty())
		{
			info.append("<b>");
			info.append(QString(_("no suitable info available")).toHtmlEscaped());
			info.append("</b>");
		}

		lblMeta->setText(info);
	});

	QPushButton *btnPrev = new QPushButton(view);
	btnPrev->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowBack));
	QObject::connect(btnPrev, &QPushButton::clicked, playlist, &QMediaPlaylist::previous);
	btnPrev->setFocusPolicy(Qt::NoFocus);
	btnPrev->hide();

	QPushButton *btnNext = new QPushButton(view);
	btnNext->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowForward));
	QObject::connect(btnNext, &QPushButton::clicked, playlist, &QMediaPlaylist::next);
	btnNext->setFocusPolicy(Qt::NoFocus);
	btnNext->hide();

	QObject::connect(playlist, &QMediaPlaylist::loaded, [btnNext, btnPrev, lblNum]()
	{
		btnPrev->show();
		btnNext->show();
		lblNum->show();
	});

	QObject::connect(playlist, &QMediaPlaylist::mediaRemoved, [btnNext, btnPrev, lblNum](int start, int end)
	{
		btnPrev->hide();
		btnNext->hide();
		lblNum->hide();
	});
#endif
	QPushButton *btnMute = new QPushButton(view);
	btnMute->setFocusPolicy(Qt::NoFocus);
	setPlayerVolume(player, btnMute);

	QObject::connect(btnMute, &QPushButton::clicked, [btnMute, player]()
	{
#if QT_VERSION >= 0x060000
		gMute = !player->audioOutput()->isMuted();
#else
		gMute = !player->isMuted();
#endif
		setPlayerVolume(player, btnMute);
	});

	QSlider *sldrVolume = new QSlider(Qt::Horizontal, view);
	sldrVolume->setRange(1, 100);
	sldrVolume->setFocusPolicy(Qt::NoFocus);

	QObject::connect(sldrVolume, &QSlider::valueChanged, [btnMute, sldrVolume, player]()
	{
		gVolume = sldrVolume->value();
		setPlayerVolume(player, btnMute);
	});

	sldrVolume->setValue(gVolume);

	QShortcut *hkInfo = new QShortcut(QKeySequence("i"), view);
	QObject::connect(hkInfo, &QShortcut::activated, [btnInfo]()
	{
		emit btnInfo->click();
	});

	QShortcut *hkLoop = new QShortcut(QKeySequence("r"), view);
	QObject::connect(hkLoop, &QShortcut::activated, [btnLoop]()
	{
		emit btnLoop->click();
	});

	QShortcut *hkMute = new QShortcut(QKeySequence("m"), view);
	QObject::connect(hkMute, &QShortcut::activated, [btnMute]()
	{
		emit btnMute->click();
	});

	QShortcut *hkVolumeUp = new QShortcut(QKeySequence("Up"), view);
	QObject::connect(hkVolumeUp, &QShortcut::activated, [sldrVolume]()
	{
		sldrVolume->setValue(sldrVolume->value() + 5);
	});

	QShortcut *hkVolumeDown = new QShortcut(QKeySequence("Down"), view);
	QObject::connect(hkVolumeDown, &QShortcut::activated, [sldrVolume]()
	{
		sldrVolume->setValue(sldrVolume->value() - 5);
	});

	QShortcut *hkPosLeft = new QShortcut(QKeySequence("Left"), view);
	QObject::connect(hkPosLeft, &QShortcut::activated, [player, sldrPos]()
	{
		player->pause();
		player->setPosition((sldrPos->sliderPosition() - 5) * 1000);
		player->play();
	});

	QShortcut *hkPosRight = new QShortcut(QKeySequence("Right"), view);
	QObject::connect(hkPosRight, &QShortcut::activated, [player, sldrPos]()
	{
		player->pause();
		player->setPosition((sldrPos->sliderPosition() + 5) * 1000);
		player->play();
	});

	QShortcut *hkPlay = new QShortcut(QKeySequence("Space"), view);
	QObject::connect(hkPlay, &QShortcut::activated, [player, btnPlay]()
	{
		setPlayerPlayPause(player, btnPlay);
	});

	controls->addWidget(btnPlay);
	controls->addWidget(btnStop);
	controls->addSpacing(10);
	controls->addWidget(btnLoop);
	controls->addSpacing(5);
	controls->addWidget(lblTime);
	controls->addSpacing(5);
	controls->addWidget(btnInfo);
#if QT_VERSION < 0x060000
	controls->addSpacing(10);
	controls->addWidget(btnPrev);
	controls->addWidget(btnNext);
	controls->addSpacing(10);
	controls->addWidget(lblNum);
#endif
	controls->addStretch(1);
	controls->addWidget(btnMute);
	controls->addWidget(sldrVolume);
	main->addLayout(controls);

	setPlayerMedia(player, FileToLoad, type.name());

	varPlayer.setValue(player);
	view->setProperty("player", varPlayer);
	view->setProperty("file", QString(FileToLoad));
	view->setProperty("type", type.name());
	view->show();

	player->play();

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QFrame *view = (QFrame*)PluginWin;
	QMediaPlayer *player = view->property("player").value<QMediaPlayer *>();

	QMimeType type = gDB.mimeTypeForFile(QString(FileToLoad));

	if (type.name().contains("audio") || type.name().contains("video"))
	{
#if QT_VERSION >= 0x060000
		if (type.name() == "audio/x-mpegurl")
			return LISTPLUGIN_OK;
#endif

		setPlayerMedia(player, FileToLoad, type.name());

		view->setProperty("file", QString(FileToLoad));
		view->setProperty("type", type.name());

		player->play();
		return LISTPLUGIN_OK;
	}

	return LISTPLUGIN_ERROR;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QFrame *view = (QFrame*)ListWin;
	QMediaPlayer *player = view->property("player").value<QMediaPlayer *>();

	player->stop();

	if (gSaveSettings)
	{
		QSettings settings(gCfgPath, QSettings::IniFormat);
		settings.setValue(PLUGNAME "/volume", gVolume);
		settings.setValue(PLUGNAME "/mute", gMute);
		settings.setValue(PLUGNAME "/loop", gLoop);
	}

	delete player;
	delete view;
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

	if (!settings.contains(PLUGNAME "/save_on_exit"))
		settings.setValue(PLUGNAME "/save_on_exit", gSaveSettings);
	else
		gSaveSettings = settings.value(PLUGNAME "/save_on_exit").toBool();

	if (!settings.contains(PLUGNAME "/info_fontsize"))
		settings.setValue(PLUGNAME "/info_fontsize", gFontSize);
	else
		gFontSize = settings.value(PLUGNAME "/info_fontsize").toInt();
}
