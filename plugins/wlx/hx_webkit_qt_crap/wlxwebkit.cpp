#include <QDir>
#include <QUrl>
#include <QProcess>
#include <QFileInfo>
#include <QWebView>
#include <QTemporaryDir>

#include <dlfcn.h>
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
		return NULL;
	}

	QWebView *webView = new QWebView((QWidget*)ParentWin);
	webView->load(QUrl::fromLocalFile(output));
	webView->setProperty("tempdir", tmpdir.path());
	webView->show();
	return webView;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QWebView *webView = (QWebView *)ListWin;
	QDir tmpdir(webView->property("tempdir").toString());
	tmpdir.removeRecursively();
	delete webView;
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	QWebPage::FindFlags sflags;

	if (SearchParameter & lcs_matchcase)
		sflags |= QWebPage::FindCaseSensitively;

	if (SearchParameter & lcs_backwards)
		sflags |= QWebPage::FindBackward;

	QWebView *webView = (QWebView *)ListWin;
	webView->findText(SearchString, sflags);
	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	QWebView *webView = (QWebView *)ListWin;

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
	static char inipath[PATH_MAX + 1];
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
