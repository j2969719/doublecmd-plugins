#include <QtWidgets>
#include <QMessageBox>
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QMimeDatabase>
#include <QMediaPlaylist>
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
		return nullptr;

	QFrame *view = new QFrame((QWidget*)ParentWin);
	view->setFrameStyle(QFrame::NoFrame);

	QMediaPlayer *player = new QMediaPlayer((QObject*)view);
	QMediaPlaylist *playlist = new QMediaPlaylist((QObject*)player);

	playlist->setPlaybackMode(QMediaPlaylist::Loop);
	player->setPlaylist(playlist);

	QObject::connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), [view, player](QMediaPlayer::Error error)
	{
		player->playlist()->setPlaybackMode(QMediaPlaylist::CurrentItemOnce);
		QMessageBox::critical(view, "", player->errorString());
	});

	QVBoxLayout *main = new QVBoxLayout(view);

	QVideoWidget *video = new QVideoWidget(view);
	player->setVideoOutput(video->videoSurface());
	video->setAspectRatioMode(Qt::KeepAspectRatio);
	video->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
	});

	QObject::connect(player, &QMediaPlayer::positionChanged, [spos](qint64 x)
	{
		spos->setValue(x / 1000);
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


	QPushButton *bmute = new QPushButton(view);
	bmute->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaVolume));
	bmute->setFocusPolicy(Qt::NoFocus);

	QObject::connect(bmute, &QPushButton::clicked, [bmute, player]()
	{
		bool state = player->isMuted();
		player->setMuted(!state);

		if (state)
			bmute->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaVolume));
		else
			bmute->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaVolumeMuted));
	});

	QSlider *svolume = new QSlider(Qt::Horizontal, view);
	svolume->setRange(0, 100);
	QObject::connect(svolume, &QSlider::valueChanged, player, &QMediaPlayer::setVolume);
	svolume->setValue(80);
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


void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QFrame *view = (QFrame*)ListWin;
	QMediaPlayer *player = view->property("player").value<QMediaPlayer *>();

	delete player;
	delete view;
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}
