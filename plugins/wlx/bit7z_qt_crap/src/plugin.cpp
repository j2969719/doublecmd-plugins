#include <QtWidgets>
#include <bit7z/bit7z.hpp>
#include "wlxplugin.h"

using namespace bit7z;

// Bit7zLibrary gBit7zLib { "/usr/lib/p7zip/7z.so" };
Bit7zLibrary gBit7zLib { "/usr/lib/7zip/7z.so" };

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	QLocale locale;
	BitArchiveReader *reader;
	vector<bit7z::BitArchiveItemInfo> items;

	try
	{
		reader = new BitArchiveReader{ gBit7zLib, FileToLoad, BitFormat::Auto };
		items = reader->items();
	}
	catch (const bit7z::BitException& ex)
	{
		return nullptr;
	}

	QFrame *view = new QFrame((QWidget*)ParentWin);
	view->setFrameStyle(QFrame::NoFrame);
	QVBoxLayout *main = new QVBoxLayout(view);

	QTableWidget *table = new QTableWidget(view);
	table->setObjectName("table");

	QStringList columns =
	{
		"MainSubfile",
		"HandlerItemIndex",
		"Path",
		"Name",
		"Extension",
		"IsDir",
		"Size",
		"PackSize",
		"Attrib",
		"CTime",
		"ATime",
		"MTime",
		"Solid",
		"Commented",
		"Encrypted",
		"SplitBefore",
		"SplitAfter",
		"DictionarySize",
		"CRC",
		"Type",
		"IsAnti",
		"Method",
		"HostOS",
		"FileSystem",
		"User",
		"Group",
		"Block",
		"Comment",
		"Position",
		"Prefix",
		"NumSubDirs",
		"NumSubFiles",
		"UnpackVer",
		"Volume",
		"IsVolume",
		"Offset",
		"Links",
		"NumBlocks",
		"NumVolumes",
		"TimeType",
		"Bit64",
		"BigEndian",
		"Cpu",
		"PhySize",
		"HeadersSize",
		"Checksum",
		"Characts",
		"Va",
		"Id",
		"ShortName",
		"CreatorApp",
		"SectorSize",
		"PosixAttrib",
		"SymLink",
		"Error",
		"TotalSize",
		"FreeSpace",
		"ClusterSize",
		"VolumeName",
		"LocalName",
		"Provider",
		"NtSecure",
		"IsAltStream",
		"IsAux",
		"IsDeleted",
		"IsTree",
		"Sha1",
		"Sha256",
		"ErrorType",
		"NumErrors",
		"ErrorFlags",
		"WarningFlags",
		"Warning",
		"NumStreams",
		"NumAltStreams",
		"AltStreamsSize",
		"VirtualSize",
		"UnpackSize",
		"TotalPhySize",
		"VolumeIndex",
		"SubType",
		"ShortComment",
		"CodePage",
		"IsNotArcType",
		"PhySizeCantBeDetected",
		"ZerosTailIsAllowed",
		"TailSize",
		"EmbeddedStubSize",
		"NtReparse",
		"HardLink",
		"INode",
		"StreamId",
		"ReadOnly",
		"OutName",
		"CopyLink",
	};
	table->setColumnCount(columns.size());

	for (int i = 0; i < columns.size(); ++i)
		table->setHorizontalHeaderItem(i, new QTableWidgetItem(columns.at(i)));

	int row = 0;
	uint32_t count = reader->itemsCount();

	for (uint32_t i = 0; i < count; i++)
	{
		table->insertRow(row);

		for (int p = (int)BitProperty::MainSubfile; p <= (int)BitProperty::CopyLink; p++)
		{
			QTableWidgetItem *item = new QTableWidgetItem(items[i].itemProperty((bit7z::BitProperty)p).toString().c_str());
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			table->setItem(row, p - 1, item);
		}

		row++;
	}

	auto props = reader->archiveProperties();
	QLabel *info = new QLabel(view);
	info->setText(QString::asprintf("%d items: %d file(s) %d folder(s)\n%s\n%s", reader->itemsCount(), reader->filesCount(), reader->foldersCount(),
	                                props[BitProperty::Method].toString().c_str(), props[BitProperty::Comment].toString().c_str()));
	info->setAlignment(Qt::AlignCenter);
	main->addWidget(info);

	QLabel *infosize = new QLabel(view);
	infosize->setAlignment(Qt::AlignCenter);
	infosize->setText(QString("PackSize: %1, Size: %2").arg(locale.formattedDataSize(reader->packSize())).arg(locale.formattedDataSize(reader->size())));
	main->addWidget(infosize);

	QProgressBar *bar = new QProgressBar(view);
	double res = (double)reader->packSize() / (double)reader->size();

	if (res > 1)
		res = 0;
	else
		res = 1 - res;

	bar->setRange(0, 100);
	bar->setValue((int)(res * 100));
	bar->setTextVisible(false);
	main->addWidget(bar);


	table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	table->setSortingEnabled(true);
	main->addWidget(table);
	view->show();

	delete reader;

	return view;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	QFrame *frame = (QFrame*)ListWin;
	QTableWidget *view = frame->findChild<QTableWidget*>("table");
	view->clear();
	delete view;
	delete frame;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	QFrame *frame = (QFrame*)ListWin;
	QTableWidget *view = frame->findChild<QTableWidget*>("table");

	switch (Command)
	{
	case lc_copy :
	{
		QModelIndexList sel = view->selectionModel()->selectedIndexes();

		if (sel.count() < 1)
			return LISTPLUGIN_ERROR;
		else if (sel.count() == 1)
			QApplication::clipboard()->setText(sel[0].data(Qt::DisplayRole).toString());
		else
		{
			QMimeData *mimedata = new QMimeData;
			QString html("<html><head><meta charset=\"utf-8\"></head><body><table><tr><td>");

			for (int i = 0; i < sel.count(); ++i)
			{
				QModelIndex cur = sel[i];
				html.append(cur.data(Qt::DisplayRole).toString());

				if (i + 1 < sel.count())
				{
					QModelIndex next = sel[i + 1];

					if (next.row() != cur.row())
						html.append("</td></tr><tr><td>");
					else
						html.append("</td><td>");
				}
			}

			html.append("</td></tr></table></body></html>");
			mimedata->setHtml(html);
			QApplication::clipboard()->setMimeData(mimedata);
		}

		break;
	}

	case lc_selectall :
		view->selectAll();
		break;

	default :
		return LISTPLUGIN_ERROR;
	}

	return LISTPLUGIN_OK;
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	QList<QTableWidgetItem*> list;
	QFrame *frame = (QFrame*)ListWin;
	QTableWidget *view = frame->findChild<QTableWidget*>("table");

	Qt::MatchFlags sflags = Qt::MatchContains;

	if (SearchParameter & lcs_matchcase)
		sflags |= Qt::MatchCaseSensitive;

	QString needle(SearchString);
	QString prev = view->property("needle").value<QString>();
	view->setProperty("needle", needle);

	list = view->findItems(QString(SearchString), sflags);

	if (!list.isEmpty())
	{
		int i = view->property("findit").value<int>();

		if (needle != prev || SearchParameter & lcs_findfirst)
		{
			if (SearchParameter & lcs_backwards)
				i = list.size() - 1;
			else
				i = 0;
		}
		else if (SearchParameter & lcs_backwards)
			i--;
		else
			i++;

		if (i >= 0 && i < list.size() && list.at(i))
		{
			view->scrollToItem(list.at(i));
			view->setCurrentItem(list.at(i));
			view->setProperty("findit", i);
			return LISTPLUGIN_OK;
		}
	}

	QMessageBox::information(view, "", QString::asprintf("\"%s\" not found!", SearchString));

	return LISTPLUGIN_ERROR;
}
