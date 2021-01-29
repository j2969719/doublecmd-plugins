#include <QtSql>
#include <QDebug>
#include <QDialog>
#include <QtWidgets>
#include <QHeaderView>
#include <QTableWidget>
#include "wlxplugin.h"

Q_DECLARE_METATYPE(QSqlDatabase)

static QMimeDatabase db;

static void fill_table(QTableWidget *table, QString strquery, QSqlQuery query)
{
	table->setColumnCount(0);
	table->setRowCount(0);

	if (!query.exec(strquery))
		QMessageBox::critical((QWidget*)table, "", query.lastError().text());
	else
	{
		QSqlRecord rec = query.record();

		if (rec.isEmpty())
			return;

		table->setColumnCount(rec.count());

		for (int i = 0; i < rec.count(); i++)
			table->setHorizontalHeaderItem(i, new QTableWidgetItem(rec.fieldName(i)));

		for (int i = 0; query.next(); i++)
		{
			table->insertRow(i);

			for (int c = 0; c < rec.count(); ++c)
			{
				QTableWidgetItem *item = new QTableWidgetItem(query.value(c).toString());
				item->setToolTip(query.value(c).toString());
				item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
				table->setItem(i, c, item);
			}
		}
	}
}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QVariant vdbase;
	QString strquery;
	QSqlDatabase dbase;

	QMimeType type = db.mimeTypeForFile(QString(FileToLoad));

	qDebug() << type.name();

	if (type.name() == "application/vnd.sqlite3")
	{
		dbase = QSqlDatabase::addDatabase("QSQLITE");
		dbase.setConnectOptions("QSQLITE_OPEN_READONLY");
		strquery = QString("SELECT name, sql FROM sqlite_master WHERE type='table';");
	}
	else
		return nullptr;


	dbase.setDatabaseName(FileToLoad);

	if (!dbase.open())
		return nullptr;

	QSqlQuery query;

	if (!query.exec(strquery))
	{
		qDebug() << query.lastError();
		dbase.close();
		return nullptr;
	}

	QFrame *view = new QFrame((QWidget*)ParentWin);
	view->setFrameStyle(QFrame::NoFrame);
	QVBoxLayout *main = new QVBoxLayout(view);
	QHBoxLayout *controls = new QHBoxLayout;

	QComboBox *cbtables = new QComboBox(view);

	QLabel *lquery = new QLabel(view);
	lquery->setAlignment(Qt::AlignCenter);

	QPushButton *bquery = new QPushButton(view);
	bquery->setText("Query");

	controls->addWidget(cbtables);
	controls->addStretch(1);
	controls->addWidget(lquery);
	controls->addStretch(1);
	controls->addWidget(bquery);
	main->addLayout(controls);

	QTableWidget *table = new QTableWidget(view);
	table->setObjectName("table");
	table->setSortingEnabled(true);
	table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	main->addWidget(table);

	QObject::connect(cbtables, QOverload<int>::of(&QComboBox::currentIndexChanged), [cbtables, table, lquery, type, query](int x)
	{
		QString strquery;

		if (type.name() == "application/vnd.sqlite3")
			strquery = QString("SELECT * FROM %1;").arg(cbtables->currentText());

		if (!strquery.isEmpty())
		{
			lquery->setText(strquery);
			fill_table(table, strquery, query);
		}
	});

	QObject::connect(bquery, &QPushButton::clicked, [cbtables, table, lquery, query](int x)
	{
		bool ret;
		QString strquery = QInputDialog::getText((QWidget*)table, "", "Query", QLineEdit::Normal, lquery->text(), &ret);

		if (ret && !strquery.isEmpty())
		{
			lquery->setText(strquery);
			fill_table(table, strquery, query);
		}
	});

	while (query.next())
		cbtables->addItem(query.value(0).toString());

	vdbase.setValue(dbase);
	view->setProperty("base", vdbase);

	view->show();

	return view;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QString name;
	QFrame *view = (QFrame*)ListWin;

	{
		QSqlDatabase dbase = view->property("base").value<QSqlDatabase>();
		name = dbase.connectionName();
		dbase.close();
	}


	delete view;

	QSqlDatabase::removeDatabase(name);
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
			QString html("<html><body><table><tr><td>");

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

		if (needle != prev)
		{
			i = 0;
		}
		else if (SearchParameter & lcs_backwards)
		{
			if (i > 0)
				i--;
		}
		else
		{
			if (i < list.size())
				i++;
		}

		if (i >= 0 && i < list.size() && list.at(i))
		{
			view->scrollToItem(list.at(i));
			view->setCurrentItem(list.at(i));
		}

		view->setProperty("findit", i);

		return LISTPLUGIN_OK;
	}

	return LISTPLUGIN_ERROR;
}
