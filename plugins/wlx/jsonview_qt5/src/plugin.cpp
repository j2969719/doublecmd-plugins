#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QMimeData>
#include <QtWidgets>
#include <QClipboard>
#include <QApplication>
#include "wlxplugin.h"

static int  g_width = 200;
static bool g_resize = false;
static bool g_expand = true;
static void check_value(const QJsonValue value, QTreeWidgetItem *item);

static void walk_array(const QJsonArray array, QTreeWidgetItem *item)
{
	for (int i = 0; i < array.count(); i++)
	{
		QTreeWidgetItem *newitem = new QTreeWidgetItem(item);
		newitem->setText(0, QString("[%1]").arg(i));
		check_value(array.at(i), newitem);
	}
}

static void walk_object(const QJsonObject object, QTreeWidgetItem *item)
{
	QJsonObject::const_iterator iter;

	for (iter = object.constBegin(); iter != object.constEnd(); ++iter)
	{
		QTreeWidgetItem *newitem = new QTreeWidgetItem(item);
		newitem->setText(0, iter.key());
		newitem->setToolTip(0, iter.key());
		check_value(iter.value(), newitem);
	}
}

static void check_value(const QJsonValue value, QTreeWidgetItem *item)
{
	switch (value.type())
	{
	case QJsonValue::Object:
	{
		item->setText(2, "Object");
		walk_object(value.toObject(), item);
		break;
	}

	case QJsonValue::Array:
	{
		item->setText(2, "Array");
		walk_array(value.toArray(), item);
		break;
	}

	case QJsonValue::String:
	{
		item->setText(2, "String");
		item->setText(1, value.toString());
		item->setToolTip(1, value.toString());
		break;
	}

	case QJsonValue::Double:
	{
		item->setText(2, "Double");
		item->setText(1, QString::number(value.toDouble()));
		item->setToolTip(1, QString::number(value.toDouble()));
		break;
	}

	case QJsonValue::Bool:
	{
		item->setText(2, "Boolean");

		if (value.toBool())
			item->setText(1, "True");
		else
			item->setText(1, "False");

		break;
	}

	case QJsonValue::Null:
	{
		item->setText(2, "Null");
		break;
	}

	default:
	{
		item->setText(2, "Undefined");
		break;
	}
	}
}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QMimeDatabase db;
	QMimeType type = db.mimeTypeForFile(QString(FileToLoad));

	if (type.name() != "text/plain" && type.name() != "application/json")
		return nullptr;

	QFile file(FileToLoad);

	if (!file.open(QFile::ReadOnly | QFile::Text))
		return nullptr;

	QJsonDocument json = QJsonDocument().fromJson(file.readAll());
	file.close();

	if (json.isNull() || json.isEmpty())
		return nullptr;

	QFileInfo fi(FileToLoad);
	QTreeWidget *view = new QTreeWidget((QWidget*)ParentWin);
	view->setColumnCount(3);

	QTreeWidgetItem *root = new QTreeWidgetItem(view);
	root->setText(0, fi.fileName());
	root->setToolTip(0, fi.fileName());

	if (json.isObject())
	{
		root->setText(2, "Object");
		walk_object(json.object(), root);
	}
	else if (json.isArray())
	{
		root->setText(2, "Array");
		walk_array(json.array(), root);
	}

	view->insertTopLevelItem(0, root);

	if (g_expand)
		view->expandAll();

	for (int i = 0; i < 3; i++)
	{
		if (g_resize)
			view->resizeColumnToContents(i);
		else
			view->setColumnWidth(i, g_width);
	}

	QStringList headers;
	headers << "Key" << "Value" << "Type";
	view->setHeaderLabels(headers);

	view->setSelectionMode(QAbstractItemView::SingleSelection);
	view->setSelectionBehavior(QAbstractItemView::SelectItems);
	view->setSortingEnabled(true);

	view->show();

	return view;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QTreeWidget *view = (QTreeWidget*)ListWin;
	view->~QTreeWidget();
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	QTreeWidget *view = (QTreeWidget*)ListWin;

	if (Command == lc_copy)
	{
		QString text(view->currentItem()->text(view->currentColumn()));

		if (!text.isEmpty())
			QApplication::clipboard()->setText(text);

		return LISTPLUGIN_OK;
	}

	return LISTPLUGIN_ERROR;
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	QList<QTreeWidgetItem*> list;
	QTreeWidget *view = (QTreeWidget*)ListWin;

	Qt::MatchFlags sflags = Qt::MatchContains | Qt::MatchRecursive;

	if (SearchParameter & lcs_matchcase)
		sflags |= Qt::MatchCaseSensitive;

	QString needle(SearchString);
	QString prev = view->property("needle").value<QString>();
	view->setProperty("needle", needle);

	list = view->findItems(QString(SearchString), sflags, view->currentColumn());

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
			view->setCurrentItem(list.at(i), view->currentColumn());
		}

		view->setProperty("findit", i);

		return LISTPLUGIN_OK;
	}

	return LISTPLUGIN_ERROR;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	QFileInfo defini(QString::fromStdString(dps->DefaultIniName));
	QString cfgpath = defini.absolutePath() + "/j2969719.ini";
	QSettings settings(cfgpath, QSettings::IniFormat);

	if (!settings.contains("jsonview/resize_columns"))
		settings.setValue("jsonview/resize_columns", false);
	else
		g_resize = settings.value("jsonview/resize_columns").toBool();

	if (!settings.contains("jsonview/tree_expand"))
		settings.setValue("jsonview/tree_expand", true);
	else
		g_expand = settings.value("jsonview/tree_expand").toBool();

	if (!settings.contains("jsonview/column_width"))
		settings.setValue("jsonview/column_width", 200);
	else
	{
		g_width = settings.value("jsonview/column_width").toInt();

		if (g_width < 10)
		{
			g_width = 10;
			settings.setValue("jsonview/column_width", 10);
		}
	}
}
