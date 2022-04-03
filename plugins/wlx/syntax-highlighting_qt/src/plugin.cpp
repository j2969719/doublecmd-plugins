#include <QFile>
#include <QFontDatabase>
#include <QPlainTextEdit>

#include <QTextCodec>

#include <SyntaxHighlighter>
#include <Repository>
#include <Definition>
#include <Theme>

#include <QSettings>
#include <QFileInfo>

#include <QMimeDatabase>

#include <QMessageBox>

#include <dlfcn.h>
#include <libintl.h>
#include <locale.h>
#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

#include "wlxplugin.h"

Q_DECLARE_METATYPE(KSyntaxHighlighting::Repository *)
Q_DECLARE_METATYPE(KSyntaxHighlighting::SyntaxHighlighter *)

bool darktheme = false;
QFont font;
QMimeDatabase db;

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QMimeType type = db.mimeTypeForFile(QString(FileToLoad));

	if (type.name() == "application/octet-stream")
		return nullptr;

	QVariant vrepo, vhgl;
	KSyntaxHighlighting::Repository *repo = new KSyntaxHighlighting::Repository();
	KSyntaxHighlighting::Definition definition = repo->definitionForMimeType(type.name());

	if (!definition.isValid())
	{
		delete repo;
		return NULL;
	}

	QFile file(FileToLoad);

	if (!file.open(QFile::ReadOnly | QFile::Text))
	{
		delete repo;
		return NULL;
	}

	QPlainTextEdit *view = new QPlainTextEdit((QWidget*)ParentWin);
	view->setPlainText(file.readAll());
	file.close();
	view->setReadOnly(true);
	view->document()->setDefaultFont(font);

	if (ShowFlags & lcp_wraptext)
		view->setLineWrapMode(QPlainTextEdit::WidgetWidth);
	else
		view->setLineWrapMode(QPlainTextEdit::NoWrap);

	KSyntaxHighlighting::SyntaxHighlighter *highlighter = new KSyntaxHighlighting::SyntaxHighlighter();
	highlighter->setDefinition(definition);

	if (darktheme)
		highlighter->setTheme(repo->defaultTheme(KSyntaxHighlighting::Repository::DarkTheme));
	else
		highlighter->setTheme(repo->defaultTheme(KSyntaxHighlighting::Repository::LightTheme));

	highlighter->setDocument(view->document());
	view->show();

	vrepo.setValue(repo);
	vhgl.setValue(highlighter);
	view->setProperty("repo", vrepo);
	view->setProperty("hgl", vhgl);
	view->setProperty("filename", QString(FileToLoad));

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QMimeType type = db.mimeTypeForFile(QString(FileToLoad));

	if (type.name() == "application/octet-stream")
		return LISTPLUGIN_ERROR;

	QPlainTextEdit *view = (QPlainTextEdit*)PluginWin;
	KSyntaxHighlighting::Repository *repo = view->property("repo").value<KSyntaxHighlighting::Repository *>();
	KSyntaxHighlighting::SyntaxHighlighter *highlighter = view->property("hgl").value<KSyntaxHighlighting::SyntaxHighlighter *>();
	KSyntaxHighlighting::Definition definition = repo->definitionForMimeType(type.name());

	if (!definition.isValid())
		return LISTPLUGIN_ERROR;

	QFile file(FileToLoad);

	if (!file.open(QFile::ReadOnly | QFile::Text))
		return LISTPLUGIN_ERROR;

	view->setPlainText(file.readAll());
	file.close();
	highlighter->setDefinition(definition);

	view->setProperty("filename", QString(FileToLoad));

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QPlainTextEdit *view = (QPlainTextEdit*)ListWin;
	KSyntaxHighlighting::Repository *repo = view->property("repo").value<KSyntaxHighlighting::Repository *>();
	KSyntaxHighlighting::SyntaxHighlighter *highlighter = view->property("hgl").value<KSyntaxHighlighting::SyntaxHighlighter *>();
	delete repo;
	delete highlighter;
	delete view;
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	QTextDocument::FindFlags sflags;

	if (SearchParameter & lcs_matchcase)
		sflags |= QTextDocument::FindCaseSensitively;

	if (SearchParameter & lcs_backwards)
		sflags |= QTextDocument::FindBackward;

	QPlainTextEdit *view = (QPlainTextEdit*)ListWin;

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
	{
		if (Parameter & lcp_wraptext)
			view->setLineWrapMode(QPlainTextEdit::WidgetWidth);
		else
			view->setLineWrapMode(QPlainTextEdit::NoWrap);

		QString filename = view->property("filename").value<QString>();
		QFile file(filename);

		if (file.open(QFile::ReadOnly | QFile::Text))
		{
			QByteArray text = file.readAll();
			file.close();
			QString newtext;

			if (Parameter & lcp_ansi)
			{
				QTextCodec *codec = QTextCodec::codecForName("Windows-1251");
				newtext = codec->toUnicode(text);
			}
			else if (Parameter & lcp_ascii)
			{
				QTextCodec *codec = QTextCodec::codecForName("IBM 866");
				newtext = codec->toUnicode(text);
			}

			if (newtext.isEmpty())
				view->setPlainText(text);
			else
				view->setPlainText(newtext);
		}

		break;
	}

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
	int size = settings.value("synthighl/fontsize").toInt();

	if (size == 0)
	{
		font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
		size = 12;
		settings.setValue("synthighl/fontsize", 12);
		settings.setValue("synthighl/font", font.family());
		settings.setValue("synthighl/fontbold", font.bold());
		settings.setValue("synthighl/darktheme", darktheme);
	}

	font.setFamily(settings.value("synthighl/font").toString());
	font.setFixedPitch(true);
	font.setPointSize(size);
	font.setBold(settings.value("synthighl/fontbold").toBool());
	darktheme = settings.value("synthighl/darktheme").toBool();

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
