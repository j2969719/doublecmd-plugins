#include <QtPdf>
#include <QWidget>
#include <QPdfView>
#include "wlxplugin.h"

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QPdfDocument *document = new QPdfDocument();

	if (document->load(QString(FileToLoad)) != QPdfDocument::NoError)
	{
		delete document;
		return NULL;
	}

	QPdfView *view = new QPdfView((QWidget*)ParentWin);
	view->setDocument(document);
	view->setPageMode(QPdfView::MultiPage);
	view->setZoomMode(QPdfView::FitToWidth);

	view->show();

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QPdfView *view = (QPdfView*)PluginWin;
	QPdfDocument *document = view->document();

	if (document != NULL)
	{
		document->close();

		if (document->load(QString(FileToLoad)) != QPdfDocument::NoError)
			return LISTPLUGIN_ERROR;

		return LISTPLUGIN_OK;
	}

	return LISTPLUGIN_ERROR;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QPdfView *view = (QPdfView*)ListWin;
	QPdfDocument *document = view->document();

	if (document != NULL)
	{
		document->close();
		delete document;
	}

	delete view;
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	strncpy(DetectString, "EXT=\"PDF\"", maxlen - 1);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}
