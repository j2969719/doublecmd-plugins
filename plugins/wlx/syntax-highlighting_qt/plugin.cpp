#include <QFile>
#include <QPlainTextEdit>

#include <SyntaxHighlighter>
#include <Repository>
#include <Definition>
#include <Theme>

#include "wlxplugin.h"

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	KSyntaxHighlighting::Repository *repo = new KSyntaxHighlighting::Repository();
	KSyntaxHighlighting::Definition definition = repo->definitionForFileName(FileToLoad);

	if (!definition.isValid())
		return NULL;

	QFile file(FileToLoad);

	if (!file.open(QFile::ReadOnly | QFile::Text))
		return NULL;

	QPlainTextEdit *view = new QPlainTextEdit((QWidget*)ParentWin);
	view->setPlainText(file.readAll());
	file.close();
	view->setReadOnly(true);


	KSyntaxHighlighting::SyntaxHighlighter *highlighter = new KSyntaxHighlighting::SyntaxHighlighter();
	highlighter->setDefinition(definition);
	//highlighter->setTheme(repo->defaultTheme(KSyntaxHighlighting::Repository::DarkTheme));
	highlighter->setTheme(repo->defaultTheme(KSyntaxHighlighting::Repository::LightTheme));
	highlighter->setDocument(view->document());
	view->show();

	return view;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	delete (QPlainTextEdit*)ListWin;
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	QTextDocument::FindFlags sflags;

	if (SearchParameter & lcs_matchcase)
		sflags |= QTextDocument::FindBackward;

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
