#include <QtWidgets>
#include <QtConcurrent>
#include <QPlainTextEdit>
#include <QMessageBox>

#include <archive.h>
#include <archive_entry.h>

#include <dlfcn.h>
#include <libintl.h>
#include <locale.h>
#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

#include "wlxplugin.h"

QFont gFont;

static bool probe_file(char *filename)
{
	bool result = true;
	struct archive *a = archive_read_new();
	archive_read_support_filter_all(a);
	archive_read_support_format_raw(a);

	int r = archive_read_open_filename(a, filename, 10240);

	if (r != ARCHIVE_OK)
		result = false;

	archive_read_close(a);
	archive_read_free(a);

	return result;
}

static QString open_file(char *filename)
{
	QString result;
	size_t size;
	const void *buff;
	la_int64_t offset;


	struct archive *a = archive_read_new();
	archive_read_support_filter_all(a);
	archive_read_support_format_raw(a);
	struct archive_entry *entry;

	int r = archive_read_open_filename(a, filename, 10240);

	if (r != ARCHIVE_OK)
	{
		archive_read_close(a);
		archive_read_free(a);
		QMessageBox::critical(nullptr, "", QString::asprintf(_("libarchive: failed to read %s"), filename));
		return nullptr;
	}

	if ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK || r == ARCHIVE_WARN)
	{
		while ((r = archive_read_data_block(a, &buff, &size, &offset)) != ARCHIVE_EOF)
		{
			result.append(QByteArray((const char*)buff, (int)size));

			if (r < ARCHIVE_OK)
			{
				QMessageBox::critical(nullptr, "", QString::asprintf("libarchive: %s", archive_error_string(a)));
				break;
			}
		}
	}

	if (r != ARCHIVE_EOF)
		QMessageBox::critical(nullptr, "", QString::asprintf("libarchive: %s", archive_error_string(a)));

	archive_read_close(a);
	archive_read_free(a);

	return result;
}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	if (!probe_file(FileToLoad))
		return nullptr;

	QPlainTextEdit *view = new QPlainTextEdit((QWidget*)ParentWin);
	QFutureWatcher<QString> *watcher = new QFutureWatcher<QString>(view);
	watcher->setObjectName("watcher");
	
	QObject::connect(watcher, &QFutureWatcher<QString>::finished, [watcher, view]()
	{
		auto result = watcher->result();
		view->setPlainText(result);
	});

	watcher->setFuture(QtConcurrent::run(open_file, FileToLoad));
	view->document()->setDefaultFont(gFont);
	view->setReadOnly(true);

	if (ShowFlags & lcp_wraptext)
		view->setLineWrapMode(QPlainTextEdit::WidgetWidth);
	else
		view->setLineWrapMode(QPlainTextEdit::NoWrap);

	view->show();

	return view;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QPlainTextEdit *view = (QPlainTextEdit*)ListWin;
	QFutureWatcher<QString> *watcher = view->findChild<QFutureWatcher<QString>*>("watcher");

	if (watcher->isRunning())
		watcher->waitForFinished();

	delete watcher;
	delete (QPlainTextEdit*)ListWin;
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	QTextDocument::FindFlags sflags;

	if (SearchParameter & lcs_matchcase)
		sflags |= QTextDocument::FindCaseSensitively;

	if (SearchParameter & lcs_backwards)
		sflags |= QTextDocument::FindBackward;

	QPlainTextEdit *view = (QPlainTextEdit*)ListWin;

	if (SearchParameter & lcs_findfirst)
	{
		QTextCursor cursor = view->textCursor();
		cursor.movePosition(SearchParameter & lcs_backwards ? QTextCursor::End : QTextCursor::Start);
		view->setTextCursor(cursor);
	}

	if (view->find(SearchString, sflags))
		return LISTPLUGIN_OK;
	else
		QMessageBox::information(view, "", QString::asprintf(_("\"%s\" not found!"), SearchString));

	return LISTPLUGIN_ERROR;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	QPlainTextEdit *view = (QPlainTextEdit*)ListWin;

	switch (Command)
	{
	case lc_copy :
		view->copy();
		break;

	case lc_selectall :
		view->selectAll();
		break;

	case lc_newparams :
		if (Parameter & lcp_wraptext)
			view->setLineWrapMode(QPlainTextEdit::WidgetWidth);
		else
			view->setLineWrapMode(QPlainTextEdit::NoWrap);

		break;

	default :
		return LISTPLUGIN_ERROR;
	}

	return LISTPLUGIN_OK;
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	snprintf(DetectString, maxlen - 1, "%s", DETECT_STRING);
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

	QFileInfo defini(QString::fromStdString(dps->DefaultIniName));
	QString cfgpath = defini.absolutePath() + "/j2969719.ini";
	QSettings settings(cfgpath, QSettings::IniFormat);
	int size = settings.value(PLUGNAME "/fontsize").toInt();

	if (size == 0)
	{
		//gFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
		gFont.setFamily("mono");
		size = 12;
		settings.setValue(PLUGNAME "/fontsize", 12);
		settings.setValue(PLUGNAME "/font", gFont.family());
		settings.setValue(PLUGNAME "/fontbold", gFont.bold());
	}

	gFont.setFixedPitch(true);
	gFont.setPointSize(size);
	gFont.setFamily(settings.value(PLUGNAME "/font").toString());
	gFont.setBold(settings.value(PLUGNAME "/fontbold").toBool());
}
