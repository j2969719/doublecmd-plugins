#include <QtPdf>
#include <QPdfView>
#include <QtWidgets>
#include "wlxplugin.h"

static void update_pagecount(QLabel *label, QPdfView *view)
{
	int total = view->pageNavigation()->pageCount();
	int page = view->pageNavigation()->currentPage() + 1;
	label->setText(QString("%1/%2").arg(page).arg(total));
}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QPdfDocument *document = new QPdfDocument();

	if (document->load(QString(FileToLoad)) != QPdfDocument::NoError)
	{
		delete document;
		return nullptr;
	}

	QFrame *view = new QFrame((QWidget*)ParentWin);
	view->setFrameStyle(QFrame::NoFrame);
	QVBoxLayout *main = new QVBoxLayout(view);
	QToolBar *controls = new QToolBar(view);

	QPdfView *pdfView = new QPdfView(view);
	main->setSpacing(0);
	main->setContentsMargins(1, 1, 1, 1);
	main->addWidget(controls);
	main->addWidget(pdfView);

	QAction *actFirst = new QAction(QIcon::fromTheme("go-first"), "First page", view);
	QObject::connect(actFirst, &QAction::triggered, [pdfView]()
	{
		pdfView->pageNavigation()->setCurrentPage(0);
	});
	controls->addAction(actFirst);

	QAction *actPrev = new QAction(QIcon::fromTheme("go-previous"), "Previous page", view);
	QObject::connect(actPrev, &QAction::triggered, [pdfView]()
	{
		pdfView->pageNavigation()->goToPreviousPage();
	});
	controls->addAction(actPrev);

	QAction *actNext = new QAction(QIcon::fromTheme("go-next"), "Next page", view);
	QObject::connect(actNext, &QAction::triggered, [pdfView]()
	{
		pdfView->pageNavigation()->goToNextPage();
	});
	controls->addAction(actNext);

	QAction *actLast = new QAction(QIcon::fromTheme("go-last"), "Last page", view);
	QObject::connect(actLast, &QAction::triggered, [pdfView]()
	{
		int pages = pdfView->pageNavigation()->pageCount();

		if (pages > 0)
			pdfView->pageNavigation()->setCurrentPage(pages - 1);
	});
	controls->addAction(actLast);

	controls->addSeparator();
	QLabel *lblPages = new QLabel(view);
	QObject::connect(pdfView->pageNavigation(), &QPdfPageNavigation::pageCountChanged, [lblPages, pdfView]()
	{
		update_pagecount(lblPages, pdfView);
	});
	QObject::connect(pdfView->pageNavigation(), &QPdfPageNavigation::currentPageChanged, [lblPages, pdfView]()
	{
		update_pagecount(lblPages, pdfView);
	});
	controls->addWidget(lblPages);
	controls->addSeparator();

	QAction *actZoomIn = new QAction(QIcon::fromTheme("zoom-in"), "Zoom In", view);
	QObject::connect(actZoomIn, &QAction::triggered, [pdfView]()
	{
		if (pdfView->zoomMode() != QPdfView::CustomZoom)
			pdfView->setZoomMode(QPdfView::CustomZoom);

		pdfView->setZoomFactor(pdfView->zoomFactor() + 0.05);
	});
	controls->addAction(actZoomIn);

	QAction *actZoomOut = new QAction(QIcon::fromTheme("zoom-out"), "Zoom Out", view);
	QObject::connect(actZoomOut, &QAction::triggered, [pdfView]()
	{
		if (pdfView->zoomMode() != QPdfView::CustomZoom)
			pdfView->setZoomMode(QPdfView::CustomZoom);

		pdfView->setZoomFactor(pdfView->zoomFactor() - 0.05);
	});
	controls->addAction(actZoomOut);

	QAction *actZoomOrg = new QAction(QIcon::fromTheme("zoom-original"), "Normal Size", view);
	QObject::connect(actZoomOrg, &QAction::triggered, [pdfView]()
	{
		if (pdfView->zoomMode() != QPdfView::CustomZoom)
			pdfView->setZoomMode(QPdfView::CustomZoom);

		pdfView->setZoomFactor(1);
	});
	controls->addAction(actZoomOrg);

	controls->addSeparator();

	QAction *actFit = new QAction(QIcon::fromTheme("zoom-fit-best"), "Fit", view);
	actFit->setCheckable(true);
	QObject::connect(actFit, &QAction::triggered, [pdfView]()
	{
		if (pdfView->zoomMode() == QPdfView::FitInView)
			pdfView->setZoomMode(QPdfView::FitToWidth);
		else
			pdfView->setZoomMode(QPdfView::FitInView);
	});
	controls->addAction(actFit);

	QAction *actPageMode = new QAction(QIcon::fromTheme("document-page-setup"), "Page Mode", view);
	actPageMode->setCheckable(true);
	QObject::connect(actPageMode, &QAction::triggered, [pdfView]()
	{
		if (pdfView->pageMode() != QPdfView::MultiPage)
			pdfView->setPageMode(QPdfView::MultiPage);
		else
			pdfView->setPageMode(QPdfView::SinglePage);
	});
	controls->addAction(actPageMode);

	controls->addSeparator();

	QAction *actInfo = new QAction(QIcon::fromTheme("dialog-information"), "Info", view);
	QObject::connect(actInfo, &QAction::triggered, [pdfView]()
	{
		QString info;
		QMap<QString, QPdfDocument::MetaDataField> fields;
		fields["Author"] = QPdfDocument::Author;
		fields["Title"] = QPdfDocument::Title;
		fields["Subject"] = QPdfDocument::Subject;
		fields["Producer"] = QPdfDocument::Producer;
		fields["Creator"] = QPdfDocument::Creator;
		fields["Keywords"] = QPdfDocument::Keywords;
		fields["Creation Date"] = QPdfDocument::CreationDate;
		fields["Modification Date"] = QPdfDocument::ModificationDate;
		QMapIterator<QString, QPdfDocument::MetaDataField> i(fields);

		while (i.hasNext())
		{
			i.next();
			QString value = pdfView->document()->metaData(i.value()).toString();

			if (!value.isEmpty())
			{
				info.append(i.key());
				info.append(": ");
				info.append(value);
				info.append("\n");
			}
		}

		if (!info.isEmpty())
			QMessageBox::information(pdfView, "", info);
		else
			QMessageBox::information(pdfView, "", "no suitable info available");
	});
	controls->addAction(actInfo);

	pdfView->setDocument(document);
	pdfView->setPageMode(QPdfView::MultiPage);
	pdfView->setObjectName("pdf_view");
	view->show();

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QFrame *view = (QFrame*)ParentWin;
	QPdfView *pdfView = view->findChild<QPdfView*>("pdf_view");
	QPdfDocument *document = pdfView->document();

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
	QFrame *view = (QFrame*)ListWin;
	QPdfView *pdfView = view->findChild<QPdfView*>("pdf_view");
	QPdfDocument *document = pdfView->document();

	if (document != NULL)
	{
		document->close();
		delete document;
	}

	delete pdfView;
	delete view;
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	snprintf(DetectString, maxlen - 1, "EXT=\"PDF\"");
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}
