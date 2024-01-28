#include <QtWidgets>
#include <QFontDatabase>
#include <QSettings>

#include <libintl.h>
#include <locale.h>
#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

#include <dlfcn.h>
#include "wlxplugin.h"

#define DEFAULT_FONTSIZE 20

QString gCfgPath;
QString gText(_("the quick brown fox jumps over the lazy dog"));
int gSize = DEFAULT_FONTSIZE;
bool gBold = false;
bool gItalic = false;

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	int id = QFontDatabase::addApplicationFont(QString(FileToLoad));

	if (id == -1)
		return nullptr;

	QFrame *view = new QFrame((QWidget*)ParentWin);
	view->setFrameStyle(QFrame::NoFrame);
	QVBoxLayout *main = new QVBoxLayout(view);
	QHBoxLayout *controls = new QHBoxLayout;
	QLabel *preview = new QLabel(view);
	QComboBox *families = new QComboBox(view);
	QSlider *fontsize = new QSlider(Qt::Horizontal, view);
	QLineEdit *usertext = new QLineEdit(view);
	QCheckBox *bold = new QCheckBox(QString(_("Bold")), view);
	QCheckBox *italic = new QCheckBox(QString(_("Italic")), view);

	fontsize->setMinimum(6);
	fontsize->setMaximum(72);
	fontsize->setValue(gSize);

	if (gBold)
		bold->setCheckState(Qt::Checked);

	if (gItalic)
		italic->setCheckState(Qt::Checked);

	QObject::connect(usertext, &QLineEdit::textChanged, [preview](QString text)
	{
		gText = text;
		preview->setText(text);
	});

	QObject::connect(families, &QComboBox::currentTextChanged, [families, bold, italic, fontsize, preview](QString text)
	{
		QFont font;
		font.setFamily(text);
		font.setPointSize(gSize);
		font.setBold((bold->checkState() == Qt::Checked));
		font.setItalic((italic->checkState() == Qt::Checked));
		preview->setFont(font);
	});

	QObject::connect(bold, &QCheckBox::stateChanged, [preview](int state)
	{
		gBold = (state == Qt::Checked);
		QFont font = preview->font();
		font.setBold(gBold);
		preview->setFont(font);
	});

	QObject::connect(italic, &QCheckBox::stateChanged, [preview](int state)
	{
		gItalic = (state == Qt::Checked);
		QFont font = preview->font();
		font.setItalic(gItalic);
		preview->setFont(font);
	});

	QObject::connect(fontsize, &QSlider::valueChanged, [preview](int x)
	{
		gSize = x;
		QFont font = preview->font();
		font.setPointSize(gSize);
		preview->setFont(font);
	});

	preview->setAlignment(Qt::AlignCenter);
	QStringList font_families = QFontDatabase::applicationFontFamilies(id);
	families->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	families->addItems(font_families);
	usertext->setText(gText);

	controls->addWidget(families);
	controls->addWidget(usertext);
	controls->addWidget(bold);
	controls->addWidget(italic);
	controls->addWidget(fontsize);
	main->addLayout(controls);
	main->addWidget(preview);

	families->setObjectName("combobox");
	view->setProperty("fontID", id);

	view->show();

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	QFrame *view = (QFrame*)PluginWin;
	int id = view->property("fontID").value<int>();
	QFontDatabase::removeApplicationFont(id);

	id = QFontDatabase::addApplicationFont(QString(FileToLoad));

	if (id == -1)
		return LISTPLUGIN_ERROR;

	QComboBox *families = view->findChild<QComboBox*>("combobox");
	QStringList font_families = QFontDatabase::applicationFontFamilies(id);
	families->clear();
	families->addItems(font_families);
	view->setProperty("fontID", id);

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QFrame *view = (QFrame*)ListWin;
	int id = view->property("fontID").value<int>();
	QFontDatabase::removeApplicationFont(id);

	QSettings settings(gCfgPath, QSettings::IniFormat);
#if QT_VERSION < 0x060000
	settings.setIniCodec("UTF-8");
#endif
	settings.setValue(PLUGNAME "/fontsize", gSize);
	settings.setValue(PLUGNAME "/text", gText);
	settings.setValue(PLUGNAME "/bold", gBold);
	settings.setValue(PLUGNAME "/italic", gItalic);

	delete view;
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	snprintf(DetectString, maxlen - 1, DETECT_STRING);
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	QFileInfo defini(QString::fromStdString(dps->DefaultIniName));
	gCfgPath = defini.absolutePath() + "/j2969719.ini";
	QSettings settings(gCfgPath, QSettings::IniFormat);
#if QT_VERSION < 0x060000
	settings.setIniCodec("UTF-8");
#endif
	gSize = settings.value(PLUGNAME "/fontsize").toInt();

	if (gSize > 0)
	{
		gBold = settings.value(PLUGNAME "/bold").toBool();
		gItalic = settings.value(PLUGNAME "/italic").toBool();
		gText = settings.value(PLUGNAME "/text").toString();
	}
	else
		gSize = DEFAULT_FONTSIZE;

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
