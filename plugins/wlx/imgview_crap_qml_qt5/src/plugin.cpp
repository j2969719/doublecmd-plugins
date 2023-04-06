#include <dlfcn.h>
#include <QtQuick>
#include <QtWidgets>
#include <QQuickWidget>
#include <QQmlContext>
#include "wlxplugin.h"

static QString gQmlFile;
static QString gDefCfg;
static QString gPlugDir;

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QWidget *parent = (QWidget*)ParentWin;

	while (parent -> parentWidget() != Q_NULLPTR)
		parent = parent->parentWidget();

	bool quickview = false;

	if (parent->windowRole() != "TfrmViewer")
		quickview = true;

	QQuickWidget *view = new QQuickWidget((QWidget*)ParentWin);
	QQmlContext *ctx = view->rootContext();
	ctx->setContextProperty("parent", (QWidget*)ParentWin);
	ctx->setContextProperty("default_cfg", gDefCfg);
	ctx->setContextProperty("plugin_dir", gPlugDir);
	ctx->setContextProperty("quickview", quickview);

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
	                          Q_ARG(QVariant, FileToLoad),
	                          Q_ARG(QVariant, ShowFlags));

	if (!ret.toBool())
	{
		delete view;
		return nullptr;
	}

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QVariant ret;
	QQuickWidget *view = (QQuickWidget*)PluginWin;
	QObject *object = qobject_cast<QObject *>(view->rootObject());
	QMetaObject::invokeMethod(object, "myListLoadNext",
	                          Q_RETURN_ARG(QVariant, ret),
	                          Q_ARG(QVariant, FileToLoad),
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
			}
		}
	}

	gDefCfg = QString(dps->DefaultIniName);
}
