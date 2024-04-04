#include <QFile>
#include <QWebView>
#include <md4c-html.h>

#include <QMessageBox>

#include <dlfcn.h>
#include <libintl.h>
#include <locale.h>
#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

#include "wlxplugin.h"

#define _detectstring "EXT=\"MD\""

static void proc_md_cb(const MD_CHAR* text, MD_SIZE size, void *userdata)
{
	QByteArray ba = QByteArray(text, size);
	QString* str = (QString *)userdata;
	str->append(ba);
}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QString html_str;
	QFile file(FileToLoad);

	if (!file.open(QFile::ReadOnly | QFile::Text))
		return nullptr;

	QByteArray text = file.readAll();
	file.close();

	if (md_html(text.data(), text.size(), proc_md_cb, &html_str, MD_DIALECT_GITHUB, MD_HTML_FLAG_SKIP_UTF8_BOM) != 0)
		return nullptr;

	QWebView *webView = new QWebView((QWidget*)ParentWin);
	webView->setStyleSheet("background-color:white;");

	webView->setHtml(html_str);
	webView->show();

	return webView;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	delete (QWebView*)ListWin;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QString html_str;
	QFile file(FileToLoad);

	if (!file.open(QFile::ReadOnly | QFile::Text))
		return LISTPLUGIN_ERROR;

	QByteArray text = file.readAll();
	file.close();

	if (md_html(text.data(), text.size(), proc_md_cb, &html_str, 0, MD_HTML_FLAG_SKIP_UTF8_BOM) != 0)
		return LISTPLUGIN_ERROR;

	QWebView *webView = (QWebView*)PluginWin;
	webView->setHtml(html_str);

	return LISTPLUGIN_OK;
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	strncpy(DetectString, _detectstring, maxlen - 1);
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	QWebPage::FindFlags sflags;

	if (SearchParameter & lcs_matchcase)
		sflags |= QWebPage::FindCaseSensitively;

	if (SearchParameter & lcs_backwards)
		sflags |= QWebPage::FindBackward;

	QWebView *webView = (QWebView*)ListWin;

	if (!webView->findText(SearchString, sflags))
		QMessageBox::information((QWidget*)ListWin, "", QString::asprintf(_("\"%s\" not found!"), SearchString));

	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	QWebView *webView = (QWebView*)ListWin;

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
}
