#include <QFile>
#include <QTextCodec>
#include <QTableWidget>
#include <QHeaderView>

#include <enca.h>
#include <locale.h>

#include "wlxplugin.h"

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	char separator;
	int columns, row = 0;
	QStringList header, list;
	QFile file(FileToLoad);

	if (!file.open(QFile::ReadOnly | QFile::Text))
		return NULL;

	QByteArray line = file.read(4096);
	EncaAnalyser analyser;
	EncaEncoding encoding;
	char lang[3], codec_name[13];
	snprintf(lang, 3, "%s", setlocale(LC_ALL, ""));
	analyser = enca_analyser_alloc(lang);

	if (analyser)
	{
		enca_set_threshold(analyser, 1.38);
		enca_set_multibyte(analyser, 1);
		enca_set_ambiguity(analyser, 1);
		enca_set_garbage_test(analyser, 1);
		enca_set_filtering(analyser, 0);
		encoding = enca_analyse(analyser, (unsigned char*)line.data(), (size_t)line.size());

		switch (encoding.charset)
		{

		case 8:
			strcpy(codec_name, "Windows-1251");
			break;

		case 13:
			strcpy(codec_name, "IBM 866");
			break;

		case 20:
			strcpy(codec_name, "KOI8-R");
			break;

		default:
			strcpy(codec_name, "System");
		}

		enca_analyser_free(analyser);
	}


	file.seek(0);
	line = file.readLine();
	QTextCodec *codec = QTextCodec::codecForName(codec_name);
	QByteArray seps(",;\t");

	QTableWidget *view = new QTableWidget((QWidget*)ParentWin);

	for (int i = 0; i < seps.size(); ++i)
	{
		separator = seps.at(i);

		if (codec)
			header = codec->toUnicode(line).split(QLatin1Char(separator));
		else
			header = QString(line).split(QLatin1Char(separator));

		columns = header.size();

		if (columns > 1)
		{
			view->setColumnCount(columns);

			for (int c = 0; c < columns; ++c)
				view->setHorizontalHeaderItem(c, new QTableWidgetItem(header.at(c).trimmed()));

			break;
		}
	}

	if (columns <= 1)
	{
		delete view;
		return NULL;
	}

	//view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	while (!file.atEnd())
	{
		view->insertRow(row);
		line = file.readLine();

		if (codec)
			list = codec->toUnicode(line).split(QLatin1Char(separator));
		else
			list = QString(line).split(QLatin1Char(separator));

		if (list.size() > columns)
		{
			columns = list.size();
			view->setColumnCount(columns);
		}

		for (int c = 0; c < list.size(); ++c)
		{
			QTableWidgetItem *item = new QTableWidgetItem(list.at(c).trimmed());
			item->setToolTip(list.at(c).trimmed());
			item->setFlags(Qt::ItemIsSelectable);
			view->setItem(row, c, item);
		}

		row++;
	}

	file.close();

	view->setSortingEnabled(true);
	view->setSelectionMode(QAbstractItemView::SingleSelection);
	view->show();

	return view;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QTableWidget *view = (QTableWidget*)ListWin;
	delete view;
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	snprintf(DetectString, maxlen - 1, "EXT=\"CSV\"");
}
