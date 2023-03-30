#include <QtWidgets>
#include <QtConcurrent>
#include "wlxplugin.h"

static QMimeDatabase gDB;

static qint64 CalcSize(QString path)
{
	QFileInfo fi(path);

	if (fi.isFile())
		return fi.size();

	qint64 total = 0;
	QDirIterator iter(path, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);

	while (iter.hasNext())
	{
		iter.next();

		if (iter.fileInfo().isFile() && !iter.fileInfo().isSymLink())
			total += iter.fileInfo().size();
	}

	return total;
}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QMimeType type = gDB.mimeTypeForFile(QString(FileToLoad));

	if (type.name() != "inode/directory")
		return nullptr;

	QFrame *view = new QFrame((QWidget*)ParentWin);
	view->setFrameStyle(QFrame::NoFrame);
	QVBoxLayout *main = new QVBoxLayout(view);
	QHBoxLayout *controls = new QHBoxLayout;

	QLineEdit *lpath = new QLineEdit(view);
	lpath->setReadOnly(true);
	lpath->hide();
	lpath->setObjectName("pathlabel");
	controls->addWidget(lpath);
	main->addLayout(controls);

	QTableWidget *table = new QTableWidget(view);
	table->setObjectName("table");
	table->setSortingEnabled(true);
	table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	QStringList columns;
	columns << "Name" << "Size";

	table->setColumnCount(columns.size());

	for (int i = 0; i < columns.size(); ++i)
		table->setHorizontalHeaderItem(i, new QTableWidgetItem(columns.at(i)));

	table->sortByColumn(1, Qt::DescendingOrder);

	table->setShowGrid(false);
	main->addWidget(table);

	QObject::connect(lpath, &QLineEdit::textChanged, [table](const QString text)
	{
		int row = 0;
		QLocale locale;
		QDirIterator iter(text, QDir::Files | QDir::Dirs | QDir::Hidden);
		table->setRowCount(0);

		QMap<QString, QFuture<qint64>> fields;

		while (iter.hasNext())
		{
			iter.next();

			if (iter.fileInfo().fileName() == ".." || iter.fileInfo().fileName() == ".")
				continue;

			fields[iter.fileInfo().filePath()] = QtConcurrent::run(CalcSize, iter.fileInfo().filePath());
		}

		QMapIterator<QString, QFuture<qint64>> i(fields);

		while (i.hasNext())
		{
			i.next();
			int col = 0;
			table->insertRow(row);
			QMimeType type = gDB.mimeTypeForFile(i.key());
			QIcon icon = QIcon::fromTheme(type.iconName());

			if (icon.isNull())
				icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);

			QTableWidgetItem *item = new QTableWidgetItem(icon, i.key());
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			table->setItem(row, col++, item);

			item = new QTableWidgetItem;
			item->setData(Qt::DisplayRole, QVariant(i.value()));
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
			table->setItem(row, col++, item);
		}
	});

	view->show();

	lpath->setText(QString(FileToLoad));

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QMimeType type = gDB.mimeTypeForFile(QString(FileToLoad));

	if (type.name() != "inode/directory")
		return LISTPLUGIN_ERROR;

	QFrame *view = (QFrame*)ParentWin;
	QLineEdit *lpath = view->findChild<QLineEdit*>("pathlabel");
	lpath->setText(QString(FileToLoad));

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QFrame *view = (QFrame*)ListWin;
	delete view;
}
