#include <QDebug>
#include <QSettings>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QProcessEnvironment>

#include <dlfcn.h>
#include <qtermwidget.h>
#include "wlxplugin.h"

static char inipath[PATH_MAX];

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QMimeDatabase db;
	QString cmd, key;

	QString filename = QString(FileToLoad);

	QFileInfo fi(filename);

	if (!fi.exists())
		return nullptr;

	QSettings settings(inipath, QSettings::IniFormat);
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

	QString quoted = filename.replace("\'", "\\\'");
	quoted.prepend("\'");
	quoted.append("\'");
	cmd.replace("$FILE", quoted);

	QStringList params;
	params << "-c" << cmd;

	QTermWidget *view = new QTermWidget(0, (QWidget*)ParentWin);
	QProcessEnvironment env;
	env.insert("TERM", "screen-256color");
	view->setEnvironment(env.toStringList());
	view->setShellProgram("/bin/sh");
	view->setArgs(params);

	if (settings.contains("qtermwidget/scheme"))
		view->setColorScheme(settings.value("qtermwidget/scheme").toString());
	else
	{
		qDebug() << "availableColorSchemes" << view->availableColorSchemes();
		view->setColorScheme("WhiteOnBlack");
	}

	if (settings.contains("qtermwidget/font"))
	{
		QFont font;
		font.setFamily(settings.value("qtermwidget/font").toString());

		if (settings.contains("qtermwidget/fontsize"))
			font.setPointSize(settings.value("qtermwidget/fontsize").toInt());

		view->setTerminalFont(font);
	}

	view->setAutoClose(false);
	view->startShellProgram();
	view->show();

	return view;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	delete (QTermWidget*)ListWin;
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	QTermWidget *view = (QTermWidget*)ListWin;
	view->toggleShowSearchBar();
	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	QTermWidget *view = (QTermWidget*)ListWin;

	switch (Command)
	{
	case lc_copy :
		view->copyClipboard();
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
