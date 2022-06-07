#include <QUrl>
#include <QWebView>
#include <QtWidgets>
#include <QMessageBox>

#include <dlfcn.h>
#include <libintl.h>
#include <locale.h>

#include "wlxplugin.h"

#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"
#define DETECT_STRING "(EXT=\"HTML\")|(EXT=\"HTM\")|(EXT=\"XHTM\")|(EXT=\"XHTML\")"

static bool gControls = true;

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QVariant var;
	QFrame *view = new QFrame((QWidget*)ParentWin);
	QFrame *frmControls = new QFrame(view);
	view->setFrameStyle(QFrame::NoFrame);
	frmControls->setFrameStyle(QFrame::NoFrame);

	QVBoxLayout *main = new QVBoxLayout(view);

	QHBoxLayout *controls = new QHBoxLayout(frmControls);
	QWebView *webView = new QWebView(view);
	main->setSpacing(0)

	QPushButton *btnPrev = new QPushButton(view);
	btnPrev->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowBack));
	QObject::connect(btnPrev, &QPushButton::clicked, webView, &QWebView::back);
	btnPrev->setFocusPolicy(Qt::NoFocus);
	controls->addWidget(btnPrev);

	QPushButton *btnNext = new QPushButton(view);
	btnNext->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowForward));
	QObject::connect(btnNext, &QPushButton::clicked, webView, &QWebView::forward);
	btnNext->setFocusPolicy(Qt::NoFocus);
	controls->addWidget(btnNext);

	QPushButton *btnReload = new QPushButton(view);
	btnReload->setIcon(QApplication::style()->standardIcon(QStyle::SP_BrowserReload));
	QObject::connect(btnReload, &QPushButton::clicked, webView, &QWebView::reload);
	btnReload->setFocusPolicy(Qt::NoFocus);
	controls->addWidget(btnReload);

	QPushButton *btnStop = new QPushButton(view);
	btnStop->setIcon(QApplication::style()->standardIcon(QStyle::SP_BrowserStop));
	QObject::connect(btnStop, &QPushButton::clicked, webView, &QWebView::stop);
	btnStop->setFocusPolicy(Qt::NoFocus);
	controls->addWidget(btnStop);
	controls->addSpacing(10);

	QPushButton *btnZoomIn = new QPushButton(view);
	btnZoomIn->setIcon(QIcon::fromTheme("zoom-in"));
	QObject::connect(btnZoomIn, &QPushButton::clicked, [webView]()
	{
		webView->setZoomFactor(webView->zoomFactor() + 0.05);
	});
	btnZoomIn->setFocusPolicy(Qt::NoFocus);
	controls->addWidget(btnZoomIn);

	QPushButton *btnZoomOut = new QPushButton(view);
	btnZoomOut->setIcon(QIcon::fromTheme("zoom-out"));
	QObject::connect(btnZoomOut, &QPushButton::clicked, [webView]()
	{
		webView->setZoomFactor(webView->zoomFactor() - 0.05);
	});
	btnZoomOut->setFocusPolicy(Qt::NoFocus);
	controls->addWidget(btnZoomOut);

	QPushButton *btnZoomOrg = new QPushButton(view);
	btnZoomOrg->setIcon(QIcon::fromTheme("zoom-original"));
	QObject::connect(btnZoomOrg, &QPushButton::clicked, [webView]()
	{
		webView->setZoomFactor(1);
	});
	btnZoomOrg->setFocusPolicy(Qt::NoFocus);
	controls->addWidget(btnZoomOrg);
	controls->addSpacing(10);

	QLineEdit *edUrl = new QLineEdit(view);
	QObject::connect(webView, &QWebView::urlChanged, [edUrl, webView]()
	{
		edUrl->setText(webView->url().toString());
	});
	QObject::connect(edUrl, &QLineEdit::returnPressed, [edUrl, webView]()
	{
		webView->load(QUrl(edUrl->text()));
	});
	controls->addWidget(edUrl);

	QShortcut *hkZoomIn = new QShortcut(QKeySequence::ZoomIn, webView);
	QObject::connect(hkZoomIn, &QShortcut::activated, [webView]()
	{
		webView->setZoomFactor(webView->zoomFactor() + 0.05);
	});

	QShortcut *hkZoomOut = new QShortcut(QKeySequence::ZoomOut, webView);
	QObject::connect(hkZoomOut, &QShortcut::activated, [webView]()
	{
		webView->setZoomFactor(webView->zoomFactor() - 0.05);
	});

	QShortcut *hkZoomOrg = new QShortcut(QKeySequence("Ctrl+0"), webView);
	QObject::connect(hkZoomOrg, &QShortcut::activated, [webView]()
	{
		webView->setZoomFactor(1);
	});

	webView->setStyleSheet("background-color:white;");
	webView->load(QUrl::fromLocalFile(FileToLoad));

	var.setValue(webView);
	view->setProperty("webView", var);
	view->show();

	if (!gControls)
		frmControls->hide();

	return view;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QFrame *view = (QFrame*)ListWin;
	QWebView *webView = view->property("webView").value<QWebView *>();

	delete webView;
	delete view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QFrame *view = (QFrame*)PluginWin;
	QWebView *webView = view->property("webView").value<QWebView *>();
	webView->load(QUrl::fromLocalFile(FileToLoad));

	return LISTPLUGIN_OK;
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	snprintf(DetectString, maxlen - 1, DETECT_STRING);
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	QFrame *view = (QFrame*)ListWin;
	QWebView *webView = view->property("webView").value<QWebView *>();
	QWebPage::FindFlags sflags;

	if (SearchParameter & lcs_matchcase)
		sflags |= QWebPage::FindCaseSensitively;

	if (SearchParameter & lcs_backwards)
		sflags |= QWebPage::FindBackward;

	if (!webView->findText(SearchString, sflags))
		QMessageBox::information((QWidget*)ListWin, "", QString::asprintf(_("\"%s\" not found!"), SearchString));

	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	QFrame *view = (QFrame*)ListWin;
	QWebView *webView = view->property("webView").value<QWebView *>();

	switch (Command)
	{
	case lc_copy :
		webView->triggerPageAction(QWebPage::Copy);
		break;

	case lc_selectall :
		webView->triggerPageAction(QWebPage::SelectAll);
		break;

	default :
		return LISTPLUGIN_ERROR;
	}

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
	QString cfgpath = defini.absolutePath() + "/j2969719.ini";
	QSettings settings(cfgpath, QSettings::IniFormat);

	if (!settings.contains("wlxwebkit/controls"))
		settings.setValue("wlxwebkit/controls", gControls);
	else
		gControls = settings.value("wlxwebkit/controls").toBool();
}
