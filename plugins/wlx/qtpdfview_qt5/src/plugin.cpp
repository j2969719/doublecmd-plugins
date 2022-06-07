#include <QtPdf>
#include <QWidget>
#include <QPdfView>
#include <QtWidgets>
#include "wlxplugin.h"

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QPdfDocument *document = new QPdfDocument();

	if (document->load(QString(FileToLoad)) != QPdfDocument::NoError)
	{
		delete document;
		return NULL;
	}
	QFrame *view = new QFrame((QWidget*)ParentWin);
	QFrame *frmControls = new QFrame(view);
	view->setFrameStyle(QFrame::NoFrame);
	frmControls->setFrameStyle(QFrame::NoFrame);

		
	QVBoxLayout *main = new QVBoxLayout(view);
	QHBoxLayout *controls = new QHBoxLayout(frmControls);
		
	QPdfView *pdfView = new QPdfView(view);
	main->setSpacing(0);
	main->setMargin(0);
	main->setContentsMargins(1,1,1,1);
	main->addWidget(pdfView,15);
	main->addWidget(frmControls,1);
	  
	QPushButton *btnZoomIn = new QPushButton(view);
	btnZoomIn->setIcon(QIcon::fromTheme("zoom-in"));
	QObject::connect(btnZoomIn, &QPushButton::clicked, [pdfView]()
	{
	  pdfView->setZoomFactor(pdfView->zoomFactor() + 0.05);
	});
	btnZoomIn->setFocusPolicy(Qt::NoFocus);
	controls->addWidget(btnZoomIn);

	QPushButton *btnZoomOut = new QPushButton(view);
	btnZoomOut->setIcon(QIcon::fromTheme("zoom-out"));
	QObject::connect(btnZoomOut, &QPushButton::clicked, [pdfView]()
	{
		pdfView->setZoomFactor(pdfView->zoomFactor() - 0.05);
	});
	btnZoomOut->setFocusPolicy(Qt::NoFocus);
	controls->addWidget(btnZoomOut);

	QPushButton *btnZoomOrg = new QPushButton(view);
	btnZoomOrg->setIcon(QIcon::fromTheme("zoom-original"));
	QObject::connect(btnZoomOrg, &QPushButton::clicked, [pdfView]()
	{
		pdfView->setZoomFactor(1);
	});
	btnZoomOrg->setFocusPolicy(Qt::NoFocus);
	controls->addWidget(btnZoomOrg);
	controls->addSpacing(10);


	pdfView->setDocument(document);
	pdfView->setPageMode(QPdfView::MultiPage);
	//pdfView->setZoomMode(QPdfView::FitToWidth);

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
