#include <QLabel>
#include <QApplication>
#include <QClipboard>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "wlxplugin.h"

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	int errsv;
	ssize_t len;
	struct stat buf;
	char path[PATH_MAX];
	QString text;

	if (lstat(FileToLoad, &buf) != 0 || !S_ISLNK(buf.st_mode))
		return nullptr;

	if (access(FileToLoad, F_OK) == -1)
	{
		errsv = errno;
	}
	else
		return nullptr;


	if ((len = readlink(FileToLoad, path, sizeof(path) - 1)) != -1)
	{
		char real_path[PATH_MAX] = "";

		if (!realpath(FileToLoad, real_path))
		{
			int errrp = errno;

			if (errrp != ENOENT)
				real_path[0] = '\0';
		}

		path[len] = '\0';
		text = QString::asprintf("%s -> %s\n%s%s%s", FileToLoad, path, real_path, (real_path[0] == '\0') ? "" : ": ", strerror(errsv));
	}
	else
		return NULL;


	QLabel *view = new QLabel((QWidget*)ParentWin);
	view->setText(text);
	view->setAlignment(Qt::AlignCenter);
	view->setTextInteractionFlags(Qt::TextSelectableByMouse);
	QFont monofont;
	monofont.setFamily("mono");
	monofont.setPointSize(13);
	view->setFont(monofont);
	view->show();

	return view;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	delete (QLabel*)ListWin;
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	QLabel *view = (QLabel*)ListWin;

	if (Command == lc_copy)
		QApplication::clipboard()->setText(view->selectedText());

	return LISTPLUGIN_OK;
}
