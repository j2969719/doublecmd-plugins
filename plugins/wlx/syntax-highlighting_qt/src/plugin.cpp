#include <QFile>
#include <QFontDatabase>
#include <QPlainTextEdit>

#include <SyntaxHighlighter>
#include <Repository>
#include <Definition>
#include <Theme>

#include <QSettings>
#include <QFileInfo>

#include "wlxplugin.h"

Q_DECLARE_METATYPE(KSyntaxHighlighting::Repository *)
Q_DECLARE_METATYPE(KSyntaxHighlighting::SyntaxHighlighter *)

bool darktheme = false;
QFont font;

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QVariant vrepo, vhgl;
	KSyntaxHighlighting::Repository *repo = new KSyntaxHighlighting::Repository();
	KSyntaxHighlighting::Definition definition = repo->definitionForFileName(FileToLoad);

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

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QPlainTextEdit *view = (QPlainTextEdit*)PluginWin;
	KSyntaxHighlighting::Repository *repo = view->property("repo").value<KSyntaxHighlighting::Repository *>();
	KSyntaxHighlighting::SyntaxHighlighter *highlighter = view->property("hgl").value<KSyntaxHighlighting::SyntaxHighlighter *>();
	KSyntaxHighlighting::Definition definition = repo->definitionForFileName(FileToLoad);

	if (!definition.isValid())
		return LISTPLUGIN_ERROR;

	QFile file(FileToLoad);

	if (!file.open(QFile::ReadOnly | QFile::Text))
		return LISTPLUGIN_ERROR;

	view->setPlainText(file.readAll());
	file.close();
	highlighter->setDefinition(definition);

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

	if (view->find(SearchString, sflags))
		return LISTPLUGIN_OK;
	else
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
}
