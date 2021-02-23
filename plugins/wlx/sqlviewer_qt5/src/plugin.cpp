#include <QtSql>
#include <QDialog>
#include <QtWidgets>
#include <QHeaderView>
#include <QTableWidget>

#include <QMessageBox>

#include <dlfcn.h>
#include <libintl.h>
#include <locale.h>
#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

#include "wlxplugin.h"

static QMimeDatabase db;

static void fill_table(QTableWidget *table, QString strquery, const QString conname)
{
	QSqlDatabase dbase = QSqlDatabase::database(conname);
	QSqlQuery query(dbase);

	if (!dbase.isValid())
		QMessageBox::critical((QWidget*)table, "", _("base not valid!"));

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
	QSqlDatabase dbase;
	QString dbtype, opts;

	QMimeType type = db.mimeTypeForFile(QString(FileToLoad));

	const QString conname = QString("wlxsql_connection_%1").arg(QTime::currentTime().toString());

	if (type.name() == "application/vnd.sqlite3" && QSqlDatabase::isDriverAvailable("QSQLITE"))
	{
		dbtype = QString("QSQLITE");
		opts = QString("QSQLITE_OPEN_READONLY");
	}
	else
		return nullptr;

	dbase = QSqlDatabase::addDatabase(dbtype, conname);
	dbase.setConnectOptions(opts);
	dbase.setDatabaseName(FileToLoad);

	if (!dbase.open())
		return nullptr;

	QFrame *view = new QFrame((QWidget*)ParentWin);
	view->setFrameStyle(QFrame::NoFrame);
	QVBoxLayout *main = new QVBoxLayout(view);
	QHBoxLayout *controls = new QHBoxLayout;

	QLabel *ltable = new QLabel(view);
	ltable->setText(_("Table:"));

	QComboBox *cbtables = new QComboBox(view);

	QLabel *lquery = new QLabel(view);
	lquery->setAlignment(Qt::AlignCenter);

	QPushButton *bquery = new QPushButton(view);
	bquery->setText(_("Query"));

	controls->addWidget(ltable);
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

	QObject::connect(cbtables, QOverload<int>::of(&QComboBox::currentIndexChanged), [cbtables, table, lquery, type, conname](int x)
	{
		QString strquery;

		if (type.name() == "application/vnd.sqlite3")
			strquery = QString("SELECT * FROM %1;").arg(cbtables->currentText());

		if (!strquery.isEmpty())
		{
			lquery->setText(strquery);
			fill_table(table, strquery, conname);
		}
	});

	QObject::connect(bquery, &QPushButton::clicked, [cbtables, table, lquery, conname](int x)
	{
		bool ret;
		QString strquery = QInputDialog::getText((QWidget*)table, "",
		                   _("Please enter a new query and press OK to execute it"), QLineEdit::Normal, lquery->text(), &ret);

		if (ret && !strquery.isEmpty())
		{
			lquery->setText(strquery);
			fill_table(table, strquery, conname);
		}
	});

	QStringList tlist = dbase.tables();

	if (tlist.isEmpty())
		QMessageBox::warning((QWidget*)ParentWin, "", _("Failed to fetch list of tables. Maybe DB is locked?"));

	for (int i = 0; i < tlist.count(); i++)
		cbtables->addItem(tlist[i]);


	view->setProperty("base", conname);

	view->show();

	return view;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QFrame *view = (QFrame*)ListWin;
	QString name = view->property("base").value<QString>();

	if (view);

	{
		QSqlDatabase dbase = QSqlDatabase::database(name);

		if (dbase.isValid() && dbase.isOpen())
			dbase.close();

		delete view;
	}

	if (!name.isEmpty())
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

		if (needle != prev)
			i = 0;
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

	QMessageBox::information(view, "", QString::asprintf(_("\"%s\" not found!"), SearchString));

	return LISTPLUGIN_ERROR;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	static char plg_path[PATH_MAX];
	const char* loc_dir = "langs";

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(plg_path, &dlinfo) != 0)
	{
		strncpy(plg_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(plg_path, '/');

		if (pos)
			strcpy(pos + 1, loc_dir);

		setlocale(LC_ALL, "");
		bindtextdomain(GETTEXT_PACKAGE, plg_path);
		textdomain(GETTEXT_PACKAGE);
	}
}
