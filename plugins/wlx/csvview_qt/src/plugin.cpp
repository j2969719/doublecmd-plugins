#include <QFile>
#include <QTableWidget>
#include <QHeaderView>

#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QSettings>
#include <QMimeData>

#include <QMessageBox>

#include <glib.h>
#include <enca.h>

#include <dlfcn.h>
#include <libintl.h>
#include <locale.h>
#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

#include "wlxplugin.h"

static bool gEnca = true;
static bool gResize = false;
static bool gReadAll = false;
static bool gQuoted = true;
static bool gGrid = false;
static QString gLang;


static QStringList parse_line(QByteArray line, char *enc, char separator)
{
	QStringList list, rawlist;

	if (enc[0] != '\0')
	{
		gsize len;
		gchar *converted = g_convert_with_fallback(line.data(), (gsize)line.size(), "UTF-8", enc, NULL, NULL, &len, NULL);

		if (converted)
			rawlist = QString(converted).split(QLatin1Char(separator));

		g_free(converted);
	}
	else
		rawlist = QString(line).split(QLatin1Char(separator));

	if (rawlist.isEmpty())
		return rawlist;

	if (rawlist.last().back() == '\n')
		rawlist.last().remove(-1, 1);

	if (!gQuoted || separator == '\t')
		list = rawlist;
	else
	{
		for (int c = 0; c < rawlist.size(); ++c)
		{
			const QString itm = rawlist.at(c);

			if (!itm.isEmpty() && itm.front() == '"')
			{
				QString temp(itm.trimmed());

				if (itm.back() == '"' && itm.count(QLatin1Char('"')) > 3 && itm.count(QLatin1Char('"')) % 2 == 0)
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

	return list;
}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	char separator;
	char enc[256] = "";
	int columns, row = 0;
	QStringList header, list;
	QFile file(FileToLoad);
	QByteArray line;

	if (!file.open(QFile::ReadOnly | QFile::Text))
		return nullptr;

	if (gEnca)
	{
		if (gReadAll)
			line = file.readAll();
		else
			line = file.read(4096);

		EncaAnalyser analyser;
		EncaEncoding encoding;
		analyser = enca_analyser_alloc(gLang.toStdString().c_str());

		if (analyser)
		{
			enca_set_threshold(analyser, 1.38);
			enca_set_multibyte(analyser, 1);
			enca_set_ambiguity(analyser, 1);
			enca_set_garbage_test(analyser, 1);
			enca_set_filtering(analyser, 0);
			encoding = enca_analyse(analyser, (unsigned char*)line.data(), (size_t)line.size());

			if (encoding.charset > 0 && encoding.charset != 27)
				snprintf(enc, sizeof(enc), "%s", enca_charset_name(encoding.charset, ENCA_NAME_STYLE_ICONV));

			enca_analyser_free(analyser);
		}


		file.seek(0);
	}

	line = file.readLine();
	QByteArray seps(",;\t");

	QTableWidget *view = new QTableWidget((QWidget*)ParentWin);

	for (int i = 0; i < seps.size(); ++i)
	{
		separator = seps.at(i);

		header = parse_line(line, enc, separator);
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

	if (gResize)
		view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	while (!file.atEnd())
	{
		view->insertRow(row);
		list = parse_line(file.readLine(), enc, separator);

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
	view->setShowGrid(gGrid);

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

	QMessageBox::information(view, "", QString::asprintf(_("\"%s\" not found!"), SearchString));

	return LISTPLUGIN_ERROR;
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	snprintf(DetectString, maxlen - 1, "(EXT=\"CSV\" | EXT=\"TSV\") & SIZE<30000000");
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	QFileInfo defini(QString::fromStdString(dps->DefaultIniName));
	QString cfgpath = defini.absolutePath() + "/j2969719.ini";
	QSettings settings(cfgpath, QSettings::IniFormat);

	if (!settings.contains(PLUGNAME "/resize_columns"))
		settings.setValue(PLUGNAME "/resize_columns", gResize);
	else
		gResize = settings.value(PLUGNAME "/resize_columns").toBool();

	if (!settings.contains(PLUGNAME "/enca"))
		settings.setValue(PLUGNAME "/enca", gEnca);
	else
		gEnca = settings.value(PLUGNAME "/enca").toBool();

	if (!settings.contains(PLUGNAME "/enca_lang"))
	{
		char lang[3];
		snprintf(lang, 3, "%s", setlocale(LC_ALL, ""));
		settings.setValue(PLUGNAME "/enca_lang", QString(lang));
	}
	else
		gLang = settings.value(PLUGNAME "/enca_lang").toString();

	if (!settings.contains(PLUGNAME "/enca_readall"))
		settings.setValue(PLUGNAME "/enca_readall", gReadAll);
	else
		gReadAll = settings.value(PLUGNAME "/enca_readall").toBool();

	if (!settings.contains(PLUGNAME "/doublequoted"))
		settings.setValue(PLUGNAME "/doublequoted", gQuoted);
	else
		gQuoted = settings.value(PLUGNAME "/doublequoted").toBool();

	if (!settings.contains(PLUGNAME "/draw_grid"))
		settings.setValue(PLUGNAME "/draw_grid", gGrid);
	else
		gGrid = settings.value(PLUGNAME "/draw_grid").toBool();

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
