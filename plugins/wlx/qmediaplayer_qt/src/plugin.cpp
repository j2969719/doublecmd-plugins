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

QMimeDatabase gDB;
int gVolume = 80;
bool gMute = false;
bool gLoop = true;
bool gSaveSettings = true;
QString gCfgPath;

static void btnMuteUpdate(QPushButton *btnMute)
{
	if (!gMute)
		btnMute->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaVolume));
	else
		btnMute->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaVolumeMuted));
}

static void btnLoopUpdate(QPushButton *btnLoop)
{
	if (!gLoop)
		btnLoop->setText("once");
	else
		btnLoop->setText("loop");
}

#if QT_VERSION >= 0x060000
HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QVariant varPlayer;
	QMimeType type = gDB.mimeTypeForFile(QString(FileToLoad));

	if (!type.name().contains("audio") && !type.name().contains("video"))
		return nullptr;

	QFrame *view = new QFrame((QWidget*)ParentWin);

	QMediaPlayer *player = new QMediaPlayer((QObject*)view);

	QAudioOutput *audio = new QAudioOutput((QObject*)view);
	player->setAudioOutput(audio);

	QPushButton *btnLoop = new QPushButton(view);
	btnLoopUpdate(btnLoop);

	QObject::connect(btnLoop, &QPushButton::clicked, [btnLoop, player]()
	{
		gLoop = !gLoop;
		player->setLoops(gLoop ? QMediaPlayer::Infinite : QMediaPlayer::Once);
		btnLoopUpdate(btnLoop);
	});

	player->setLoops(gLoop ? QMediaPlayer::Infinite : QMediaPlayer::Once);

	QVBoxLayout *main = new QVBoxLayout(view);

	QVideoWidget *video = new QVideoWidget(view);
	video->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	player->setVideoOutput(video);
	video->hide();
	main->addWidget(video);

	QLabel *lblThumb = new QLabel(view);
	lblThumb->setAlignment(Qt::AlignCenter);
	lblThumb->hide();
	main->addWidget(lblThumb);

	QLabel *lblMeta = new QLabel(view);
	lblMeta->setAlignment(Qt::AlignCenter);
	lblMeta->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	lblMeta->setTextInteractionFlags(Qt::TextSelectableByMouse);
	main->addWidget(lblMeta);

	QLabel *lblError = new QLabel(view);
	lblError->setAlignment(Qt::AlignCenter);
	lblError->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	lblError->hide();
	main->addWidget(lblError);

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
	});

	QSlider *sldrPos = new QSlider(Qt::Horizontal, view);
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

	QObject::connect(player, &QMediaPlayer::seekableChanged, [sldrPos](bool seekable)
	{
		sldrPos->setEnabled(seekable);
	});

	QLabel *lblTime = new QLabel(DURATION_EMPTY, view);

	QObject::connect(player, &QMediaPlayer::positionChanged, [sldrPos, lblTime](qint64 x)
	{
		sldrPos->setValue(x / 1000);
		qint64 duration = sldrPos->property("duration").value<qint64>();
		lblTime->setText(QString::asprintf("%" MY_TIME_FORMAT "/%" MY_TIME_FORMAT, MY_TIME_ARGS(x), MY_TIME_ARGS(duration)));
	});

	main->addWidget(sldrPos);

	QHBoxLayout *controls = new QHBoxLayout;

	QPushButton *btnPlay = new QPushButton(view);
	btnPlay->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
	QObject::connect(btnPlay, SIGNAL(clicked()), player, SLOT(play()));

	QPushButton *btnPause = new QPushButton(view);
	btnPause->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPause));
	QObject::connect(btnPause, SIGNAL(clicked()), player, SLOT(pause()));

	QPushButton *btnInfo = new QPushButton(view);
	btnInfo->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));
	btnInfo->setFocusPolicy(Qt::NoFocus);
	btnInfo->setEnabled(false);

	QObject::connect(player, &QMediaPlayer::hasVideoChanged, [lblMeta, btnInfo, lblThumb, video](bool videoAvailable)
	{
		lblMeta->setVisible(!videoAvailable);
		btnInfo->setEnabled(videoAvailable);
		video->setVisible(videoAvailable);
		lblThumb->hide();
	});

	QObject::connect(btnInfo, &QPushButton::clicked, [player, lblMeta, video]()
	{
		bool isMeta = lblMeta->isVisible();
		lblMeta->setVisible(!isMeta);
		video->setVisible(isMeta);
	});

	QObject::connect(player, &QMediaPlayer::metaDataChanged, [player, lblMeta, lblThumb]()
	{
		QString info;
		QList<QMediaMetaData::Key> keys = player->metaData().keys();
		lblThumb->hide();

		for (qsizetype i = 0; i < keys.size(); ++i)
		{
			QString str, val;

			if (keys.at(i) == QMediaMetaData::VideoBitRate || keys.at(i) == QMediaMetaData::AudioBitRate)
				val = QString::asprintf("%'d", player->metaData().value(keys.at(i)).toInt());
			else if (keys.at(i) == QMediaMetaData::ThumbnailImage)
			{
				QImage thumb = player->metaData()[keys.at(i)].value<QImage>();
				lblThumb->setPixmap(QPixmap::fromImage(thumb).scaled(256, 256, Qt::KeepAspectRatio));
				lblThumb->show();
			}
			else
			{
				val = player->metaData().stringValue(keys.at(i));
			}

			if (!val.isEmpty() && player->metaData().value(keys.at(i)) != 0)
			{
				str.append(player->metaData().metaDataKeyToString(keys.at(i)).replace(QRegularExpression("([a-z])([A-Z])"), "\\1 \\2"));
				str.append(": ");
				str.append(val);
				str.append("\n");
				info.append(str);
			}
		}

		if (!info.isEmpty())
			lblMeta->setText(info);
		else
			lblMeta->setText("no suitable info available");
	});

	QPushButton *btnMute = new QPushButton(view);
	btnMute->setFocusPolicy(Qt::NoFocus);
	audio->setMuted(gMute);
	btnMuteUpdate(btnMute);

	QObject::connect(btnMute, &QPushButton::clicked, [btnMute, audio]()
	{
		gMute = !audio->isMuted();
		audio->setMuted(gMute);
		btnMuteUpdate(btnMute);
	});

	QSlider *sldrVolume = new QSlider(Qt::Horizontal, view);
	sldrVolume->setRange(1, 100);

	QObject::connect(sldrVolume, &QSlider::valueChanged, [sldrVolume, audio]()
	{
		gVolume = sldrVolume->value();
		audio->setVolume(QAudio::convertVolume(gVolume / qreal(100), QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale));
	});

	sldrVolume->setValue(gVolume);

	controls->addWidget(btnPlay);
	controls->addWidget(btnPause);
	controls->addSpacing(10);
	controls->addWidget(btnInfo);
	controls->addSpacing(5);
	controls->addWidget(lblTime);
	controls->addSpacing(5);
	controls->addWidget(btnLoop);
	controls->addStretch(1);
	controls->addWidget(btnMute);
	controls->addWidget(sldrVolume);
	main->addLayout(controls);

	varPlayer.setValue(player);
	view->setProperty("player", varPlayer);
	view->show();
	player->setSource(QUrl::fromLocalFile(FileToLoad));
	player->play();

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QWidget *view = (QWidget*)PluginWin;
	QMediaPlayer *player = view->property("player").value<QMediaPlayer *>();

	QMimeType type = gDB.mimeTypeForFile(QString(FileToLoad));

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
	QVariant varPlayer;
	QMimeType type = gDB.mimeTypeForFile(QString(FileToLoad));

	if (!type.name().contains("audio") && !type.name().contains("video"))
		return nullptr;

	QFrame *view = new QFrame((QWidget*)ParentWin);
	view->setFrameStyle(QFrame::NoFrame);

	QMediaPlayer *player = new QMediaPlayer((QObject*)view);
	QMediaPlaylist *playlist = new QMediaPlaylist((QObject*)player);

	player->setPlaylist(playlist);

	QPushButton *btnLoop = new QPushButton(view);
	btnLoopUpdate(btnLoop);

	QObject::connect(btnLoop, &QPushButton::clicked, [btnLoop, player, playlist]()
	{
		gLoop = !gLoop;
		playlist->setPlaybackMode(gLoop ? QMediaPlaylist::Loop : QMediaPlaylist::Sequential);
		btnLoopUpdate(btnLoop);
	});

	playlist->setPlaybackMode(gLoop ? QMediaPlaylist::Loop : QMediaPlaylist::Sequential);

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

	QLabel *lblMeta = new QLabel(view);
	lblMeta->setAlignment(Qt::AlignCenter);
	lblMeta->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	lblMeta->setTextInteractionFlags(Qt::TextSelectableByMouse);
	lblMeta->hide();
	main->addWidget(lblMeta);

	QLabel *lblError = new QLabel(view);
	lblError->setAlignment(Qt::AlignCenter);
	lblError->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	lblError->hide();
	main->addWidget(lblError);

	QLabel *lblInfo = new QLabel(view);
	lblInfo->setAlignment(Qt::AlignCenter);
	lblInfo->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	main->addWidget(lblInfo);

	QObject::connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), [view, player, lblError, video](QMediaPlayer::Error error)
	{
		player->playlist()->setPlaybackMode(QMediaPlaylist::CurrentItemOnce);
		player->stop();
		lblError->setText(QString("ERROR: %1").arg(player->errorString()));
		lblError->show();
	});

	QObject::connect(player, &QMediaPlayer::currentMediaChanged, [lblMeta, lblError, video](const QMediaContent & media)
	{
		lblError->hide();
		lblMeta->hide();
		video->show();
	});

	QLabel *lblNum = new QLabel(view);
	lblNum->hide();

	QObject::connect(playlist, &QMediaPlaylist::currentIndexChanged, [player, lblInfo, lblNum, lblError](int x)
	{
		lblInfo->setText("");
		lblNum->setText(QString("%1/%2").arg(x + 1).arg(player->playlist()->mediaCount()));
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

	QObject::connect(player, &QMediaPlayer::seekableChanged, [sldrPos](bool seekable)
	{
		sldrPos->setEnabled(seekable);
	});

	QLabel *lblTime = new QLabel(DURATION_EMPTY, view);

	QObject::connect(player, &QMediaPlayer::positionChanged, [sldrPos, lblTime](qint64 x)
	{
		sldrPos->setValue(x / 1000);
		qint64 duration = sldrPos->property("duration").value<qint64>();
		lblTime->setText(QString::asprintf("%" MY_TIME_FORMAT "/%" MY_TIME_FORMAT, MY_TIME_ARGS(x), MY_TIME_ARGS(duration)));
	});

	main->addWidget(sldrPos);

	QHBoxLayout *controls = new QHBoxLayout;

	QPushButton *btnPlay = new QPushButton(view);
	btnPlay->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
	QObject::connect(btnPlay, &QPushButton::clicked, player, &QMediaPlayer::play);
	btnPlay->setFocusPolicy(Qt::NoFocus);

	QPushButton *btnPause = new QPushButton(view);
	btnPause->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPause));
	QObject::connect(btnPause, &QPushButton::clicked, player, &QMediaPlayer::pause);
	btnPause->setFocusPolicy(Qt::NoFocus);

	QPushButton *btnInfo = new QPushButton(view);
	btnInfo->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));
	btnInfo->setFocusPolicy(Qt::NoFocus);

	QObject::connect(btnInfo, &QPushButton::clicked, [playlist, lblMeta, video]()
	{
		bool isMeta = lblMeta->isVisible();

		if (!isMeta)
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
				lblMeta->setText(info);
			else
				lblMeta->setText("no suitable info available");
		}

		lblMeta->setVisible(!isMeta);
		video->setVisible(isMeta);
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

	QObject::connect(playlist, &QMediaPlaylist::mediaRemoved, [lblInfo, player, btnNext, btnPrev, lblNum, lblMeta, video](int start, int end)
	{
		btnPrev->hide();
		btnNext->hide();
		lblNum->hide();
		lblInfo->setText("");
	});

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	QObject::connect(player, QOverload<const QString &, const QVariant &>::of(&QMediaObject::metaDataChanged), [video, lblInfo, playlist, lblMeta](const QString & key, const QVariant & value)
	{
		if (key == "CoverArtImage")
		{
			QImage image = value.value<QImage>().convertToFormat(QImage::Format_ARGB32);
			QVideoSurfaceFormat format(image.size(), QVideoFrame::Format_ARGB32);
			video->videoSurface()->start(format);
			video->videoSurface()->present(image);
		}
		else if (key == "ContributingArtist")
			lblInfo->setText(value.value<QString>());
		else if (key == "Title")
		{
			QString text = lblInfo->text();

			if (text.length() > 256 || text.contains("\t—\t"))
				text.clear();

			if (!text.isEmpty())
				text.append("\t—\t");

			text.append(value.value<QString>());
			lblInfo->setText(text);
		}
	});
#endif

	QPushButton *btnMute = new QPushButton(view);
	btnMute->setFocusPolicy(Qt::NoFocus);
	player->setMuted(gMute);
	btnMuteUpdate(btnMute);

	QObject::connect(btnMute, &QPushButton::clicked, [btnMute, player]()
	{
		gMute = !player->isMuted();
		player->setMuted(gMute);
		btnMuteUpdate(btnMute);
	});

	QSlider *sldrVolume = new QSlider(Qt::Horizontal, view);
	sldrVolume->setRange(1, 100);
	sldrVolume->setValue(gVolume);
	player->setVolume(gVolume);

	QObject::connect(sldrVolume, &QSlider::valueChanged, [sldrVolume, player]()
	{
		gVolume = sldrVolume->value();
		player->setVolume(gVolume);
	});

	sldrVolume->setFocusPolicy(Qt::NoFocus);

	controls->addWidget(btnPlay);
	controls->addWidget(btnPause);
	controls->addSpacing(10);
	controls->addWidget(btnInfo);
	controls->addSpacing(5);
	controls->addWidget(lblTime);
	controls->addSpacing(5);
	controls->addWidget(btnLoop);
	controls->addSpacing(10);
	controls->addWidget(btnPrev);
	controls->addWidget(btnNext);
	controls->addSpacing(10);
	controls->addWidget(lblNum);
	controls->addStretch(1);
	controls->addWidget(btnMute);
	controls->addWidget(sldrVolume);
	main->addLayout(controls);

	if (type.name() == "audio/x-mpegurl")
		playlist->load(QUrl::fromLocalFile(FileToLoad));
	else
		playlist->addMedia(QUrl::fromLocalFile(FileToLoad));

	varPlayer.setValue(player);
	view->setProperty("player", varPlayer);
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
}
