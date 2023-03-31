#include <QtCharts>
#include <QtWidgets>
#include <QtConcurrent>

#include <dlfcn.h>
#include "wlxplugin.h"

static char inipath[PATH_MAX];

static QMap<QString, qint64> CalcContentSize(QString path)
{
	qDebug() << "CalcContentSize";
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
			result["..."]  += iter.fileInfo().size();
	}

	qDebug() << result;
	return result;
}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QFileInfo fi(FileToLoad);

	if (!fi.isDir())
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
	chart->chart()->addSeries(series);
	QPalette palette = QApplication::palette();
	chart->chart()->setBackgroundBrush(palette.brush(QPalette::Window));
	chart->chart()->setTitleBrush(palette.brush(QPalette::WindowText));
	chart->chart()->legend()->setAlignment(Qt::AlignRight);
	chart->chart()->legend()->setLabelColor(palette.color(QPalette::WindowText));
	main->addWidget(chart);

	QFutureWatcher<QMap<QString, qint64>> *watcher = new QFutureWatcher<QMap<QString, qint64>>(view);
	watcher->setObjectName("watcher");

	QObject::connect(watcher, &QFutureWatcher<QMap<QString, qint64>>::finished, [lpath, watcher, series, chart]()
	{
		QLocale locale;
		auto result = watcher->result();
		QMapIterator<QString, qint64> iter(result);

		series->clear();

		qint64 total = 0;
		qint64 maxsize = 0;
		QStringList list;

		while (iter.hasNext())
		{
			iter.next();
			total += iter.value();

			if (iter.key() != "...")
			{
				if (iter.value() > maxsize)
				{
					maxsize = iter.value();
					list.prepend(iter.key());
				}
				else
				{
					bool inserted = false;

					for (auto i = 0; i < list.size(); i++)
					{
						if (result[list[i]] <= iter.value())
						{
							inserted = true;
							list.insert(i, iter.key());
							break;
						}
					}

					if (!inserted)
						list.append(iter.key());
				}
			}
		}

		list.append("...");

		for (const auto& i : list)
		{
			series->append(QString("%1 (%2)").arg(i).arg(locale.formattedDataSize(result[i])), result[i]);
		}

		chart->chart()->setTitle(QString("%1 (%2)").arg(lpath->text()).arg(locale.formattedDataSize(total)));
	});

	QObject::connect(lpath, &QLineEdit::textChanged, [series, watcher, chart](const QString text)
	{
		series->clear();
		chart->chart()->setTitle("Scaning...");
		watcher->setFuture(QtConcurrent::run(CalcContentSize, text));
	});

	view->show();

	lpath->setText(QString(FileToLoad));

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QFileInfo fi(FileToLoad);

	if (!fi.isDir())
		return LISTPLUGIN_ERROR;

	QFrame *view = (QFrame*)ParentWin;
	QLineEdit *lpath = view->findChild<QLineEdit*>("pathlabel");
	lpath->setText(QString(FileToLoad));

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QFrame *view = (QFrame*)ListWin;
	QFutureWatcher<QMap<QString, qint64>> *watcher = view->findChild<QFutureWatcher<QMap<QString, qint64>> *>("watcher");

	if (watcher->isRunning())
		watcher->waitForFinished();

	delete watcher;
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
