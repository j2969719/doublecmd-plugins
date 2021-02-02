#include <QUrl>
#include <QProcess>
#include <QWebView>
#include <QSettings>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QTemporaryDir>

#include <dlfcn.h>
#include "wlxplugin.h"

static char inipath[PATH_MAX];

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QProcess proc;
	QMimeDatabase db;
	QString cmd, key;

	QString filename = QString(FileToLoad);

	QFileInfo fi(filename);

	if (!fi.exists())
		return nullptr;

	QSettings settings(inipath, QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	key = QString("%1/command").arg(fi.suffix().toLower());

	if (settings.contains(key))
		cmd = QString::fromUtf8(settings.value(key).toByteArray());
	else
	{
		QMimeType type = db.mimeTypeForFile(filename);
		key = QString("%1/command").arg(type.name());

		if (settings.contains(key))
			cmd = QString::fromUtf8(settings.value(key).toByteArray());
	}

	if (cmd.isEmpty())
		return nullptr;

	QTemporaryDir tmpdir("/tmp/_dc-webkit.XXXXXX");

	QString out = QString("%1/output.html").arg(tmpdir.path());

	key.replace("/command", "/noautoremove");

	if (settings.value(key).toBool())
		tmpdir.setAutoRemove(false);


	cmd.replace("$FILE", filename.replace(" ", "\\ "));
	cmd.replace("$HTML", out);

	QStringList params;
	params << "-c" << cmd;

	proc.start("/bin/sh", params);
	proc.waitForFinished();

	if (proc.exitStatus() != QProcess::NormalExit)
		return nullptr;

	QFileInfo fiout(out);

	if (!fiout.exists())
		return nullptr;

	QWebView *webView = new QWebView((QWidget*)ParentWin);

	webView->setStyleSheet("background-color:white;");

	webView->load(QUrl::fromLocalFile(out));
	webView->show();
	return webView;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QWebView *webView = (QWebView*)ListWin;
	webView->~QWebView();
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	QWebPage::FindFlags sflags;

	if (SearchParameter & lcs_matchcase)
		sflags |= QWebPage::FindCaseSensitively;

	if (SearchParameter & lcs_backwards)
		sflags |= QWebPage::FindBackward;

	QWebView *webView = (QWebView*)ListWin;
	webView->findText(SearchString, sflags);
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
	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(inipath, &dlinfo) != 0)
	{
		QFileInfo plugnfo(QString(dlinfo.dli_fname));
		snprintf(inipath, PATH_MAX, "%s/settings.ini", plugnfo.absolutePath().toStdString().c_str());
	}
}
