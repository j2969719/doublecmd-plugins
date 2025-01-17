#include <QUrl>
#include <QWebEngineView>
#include <QWebEngineSettings>
#include "wlxplugin.h"

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QWebEngineView *view = new QWebEngineView((QWidget*)ParentWin);

	//qt 6.5+
	//view->settings()->setAttribute(QWebEngineSettings::ForceDarkMode, true);

	view->load(QUrl::fromLocalFile(FileToLoad));
	view->show();

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QWebEngineView *view = (QWebEngineView*)PluginWin;

	view->load(QUrl::fromLocalFile(FileToLoad));

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QWebEngineView *view = (QWebEngineView*)ListWin;
	view->~QWebEngineView();
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	QWebEnginePage::FindFlags sflags;
	QWebEngineView *view = (QWebEngineView*)ListWin;

	if (SearchParameter & lcs_matchcase)
		sflags |= QWebEnginePage::FindCaseSensitively;

	if (SearchParameter & lcs_backwards)
		sflags |= QWebEnginePage::FindBackward;

	view->findText(SearchString, sflags);

	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	QWebEngineView *view = (QWebEngineView*)ListWin;

	switch (Command)
	{
	case lc_copy :
		view->triggerPageAction(QWebEnginePage::Copy);
		break;

	case lc_selectall :
		view->triggerPageAction(QWebEnginePage::SelectAll);
		break;

	default :
		return LISTPLUGIN_ERROR;
	}

	return LISTPLUGIN_OK;
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	snprintf(DetectString, maxlen - 1, "%s", DETECT_STRING);
}
