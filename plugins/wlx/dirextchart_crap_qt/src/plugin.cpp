#include <QtCharts>
#include <QtWidgets>
#include <QtConcurrent>

#include <dlfcn.h>
#include "wlxplugin.h"

static QMimeDatabase gDB;
static char inipath[PATH_MAX];

static QMap<QString, qint64> CalcContentSize(QString path)
{
	QMap<QString, qint64> result;

	QSettings settings(inipath, QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	QDirIterator iter(path, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);

	while (iter.hasNext())
	{
		iter.next();

		if (iter.fileInfo().isSymLink())
			continue;

		bool found = false;

		for (const auto& i : settings.allKeys())
		{
			if (settings.value(i).value<QStringList>().contains(iter.fileInfo().suffix()))
			{
				result[i]  += iter.fileInfo().size();
				found = true;
			}
		}

		if (!found)
			result["other..."]  += iter.fileInfo().size();
	}

	return result;
}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QMimeType type = gDB.mimeTypeForFile(QString(FileToLoad));

	if (type.name() != "inode/directory")
		return nullptr;

	QFrame *view = new QFrame((QWidget*)ParentWin);
	view->setFrameStyle(QFrame::NoFrame);
	QVBoxLayout *main = new QVBoxLayout(view);
	QHBoxLayout *controls = new QHBoxLayout;

	QLineEdit *lpath = new QLineEdit(view);
	lpath->setReadOnly(true);
	lpath->hide();
	lpath->setObjectName("pathlabel");
	controls->addWidget(lpath);
	main->addLayout(controls);

	QChartView *chart = new QChartView(view);
	QPieSeries *series = new QPieSeries(view);
	chart->chart()->legend()->setAlignment(Qt::AlignRight);
	chart->chart()->addSeries(series);

	main->addWidget(chart);

	QObject::connect(lpath, &QLineEdit::textChanged, [series](const QString text)
	{
		QLocale locale;
		auto rows = QtConcurrent::run(CalcContentSize, text);
		QMapIterator<QString, qint64> i(rows);

		series->clear();

		while (i.hasNext())
		{
			i.next();
			series->append(QString("%1 (%2)").arg(i.key()).arg(locale.formattedDataSize(i.value())), i.value());
		}
	});

	view->show();

	lpath->setText(QString(FileToLoad));

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QMimeType type = gDB.mimeTypeForFile(QString(FileToLoad));

	if (type.name() != "inode/directory")
		return LISTPLUGIN_ERROR;

	QFrame *view = (QFrame*)ParentWin;
	QLineEdit *lpath = view->findChild<QLineEdit*>("pathlabel");
	lpath->setText(QString(FileToLoad));

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QFrame *view = (QFrame*)ListWin;
	delete view;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(inipath, &dlinfo) != 0)
	{
		QFileInfo plugnfo(QString(dlinfo.dli_fname));
		snprintf(inipath, PATH_MAX, "%s/settings.ini", plugnfo.absolutePath().toStdString().c_str());
	}
}
