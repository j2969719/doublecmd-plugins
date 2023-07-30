#include <archive.h>
#include <archive_entry.h>

#include <QProcess>
#include <QMimeData>
#include <QtWidgets>
#include "wlxplugin.h"

#define COMMENT_CMD "7z l %s -y | pcregrep -M -o1 \"(?s)(?<=Comment\\s=\\s)(.*?)\n\n\\s+Date\""

static QMimeDatabase db;

QString getOutput(char* FileToLoad)
{
	QString ecapedpath = QString(FileToLoad).replace(" ", "\\ ").replace("'", "\\'");
	QString command = QString::asprintf(COMMENT_CMD, ecapedpath.toStdString().c_str());
	QProcess proc;
	QString exec = "/bin/sh";
	QStringList params;
	params << "-c" << command;
	proc.start(exec, params);
	proc.waitForFinished();
	QString output(proc.readAllStandardOutput());
	proc.close();
	return output;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{

	struct archive *a;
	struct archive_entry *entry;
	int r, row = 0;
	size_t totalsize = 0;

	if (db.mimeTypeForFile(FileToLoad).name() == "text/plain")
		return nullptr;

	a = archive_read_new();
	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);
	r = archive_read_open_filename(a, FileToLoad, 10240);

	if (r != ARCHIVE_OK)
	{
		archive_read_close(a);
		archive_read_free(a);
		return nullptr;
	}

	QFrame *view = new QFrame((QWidget*)ParentWin);
	view->setFrameStyle(QFrame::NoFrame);
	QVBoxLayout *main = new QVBoxLayout(view);

	QTableWidget *table = new QTableWidget(view);
	table->setObjectName("table");

	QStringList columns;
	columns << "Name" << "Size" << "Date" << "Attr" << "Owner" << "Symlink" << "Hardlink";

	table->setColumnCount(columns.size());

	for (int i = 0; i < columns.size(); ++i)
		table->setHorizontalHeaderItem(i, new QTableWidgetItem(columns.at(i)));

	QFont monofont;
	//monofont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	monofont.setFamily("mono");

	while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK || r == ARCHIVE_WARN)
	{
		int c = 0;
		table->insertRow(row);

		const char *pathname = archive_entry_pathname(entry);
		const char *hardlink = archive_entry_hardlink_utf8(entry);
		const char *symlink = archive_entry_symlink_utf8(entry);

		QIcon icon;

		if (archive_entry_filetype(entry) == AE_IFDIR)
			icon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
		else if (symlink || hardlink)
			if (QString(pathname).back() == QChar('/'))
				icon = QApplication::style()->standardIcon(QStyle::SP_DirLinkIcon);
			else
				icon = QApplication::style()->standardIcon(QStyle::SP_FileLinkIcon);
		else
		{
			QMimeType type = db.mimeTypeForFile(pathname);
			icon = QIcon::fromTheme(type.iconName());
		}

		if (icon.isNull())
			icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);

		QTableWidgetItem *item = new QTableWidgetItem(icon, pathname);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		table->setItem(row, c++, item);

		size_t entrysize = (size_t)archive_entry_size(entry);
		totalsize = totalsize + entrysize;
		item = new QTableWidgetItem;
		item->setData(Qt::EditRole, (qint64)entrysize);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
		item->setFont(monofont);
		table->setItem(row, c++, item);

		item = new QTableWidgetItem(QDateTime::fromSecsSinceEpoch(archive_entry_mtime(entry)).toString("yyyy-MM-dd hh:mm"));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		item->setFont(monofont);
		item->setTextAlignment(Qt::AlignCenter);
		table->setItem(row, c++, item);

		item = new QTableWidgetItem(QString(archive_entry_strmode(entry)).trimmed());
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		item->setFont(monofont);
		item->setTextAlignment(Qt::AlignCenter);
		table->setItem(row, c++, item);

		QString owner = QString("%1/%2").arg(archive_entry_uname_utf8(entry)).arg(archive_entry_gname_utf8(entry));
		item = new QTableWidgetItem;

		if (owner != "/")
			item->setData(Qt::EditRole, owner);

		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		item->setTextAlignment(Qt::AlignCenter);
		table->setItem(row, c++, item);

		item = new QTableWidgetItem(symlink);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		table->setItem(row, c++, item);

		item = new QTableWidgetItem(hardlink);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		table->setItem(row, c++, item);

		row++;
	}

	if (archive_format(a) == ARCHIVE_FORMAT_EMPTY || archive_format(a) == ARCHIVE_FORMAT_MTREE || archive_format(a) == 0)
	{
		archive_read_close(a);
		archive_read_free(a);
		table->clear();
		delete table;
		delete main;
		delete view;
		return nullptr;
	}

	QString infotext;

	if (r != ARCHIVE_EOF)
	{
		const char *errstr = archive_error_string(a);
		infotext = QString::asprintf("ERROR: %s", errstr ? errstr : "unknown error");
	}
	else
	{
		int fc = archive_filter_count(a);
		QString filters;

		for (int i = 0; i < fc; i++)
		{
			const char *fn = archive_filter_name(a, i);

			if (strcmp(fn, "none") != 0)
			{
				if (filters.isEmpty())
					filters = QString(fn);
				else
					filters.append(" " + QString(fn));
			}

			if (filters.isEmpty())
				infotext = QString::asprintf("%s, %d file(s)", (char*)archive_format_name(a), archive_file_count(a));
			else
				infotext = QString("%1 (filter(s): %2), %3 file(s)").arg(archive_format_name(a)).arg(filters).arg(archive_file_count(a));
		}
	}

	QLabel *info = new QLabel(view);
	info->setText(infotext);
	info->setAlignment(Qt::AlignCenter);
	main->addWidget(info);

	archive_read_close(a);
	archive_read_free(a);

	if (r == ARCHIVE_EOF)
	{
		QStringList comment_content;
		comment_content << "application/zip" << "application/x-7z-compressed" << "application/vnd.rar";
		QMimeType type = db.mimeTypeForFile(FileToLoad);

		for (int i = 0; i < comment_content.size(); ++i)
		{
			if (type.name() == comment_content.at(i))
			{
				QString output = getOutput(FileToLoad);
				//qDebug() << output;

				if (!output.isNull() && !output.isEmpty())
				{
					if (output.back() == QChar('\n'))
						output.chop(1);

					if (!output.isEmpty())
					{
						QHBoxLayout *hbox = new QHBoxLayout;
						QLabel *comment = new QLabel(view);
						comment->setText(output);
						comment->setFont(monofont);
						hbox->setAlignment(Qt::AlignCenter);
						hbox->addWidget(comment);
						main->addLayout(hbox);
					}
				}

				break;
			}
		}

		struct stat buf;

		if (totalsize > 0 && stat(FileToLoad, &buf) == 0)
		{
			QProgressBar *bar = new QProgressBar(view);
			double res = (double)buf.st_size / (double)totalsize;

			if (res > 1)
				res = 0;
			else
				res = 1 - res;

			bar->setRange(0, 100);
			bar->setValue((int)(res * 100));
			bar->setTextVisible(false);
			main->addWidget(bar);
		}
	}

	table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	table->setSortingEnabled(true);
	main->addWidget(table);
	QLabel *version = new QLabel(view);
	version->setText(archive_version_details());
	version->setAlignment(Qt::AlignRight);
	main->addWidget(version);
	view->show();

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
