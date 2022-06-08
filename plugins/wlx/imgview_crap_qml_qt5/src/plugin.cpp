#include <dlfcn.h>
#include <QtQuick>
#include <QtWidgets>
#include <QQuickWidget>
#include <QQmlContext>
#include "wlxplugin.h"

QString gQmlFile;

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QQuickWidget *view = new QQuickWidget((QWidget*)ParentWin);
	QQmlContext *ctx = view->rootContext();
	ctx->setContextProperty("parent", (QWidget*)ParentWin);
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
	QMetaObject::invokeMethod(object, "myListLoad", 
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

	return LISTPLUGIN_OK;
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

	return LISTPLUGIN_OK;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
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
			strcpy(pos + 1, qml);
		gQmlFile = QString(path);
	}
}
