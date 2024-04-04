#include <QFile>
#include <QTextBrowser>
#include <md4c-html.h>

#include <QMessageBox>

#include <dlfcn.h>
#include <libintl.h>
#include <locale.h>
#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

#include "wlxplugin.h"


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

	QTextBrowser *view = new QTextBrowser((QWidget*)ParentWin);

	view->setHtml(html_str);
	view->show();

	return view;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	delete (QTextBrowser*)ListWin;
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

	QTextBrowser *view = (QTextBrowser*)PluginWin;
	view->setHtml(html_str);

	return LISTPLUGIN_OK;
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	snprintf(DetectString, maxlen - 1, "%s", DETECT_STRING);
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	QTextDocument::FindFlags sflags;

	if (SearchParameter & lcs_matchcase)
		sflags |= QTextDocument::FindCaseSensitively;

	if (SearchParameter & lcs_backwards)
		sflags |= QTextDocument::FindBackward;

	QTextBrowser *view = (QTextBrowser*)ListWin;

	if (SearchParameter & lcs_findfirst)
	{
		QTextCursor cursor = view->textCursor();
		cursor.movePosition(SearchParameter & lcs_backwards ? QTextCursor::End : QTextCursor::Start);
		view->setTextCursor(cursor);
	}

	if (view->find(SearchString, sflags))
		return LISTPLUGIN_OK;
	else
		QMessageBox::information(view, "", QString::asprintf(_("\"%s\" not found!"), SearchString));

	return LISTPLUGIN_ERROR;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	QTextBrowser *view = (QTextBrowser*)ListWin;

	switch (Command)
	{
	case lc_copy :
		view->copy();
		break;

	case lc_selectall :
		view->selectAll();
		break;

	case lc_newparams :
		if (Parameter & lcp_wraptext)
			view->setLineWrapMode(QTextBrowser::WidgetWidth);
		else
			view->setLineWrapMode(QTextBrowser::NoWrap);

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
