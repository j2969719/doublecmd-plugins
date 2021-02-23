#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QFontDatabase>

#include <QSettings>

#include <QMessageBox>

#include <libintl.h>
#include <locale.h>
#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

#include <dlfcn.h>
#include "wlxplugin.h"

QString path;
QFont font;

QString getOutput(char* FileToLoad)
{
	QProcess proc;
	QString exec = "./fileinfo.sh";
	QStringList params;
	params << FileToLoad;
	proc.setWorkingDirectory(path);
	proc.start(exec, params);
	proc.waitForFinished();
	QString output(proc.readAllStandardOutput());
	proc.close();
	return output;
}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QString output(getOutput(FileToLoad));

	if (output.isEmpty())
		return NULL;

	QPlainTextEdit *view = new QPlainTextEdit((QWidget*)ParentWin);
	view->document()->setDefaultFont(font);
	view->setPlainText(output);
	view->setReadOnly(true);

	if (ShowFlags & lcp_wraptext)
		view->setLineWrapMode(QPlainTextEdit::WidgetWidth);
	else
		view->setLineWrapMode(QPlainTextEdit::NoWrap);

	view->show();

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QString output(getOutput(FileToLoad));

	if (output.isEmpty())
		return LISTPLUGIN_ERROR;

	QPlainTextEdit *view = (QPlainTextEdit*)PluginWin;
	view->setPlainText(output);
	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	delete (QPlainTextEdit*)ListWin;
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	QTextDocument::FindFlags sflags;

	if (SearchParameter & lcs_matchcase)
		sflags |= QTextDocument::FindCaseSensitively;

	if (SearchParameter & lcs_backwards)
		sflags |= QTextDocument::FindBackward;

	QPlainTextEdit *view = (QPlainTextEdit*)ListWin;

	if (view->find(SearchString, sflags))
		return LISTPLUGIN_OK;
	else
		QMessageBox::information(view, "", QString::asprintf(_("\"%s\" not found!"), SearchString));

	return LISTPLUGIN_ERROR;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	QPlainTextEdit *view = (QPlainTextEdit*)ListWin;

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
			view->setLineWrapMode(QPlainTextEdit::WidgetWidth);
		else
			view->setLineWrapMode(QPlainTextEdit::NoWrap);

		break;

	default :
		return LISTPLUGIN_ERROR;
	}

	return LISTPLUGIN_OK;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	QFileInfo defini(QString::fromStdString(dps->DefaultIniName));
	QString cfgpath = defini.absolutePath() + "/j2969719.ini";
	QSettings settings(cfgpath, QSettings::IniFormat);
	int size = settings.value("fileinfo/fontsize").toInt();

	if (size == 0)
	{
		font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
		size = 12;
		settings.setValue("fileinfo/fontsize", 12);
		settings.setValue("fileinfo/font", font.family());
		settings.setValue("fileinfo/fontbold", font.bold());
	}

	font.setFixedPitch(true);
	font.setPointSize(size);
	font.setFamily(settings.value("fileinfo/font").toString());
	font.setBold(settings.value("fileinfo/fontbold").toBool());

	static char inipath[PATH_MAX + 1];
	strncpy(inipath, dps->DefaultIniName, PATH_MAX);
	QString scrpath(inipath);
	QFileInfo script(scrpath);
	scrpath = script.absolutePath() + QDir::separator() + "fileinfo.sh";
	script.setFile(scrpath);

	if (!script.isExecutable())
	{
		Dl_info dlinfo;
		memset(&dlinfo, 0, sizeof(dlinfo));

		if (dladdr(inipath, &dlinfo) != 0)
		{
			scrpath = QString(dlinfo.dli_fname);
			script.setFile(scrpath);
		}
	}

	path = script.absolutePath();

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
