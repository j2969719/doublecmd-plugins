#include <QProcess>
#include <QTextBrowser>
#include <QSettings>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QTemporaryDir>

#include <QMessageBox>

#include <dlfcn.h>
#include <libintl.h>
#include <locale.h>
#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

#include "wlxplugin.h"

QString path;

QString convert(char* FileToLoad, QString tmpdir)
{
	QProcess proc;
	QString output = tmpdir + "/output.html";
	QString exec = "./exsimple";
	QStringList params;
	params << FileToLoad << output << path + "/redist/default.cfg";
	proc.setWorkingDirectory(path + "/redist");
	proc.start(exec, params);
	proc.waitForFinished();
	proc.close();
	return output;
}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QTemporaryDir tmpdir;
	tmpdir.setAutoRemove(false);
	QString output = convert(FileToLoad, tmpdir.path());
	QFileInfo html(output);

	if (!html.exists())
	{
		tmpdir.remove();
		return nullptr;
	}

	QFile file(output);

	if (!file.open(QFile::ReadOnly | QFile::Text))
		return nullptr;

	QTextBrowser *view = new QTextBrowser((QWidget*)ParentWin);

	view->setHtml(file.readAll());
	file.close();
	view->show();

	return view;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	delete (QTextBrowser*)ListWin;
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
	static char inipath[PATH_MAX];
	strncpy(inipath, dps->DefaultIniName, PATH_MAX);

	Dl_info dlinfo;
	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(inipath, &dlinfo) != 0)
	{
		QString scrpath(dlinfo.dli_fname);
		QFileInfo script(scrpath);
		path = script.absolutePath();
	}
}
