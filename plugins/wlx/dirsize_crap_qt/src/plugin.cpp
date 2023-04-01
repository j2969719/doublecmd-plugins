#include <QtCharts>
#include <QtWidgets>
#include <QtConcurrent>
#include "wlxplugin.h"

#define MAXSLICES 10
static QMimeDatabase gDB;

static qint64 CalcSize(QString path)
{
	QFileInfo fi(path);

	if (fi.isFile() || fi.isSymLink())
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

static QMap<QString, qint64> GetSizes(QString path)
{
	qDebug() << "GetSizes";
	QMap<QString, qint64> result;

	QMap<QString, QFuture<qint64>> fields;
	QDirIterator iter(path, QDir::Files | QDir::Dirs | QDir::Hidden);

	while (iter.hasNext())
	{
		iter.next();

		if (iter.fileInfo().fileName() == ".." || iter.fileInfo().fileName() == ".")
			continue;

		result[iter.fileInfo().filePath()] = CalcSize(iter.fileInfo().filePath());
	}

	qDebug() << result;
	return result;
}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QFileInfo fi(FileToLoad);

	if (!fi.isDir())
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

	QChartView *chart = new QChartView(view);
	QPieSeries *series = new QPieSeries(view);
	chart->chart()->addSeries(series);
	QPalette palette = QApplication::palette();
	chart->chart()->setBackgroundBrush(palette.brush(QPalette::Window));
	chart->chart()->setTitleBrush(palette.brush(QPalette::WindowText));
	chart->chart()->legend()->setAlignment(Qt::AlignRight);
	chart->chart()->legend()->setLabelColor(palette.color(QPalette::WindowText));
	main->addWidget(chart);

	QTableWidget *table = new QTableWidget(view);
	table->setObjectName("table");
	table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	table->setShowGrid(false);
	table->verticalHeader()->hide();
	table->horizontalHeader()->hide();
	main->addWidget(table);

	QStringList columns;
	columns << "Name" << "Size" << "Type";
	table->setColumnCount(columns.size());

	for (int i = 0; i < columns.size(); ++i)
		table->setHorizontalHeaderItem(i, new QTableWidgetItem(columns.at(i)));

	QFutureWatcher<QMap<QString, qint64>> *watcher = new QFutureWatcher<QMap<QString, qint64>>(view);
	watcher->setObjectName("watcher");

	QObject::connect(watcher, &QFutureWatcher<QMap<QString, qint64>>::finished, [table, lpath, watcher, series, chart]()
	{
		QLocale locale;
		auto result = watcher->result();
		QMapIterator<QString, qint64> iter(result);

		series->clear();

		qint64 total = 0;
		qint64 maxsize = 0;
		QStringList list;

		while (iter.hasNext())
		{
			iter.next();
			total += iter.value();

			if (iter.key() != "...")
			{
				if (iter.value() > maxsize)
				{
					maxsize = iter.value();
					list.prepend(iter.key());
				}
				else
				{
					bool inserted = false;

					for (auto i = 0; i < list.size(); i++)
					{
						if (result[list[i]] <= iter.value())
						{
							inserted = true;
							list.insert(i, iter.key());
							break;
						}
					}

					if (!inserted)
						list.append(iter.key());
				}
			}
		}

		int row = 0;
		qint64 rest = 0;

		table->setRowCount(list.size());

		for (const auto& i : list)
		{
			QFileInfo fi(i);

			if (row <= MAXSLICES)
				series->append(QString("%1 (%2)").arg(fi.fileName()).arg(locale.formattedDataSize(result[i])), result[i]);
			else
				rest += result[i];

			int col = 0;
			QMimeType type = gDB.mimeTypeForFile(i);
			QIcon icon = QIcon::fromTheme(type.iconName());

			if (icon.isNull())
				icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);

			QTableWidgetItem *item = new QTableWidgetItem(icon, fi.fileName());
			item->setFlags(Qt::ItemIsEnabled);
			table->setItem(row, col++, item);

			item = new QTableWidgetItem(locale.formattedDataSize(result[i]));
			item->setFlags(Qt::ItemIsEnabled);
			item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
			table->setItem(row, col++, item);

			item = new QTableWidgetItem(type.comment());
			item->setFlags(Qt::ItemIsEnabled);
			table->setItem(row, col++, item);

			row++;

		}

		if (row > MAXSLICES)
			series->append(QString("... (%1: %2)").arg(row - MAXSLICES).arg(locale.formattedDataSize(rest)), rest);

		chart->chart()->setTitle(QString("%1 (%2: %3)").arg(lpath->text()).arg(row).arg(locale.formattedDataSize(total)));
	});

	QObject::connect(lpath, &QLineEdit::textChanged, [table, series, watcher, chart](const QString text)
	{
		table->setRowCount(0);
		series->clear();
		chart->chart()->setTitle("Scaning...");
		watcher->setFuture(QtConcurrent::run(GetSizes, text));
	});

	view->show();
	lpath->setText(QString(FileToLoad));

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QFileInfo fi(FileToLoad);

	if (!fi.isDir())
		return LISTPLUGIN_ERROR;

	QFrame *view = (QFrame*)ParentWin;
	QLineEdit *lpath = view->findChild<QLineEdit*>("pathlabel");
	lpath->setText(QString(FileToLoad));

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QFrame *view = (QFrame*)ListWin;
	QFutureWatcher<QMap<QString, qint64>> *watcher = view->findChild<QFutureWatcher<QMap<QString, qint64>> *>("watcher");

	if (watcher->isRunning())
		watcher->waitForFinished();

	delete watcher;
	delete view;
}
