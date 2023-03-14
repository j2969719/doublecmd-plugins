#include <dlfcn.h>
#include <QtQuick>
#include <QtWidgets>
#include <QSettings>
#include <QQuickWidget>
#include <QQmlContext>
#include "wlxplugin.h"

#define TMPDIRNAME "/tmp/_dc-imgviewcrap.XXXXXX"
#define INFILE "$FILE"
#define OUTFILE "$PNG"
#define OUTFILENAME "output.png"

static QString gQmlFile;
static QString gDefCfg;
static QString gPlugDir;
static QString gSettings;
QMimeDatabase gDB;

static QString GetOutputFile(char* FileToLoad, QString tmpdir)
{
	QProcess proc;
	QString cmd, key;

	QString filename = QString(FileToLoad);

	QFileInfo fi(filename);

	if (!fi.exists())
		return nullptr;

	QSettings settings(gSettings, QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	key = QString("%1/command").arg(fi.suffix().toLower());

	if (settings.contains(key))
		cmd = QString::fromUtf8(settings.value(key).toByteArray());
	else
	{
		QMimeType type = gDB.mimeTypeForFile(filename);
		key = QString("%1/command").arg(type.name());

		if (settings.contains(key))
			cmd = QString::fromUtf8(settings.value(key).toByteArray());
	}

	if (cmd.isEmpty())
		return nullptr;

	QString out = QString("%1/" OUTFILENAME).arg(tmpdir);

	QString quoted = filename.replace("\'", "\\\'");
	quoted.prepend("\'");
	quoted.append("\'");
	cmd.replace(INFILE, quoted);
	cmd.replace(OUTFILE, out);

	QStringList params;
	params << "-c" << cmd;


	proc.start("/bin/sh", params);
	proc.waitForFinished();

	if (proc.exitStatus() != QProcess::NormalExit)
		return nullptr;

	QFileInfo fiout(out);

	if (!fiout.exists())
		return nullptr;

	return out;

}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QTemporaryDir tmpdir(TMPDIRNAME);
	tmpdir.setAutoRemove(false);
	QString out = GetOutputFile(FileToLoad, tmpdir.path());

	if (out.isNull())
	{
		QDir dir(tmpdir.path());
		dir.removeRecursively();
		return nullptr;
	}

	QQuickWidget *view = new QQuickWidget((QWidget*)ParentWin);
	QQmlContext *ctx = view->rootContext();
	ctx->setContextProperty("parent", (QWidget*)ParentWin);
	ctx->setContextProperty("default_cfg", gDefCfg);
	ctx->setContextProperty("plugin_dir", gPlugDir);

	ctx->setContextProperty("lc_copy", lc_copy);
	ctx->setContextProperty("lc_newparams", lc_newparams);
	ctx->setContextProperty("lc_selectall", lc_selectall);
	ctx->setContextProperty("lc_setpercent", lc_setpercent);

	ctx->setContextProperty("lcp_wraptext", lcp_wraptext);
	ctx->setContextProperty("lcp_fittowindow", lcp_fittowindow);
	ctx->setContextProperty("lcp_ansi", lcp_ansi);
	ctx->setContextProperty("lcp_ascii", lcp_ascii);
	ctx->setContextProperty("lcp_variable", lcp_variable);
	ctx->setContextProperty("lcp_forceshow", lcp_forceshow);
	ctx->setContextProperty("lcp_fitlargeronly", lcp_fitlargeronly);
	ctx->setContextProperty("lcp_center", lcp_center);

	ctx->setContextProperty("lcs_findfirst", lcs_findfirst);
	ctx->setContextProperty("lcs_matchcase", lcs_matchcase);
	ctx->setContextProperty("lcs_wholewords", lcs_wholewords);
	ctx->setContextProperty("lcs_backwards", lcs_backwards);

	view->setSource(QUrl::fromLocalFile(gQmlFile));
	view->setResizeMode(QQuickWidget::SizeRootObjectToView);;
	view->show();

	QVariant ret;
	QObject *object = qobject_cast<QObject *>(view->rootObject());
	QMetaObject::invokeMethod(object, "myListLoad",
	                          Q_RETURN_ARG(QVariant, ret),
	                          Q_ARG(QVariant, out),
	                          Q_ARG(QVariant, ShowFlags));

	if (!ret.toBool())
	{
		QDir dir(tmpdir.path());
		dir.removeRecursively();
		delete view;
		return nullptr;
	}

	view->setProperty("tmpdir", tmpdir.path());

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QQuickWidget *view = (QQuickWidget*)PluginWin;
	QString oldtmpdir = view->property("tmpdir").value<QString>();
	QDir dir(oldtmpdir);
	dir.removeRecursively();
	QTemporaryDir tmpdir(TMPDIRNAME);
	tmpdir.setAutoRemove(false);
	view->setProperty("tmpdir", tmpdir.path());
	QString out = GetOutputFile(FileToLoad, tmpdir.path());

	if (out.isNull())
		return LISTPLUGIN_ERROR;

	QVariant ret;
	QMimeType type = gDB.mimeTypeForFile(QString(FileToLoad));

	QObject *object = qobject_cast<QObject *>(view->rootObject());
	QMetaObject::invokeMethod(object, "myListLoadNext",
	                          Q_RETURN_ARG(QVariant, ret),
	                          Q_ARG(QVariant, out),
	                          Q_ARG(QVariant, ShowFlags));

	if (ret.toBool())
		return LISTPLUGIN_OK;

	return LISTPLUGIN_ERROR;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QVariant ret;
	QQuickWidget *view = (QQuickWidget*)ListWin;
	QObject *object = qobject_cast<QObject *>(view->rootObject());
	QMetaObject::invokeMethod(object, "myListCloseWindow");

	QString oldtmpdir = view->property("tmpdir").value<QString>();
	QDir dir(oldtmpdir);
	dir.removeRecursively();

	delete object;
	delete view;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	QVariant ret;
	QQuickWidget *view = (QQuickWidget*)ListWin;
	QObject *object = qobject_cast<QObject *>(view->rootObject());
	QMetaObject::invokeMethod(object, "myListSendCommand",
	                          Q_RETURN_ARG(QVariant, ret),
	                          Q_ARG(QVariant, Command),
	                          Q_ARG(QVariant, Parameter));

	if (ret.toBool())
		return LISTPLUGIN_OK;

	return LISTPLUGIN_ERROR;
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	snprintf(DetectString, maxlen - 1, DETECT_STRING);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	QVariant ret;
	QQuickWidget *view = (QQuickWidget*)ListWin;
	QObject *object = qobject_cast<QObject *>(view->rootObject());
	QMetaObject::invokeMethod(object, "myListSearchDialog",
	                          Q_RETURN_ARG(QVariant, ret),
	                          Q_ARG(QVariant, FindNext));

	if (ret.toBool())
		return LISTPLUGIN_OK;

	return LISTPLUGIN_ERROR;
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	QVariant ret;
	QQuickWidget *view = (QQuickWidget*)ListWin;
	QObject *object = qobject_cast<QObject *>(view->rootObject());
	QMetaObject::invokeMethod(object, "myListSearchText",
	                          Q_RETURN_ARG(QVariant, ret),
	                          Q_ARG(QVariant, SearchString),
	                          Q_ARG(QVariant, SearchParameter));

	if (ret.toBool())
		return LISTPLUGIN_OK;

	return LISTPLUGIN_ERROR;
}

int DCPCALL ListPrint(HWND ListWin, char* FileToPrint, char* DefPrinter, int PrintFlags, RECT* Margins)
{
	QVariant ret;
	QRect rect = QRect((int)Margins->left, (int)Margins->top,
	                   (int)Margins->right, (int)Margins->bottom);
	QQuickWidget *view = (QQuickWidget*)ListWin;
	QObject *object = qobject_cast<QObject *>(view->rootObject());
	QMetaObject::invokeMethod(object, "myListPrint",
	                          Q_RETURN_ARG(QVariant, ret),
	                          Q_ARG(QVariant, FileToPrint),
	                          Q_ARG(QVariant, DefPrinter),
	                          Q_ARG(QVariant, PrintFlags),
	                          Q_ARG(QVariant, rect));

	if (ret.toBool())
		return LISTPLUGIN_OK;

	return LISTPLUGIN_ERROR;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	if (gQmlFile.isEmpty() and gPlugDir.isEmpty())
	{
		Dl_info dlinfo;
		static char path[PATH_MAX];
		const char* qml = "plugin.qml";
		memset(&dlinfo, 0, sizeof(dlinfo));

		if (dladdr(path, &dlinfo) != 0)
		{
			strncpy(path, dlinfo.dli_fname, PATH_MAX);
			char *pos = strrchr(path, '/');

			if (pos)
			{
				strcpy(pos + 1, qml);
				gQmlFile = QString(path);
				*pos = '\0';
				gPlugDir = QString(path);
				gSettings = QString("/settings.ini").prepend(gPlugDir);
			}
		}
	}

	gDefCfg = QString(dps->DefaultIniName);
}
