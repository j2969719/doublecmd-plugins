#include <QFile>
#include <QTextCodec>
#include <QTableWidget>
#include <QHeaderView>

#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QSettings>
#include <QMimeData>

#include <QMessageBox>

#include <enca.h>

#include <dlfcn.h>
#include <libintl.h>
#include <locale.h>
#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

#include "wlxplugin.h"

static bool g_enca = true;
static bool g_resize = false;
static bool g_readall = false;
static bool g_quoted = true;
static QString g_lang;

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	char separator;
	char codec_name[13];
	int columns, row = 0;
	QStringList header, list;
	QFile file(FileToLoad);
	QByteArray line;

	if (!file.open(QFile::ReadOnly | QFile::Text))
		return nullptr;

	if (g_enca)
	{

		if (g_readall)
			line = file.readAll();
		else
			line = file.read(4096);

		EncaAnalyser analyser;
		EncaEncoding encoding;
		analyser = enca_analyser_alloc(g_lang.toStdString().c_str());

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
	}

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
		return nullptr;
	}

	if (g_resize)
		view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	while (!file.atEnd())
	{
		view->insertRow(row);
		line = file.readLine();

		QStringList rawlist;

		if (codec)
			rawlist = codec->toUnicode(line).split(QLatin1Char(separator));
		else
			rawlist = QString(line).split(QLatin1Char(separator));

		list.clear();

		if (!g_quoted)
			list = rawlist;
		else
		{
			for (int c = 0; c < rawlist.size(); ++c)
			{
				const QString itm = rawlist.at(c);

				if (!itm.isEmpty() && itm.front() == '"')
				{
					QString temp(itm.trimmed());

					if (itm.back() == '"' && itm.count(QLatin1Char('"')) % 2 == 0)
						temp = QString(itm).remove(0, 1).remove(-1, 1);
					else
					{
						for (int x = c + 1; x < rawlist.size(); x++)
						{
							const QString nitm = rawlist.at(x);

							if (!nitm.isEmpty() && nitm.back() == '"')
							{
								temp = rawlist.mid(c, x - c + 1).join(QLatin1Char(separator)).remove(0, 1).remove(-1, 1);

								if (temp.count(QLatin1Char('"')) % 2 == 0)
								{
									c = x;
									break;
								}
							}
						}
					}

					list.append(temp);
				}
				else
					list.append(rawlist.at(c).trimmed());

				list.last().replace("\"\"", "\"");
			}
		}

		if (list.size() > columns)
		{
			columns = list.size();
			view->setColumnCount(columns);
		}

		for (int c = 0; c < list.size(); ++c)
		{
			QTableWidgetItem *item = new QTableWidgetItem(list.at(c).trimmed());
			item->setToolTip(list.at(c).trimmed());
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			view->setItem(row, c, item);
		}

		row++;
	}

	file.close();

	view->setSortingEnabled(true);

	view->show();

	return view;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QTableWidget *view = (QTableWidget*)ListWin;
	delete view;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	QTableWidget *view = (QTableWidget*)ListWin;

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
	QTableWidget *view = (QTableWidget*)ListWin;

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

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	snprintf(DetectString, maxlen - 1, "EXT=\"CSV\"|EXT=\"TSV\"");
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	QFileInfo defini(QString::fromStdString(dps->DefaultIniName));
	QString cfgpath = defini.absolutePath() + "/j2969719.ini";
	QSettings settings(cfgpath, QSettings::IniFormat);

	if (!settings.contains("csvviewer/resize_columns"))
		settings.setValue("csvviewer/resize_columns", false);
	else
		g_resize = settings.value("csvviewer/resize_columns").toBool();

	if (!settings.contains("csvviewer/enca"))
		settings.setValue("csvviewer/enca", true);
	else
		g_enca = settings.value("csvviewer/enca").toBool();

	if (!settings.contains("csvviewer/enca_lang"))
	{
		char lang[3];
		snprintf(lang, 3, "%s", setlocale(LC_ALL, ""));
		settings.setValue("csvviewer/enca_lang", lang);
	}
	else
		g_lang = settings.value("csvviewer/enca_lang").toString();

	if (!settings.contains("csvviewer/enca_readall"))
		settings.setValue("csvviewer/enca_readall", false);
	else
		g_readall = settings.value("csvviewer/enca_readall").toBool();

	if (!settings.contains("csvviewer/doublequoted"))
		settings.setValue("csvviewer/doublequoted", true);
	else
		g_quoted = settings.value("csvviewer/doublequoted").toBool();

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
