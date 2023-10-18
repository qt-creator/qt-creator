// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dialogs/restartdialog.h"
#include "dialogs/ioptionspage.h"
#include "generalsettings.h"
#include "coreconstants.h"
#include "coreplugintr.h"
#include "icore.h"
#include "themechooser.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/hostosinfo.h>
#include <utils/infobar.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcolorbutton.h>
#include <utils/stylehelper.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStyleHints>
#include <QTextCodec>

using namespace Utils;
using namespace Layouting;

namespace Core::Internal {

const char settingsKeyDpiPolicy[] = "Core/HighDpiScaleFactorRoundingPolicy";
const char settingsKeyCodecForLocale[] = "General/OverrideCodecForLocale";
const char settingsKeyToolbarStyle[] = "General/ToolbarStyle";

static bool defaultShowShortcutsInContextMenu()
{
    return QGuiApplication::styleHints()->showShortcutsInContextMenus();
}

GeneralSettings &generalSettings()
{
    static GeneralSettings theSettings;
    return theSettings;
}

GeneralSettings::GeneralSettings()
{
    setAutoApply(false);

    showShortcutsInContextMenus.setSettingsKey("General/ShowShortcutsInContextMenu");
    showShortcutsInContextMenus.setDefaultValue(defaultShowShortcutsInContextMenu());
    showShortcutsInContextMenus.setLabelText(
        Tr::tr("Show keyboard shortcuts in context menus (default: %1)")
            .arg(defaultShowShortcutsInContextMenu() ? Tr::tr("on") : Tr::tr("off")));

    connect(&showShortcutsInContextMenus, &BaseAspect::changed, this, [this] {
        QCoreApplication::setAttribute(Qt::AA_DontShowShortcutsInContextMenus,
                                       !showShortcutsInContextMenus());
    });

    readSettings();
}

class GeneralSettingsWidget final : public IOptionsPageWidget
{
public:
    GeneralSettingsWidget();

    void apply() final;

    void resetInterfaceColor();
    void resetWarnings();
    void resetLanguage();

    static bool canResetWarnings();
    void fillLanguageBox() const;
    static QString language();
    static void setLanguage(const QString&);
    void fillCodecBox() const;
    static QByteArray codecForLocale();
    static void setCodecForLocale(const QByteArray&);
    void fillToolbarStyleBox() const;
    static void setDpiPolicy(Qt::HighDpiScaleFactorRoundingPolicy policy);

    QComboBox *m_languageBox;
    QComboBox *m_codecBox;
    QtColorButton *m_colorButton;
    ThemeChooser *m_themeChooser;
    QPushButton *m_resetWarningsButton;
    QComboBox *m_toolbarStyleBox;
    QComboBox *m_policyComboBox = nullptr;
};

GeneralSettingsWidget::GeneralSettingsWidget()
    : m_languageBox(new QComboBox)
    , m_codecBox(new QComboBox)
    , m_colorButton(new QtColorButton)
    , m_themeChooser(new ThemeChooser)
    , m_resetWarningsButton(new QPushButton)
    , m_toolbarStyleBox(new QComboBox)
{
    m_languageBox->setObjectName("languageBox");
    m_languageBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_languageBox->setMinimumContentsLength(20);

    m_codecBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_codecBox->setMinimumContentsLength(20);

    m_colorButton->setMinimumSize(QSize(64, 0));
    m_colorButton->setProperty("alphaAllowed", QVariant(false));

    m_resetWarningsButton->setText(Tr::tr("Reset Warnings", "Button text"));
    m_resetWarningsButton->setToolTip(
        Tr::tr("Re-enable warnings that were suppressed by selecting \"Do Not "
           "Show Again\" (for example, missing highlighter).",
           nullptr));

    m_toolbarStyleBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    auto resetColorButton = new QPushButton(Tr::tr("Reset"));
    resetColorButton->setToolTip(Tr::tr("Reset to default.", "Color"));

    Form form;
    form.addRow({Tr::tr("Color:"), m_colorButton, resetColorButton, st});
    form.addRow({Tr::tr("Theme:"), m_themeChooser});
    form.addRow({Tr::tr("Toolbar style:"), m_toolbarStyleBox, st});
    form.addRow({Tr::tr("Language:"), m_languageBox, st});

    if (StyleHelper::defaultHighDpiScaleFactorRoundingPolicy()
        != Qt::HighDpiScaleFactorRoundingPolicy::Unset) {
        using Policy = Qt::HighDpiScaleFactorRoundingPolicy;
        m_policyComboBox = new QComboBox;
        m_policyComboBox->addItem(Tr::tr("Round Up for .5 and Above"), int(Policy::Round));
        m_policyComboBox->addItem(Tr::tr("Always Round Up"), int(Policy::Ceil));
        m_policyComboBox->addItem(Tr::tr("Always Round Down"), int(Policy::Floor));
        m_policyComboBox->addItem(Tr::tr("Round Up for .75 and Above"),
                                  int(Policy::RoundPreferFloor));
        m_policyComboBox->addItem(Tr::tr("Don't Round"), int(Policy::PassThrough));
        m_policyComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        const Policy userPolicy =
            ICore::settings()->value(settingsKeyDpiPolicy,
                                     int(StyleHelper::defaultHighDpiScaleFactorRoundingPolicy()))
                                      .value<Policy>();
        m_policyComboBox->setCurrentIndex(m_policyComboBox->findData(int(userPolicy)));

        form.addRow({Tr::tr("DPI rounding policy:"), m_policyComboBox, st});
    }

    form.addRow({empty, generalSettings().showShortcutsInContextMenus});
    form.addRow({Row{m_resetWarningsButton, st}});
    form.addRow({Tr::tr("Text codec for tools:"), m_codecBox, st});
    Column{Group{title(Tr::tr("User Interface")), form}}.attachTo(this);

    fillLanguageBox();
    fillCodecBox();
    fillToolbarStyleBox();

    m_colorButton->setColor(StyleHelper::requestedBaseColor());
    m_resetWarningsButton->setEnabled(canResetWarnings());

    connect(resetColorButton,
            &QAbstractButton::clicked,
            this,
            &GeneralSettingsWidget::resetInterfaceColor);
    connect(m_resetWarningsButton,
            &QAbstractButton::clicked,
            this,
            &GeneralSettingsWidget::resetWarnings);
}

static bool hasQmFilesForLocale(const QString &locale, const QString &creatorTrPath)
{
    static const QString qtTrPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);

    const QString trFile = QLatin1String("/qt_") + locale + QLatin1String(".qm");
    return QFileInfo::exists(qtTrPath + trFile) || QFileInfo::exists(creatorTrPath + trFile);
}

void GeneralSettingsWidget::fillLanguageBox() const
{
    const QString currentLocale = language();

    m_languageBox->addItem(Tr::tr("<System Language>"), QString());
    // need to add this explicitly, since there is no qm file for English
    m_languageBox->addItem(QLatin1String("English"), QLatin1String("C"));
    if (currentLocale == QLatin1String("C"))
        m_languageBox->setCurrentIndex(m_languageBox->count() - 1);

    const FilePath creatorTrPath = ICore::resourcePath("translations");
    const FilePaths languageFiles = creatorTrPath.dirEntries(
        QStringList(QLatin1String("qtcreator*.qm")));

    for (const FilePath &languageFile : languageFiles) {
        const QString name = languageFile.fileName();
        int start = name.indexOf('_') + 1;
        int end = name.lastIndexOf('.');
        const QString locale = name.mid(start, end - start);
        // no need to show a language that creator will not load anyway
        if (hasQmFilesForLocale(locale, creatorTrPath.toString())) {
            QLocale tmpLocale(locale);
            QString languageItem = QLocale::languageToString(tmpLocale.language()) + QLatin1String(" (")
                                   + QLocale::countryToString(tmpLocale.country()) + QLatin1Char(')');
            m_languageBox->addItem(languageItem, locale);
            if (locale == currentLocale)
                m_languageBox->setCurrentIndex(m_languageBox->count() - 1);
        }
    }
}

void GeneralSettingsWidget::apply()
{
    generalSettings().apply();
    generalSettings().writeSettings();

    int currentIndex = m_languageBox->currentIndex();
    setLanguage(m_languageBox->itemData(currentIndex, Qt::UserRole).toString());
    if (m_policyComboBox) {
        const Qt::HighDpiScaleFactorRoundingPolicy selectedPolicy =
            m_policyComboBox->currentData().value<Qt::HighDpiScaleFactorRoundingPolicy>();
        setDpiPolicy(selectedPolicy);
    }
    currentIndex = m_codecBox->currentIndex();
    setCodecForLocale(m_codecBox->itemText(currentIndex).toLocal8Bit());
    // Apply the new base color if accepted
    StyleHelper::setBaseColor(m_colorButton->color());
    m_themeChooser->apply();
    if (const auto newStyle = m_toolbarStyleBox->currentData().value<StyleHelper::ToolbarStyle>();
        newStyle != StyleHelper::toolbarStyle()) {
        ICore::settings()->setValueWithDefault(settingsKeyToolbarStyle, int(newStyle),
                                               int(StyleHelper::defaultToolbarStyle));
        StyleHelper::setToolbarStyle(newStyle);
        QStyle *applicationStyle = QApplication::style();
        for (QWidget *widget : QApplication::allWidgets())
            applicationStyle->polish(widget);
    }
}

void GeneralSettingsWidget::resetInterfaceColor()
{
    m_colorButton->setColor(StyleHelper::DEFAULT_BASE_COLOR);
}

void GeneralSettingsWidget::resetWarnings()
{
    InfoBar::clearGloballySuppressed();
    CheckableMessageBox::resetAllDoNotAskAgainQuestions();
    m_resetWarningsButton->setEnabled(false);
}

bool GeneralSettingsWidget::canResetWarnings()
{
    return InfoBar::anyGloballySuppressed() || CheckableMessageBox::hasSuppressedQuestions();
}

void GeneralSettingsWidget::resetLanguage()
{
    // system language is default
    m_languageBox->setCurrentIndex(0);
}

QString GeneralSettingsWidget::language()
{
    QtcSettings *settings = ICore::settings();
    return settings->value("General/OverrideLanguage").toString();
}

void GeneralSettingsWidget::setLanguage(const QString &locale)
{
    QtcSettings *settings = ICore::settings();
    if (settings->value("General/OverrideLanguage").toString() != locale) {
        RestartDialog dialog(ICore::dialogParent(),
                             Tr::tr("The language change will take effect after restart."));
        dialog.exec();
    }

    settings->setValueWithDefault("General/OverrideLanguage", locale, {});
}

void GeneralSettingsWidget::fillCodecBox() const
{
    const QByteArray currentCodec = codecForLocale();

    const QByteArrayList codecs = Utils::sorted(QTextCodec::availableCodecs());
    for (const QByteArray &codec : codecs) {
        m_codecBox->addItem(QString::fromLocal8Bit(codec));
        if (codec == currentCodec)
            m_codecBox->setCurrentIndex(m_codecBox->count() - 1);
    }
}

QByteArray GeneralSettingsWidget::codecForLocale()
{
    QtcSettings *settings = ICore::settings();
    QByteArray codec = settings->value(settingsKeyCodecForLocale).toByteArray();
    if (codec.isEmpty())
        codec = QTextCodec::codecForLocale()->name();
    return codec;
}

void GeneralSettingsWidget::setCodecForLocale(const QByteArray &codec)
{
    QtcSettings *settings = ICore::settings();
    settings->setValueWithDefault(settingsKeyCodecForLocale, codec, {});
    QTextCodec::setCodecForLocale(QTextCodec::codecForName(codec));
}

StyleHelper::ToolbarStyle toolbarStylefromSettings()
{
    if (!ExtensionSystem::PluginManager::instance()) // May happen in tests
        return StyleHelper::defaultToolbarStyle;

    return StyleHelper::ToolbarStyle(
        ICore::settings()->value(settingsKeyToolbarStyle,
                                 StyleHelper::defaultToolbarStyle).toInt());
}

void GeneralSettingsWidget::fillToolbarStyleBox() const
{
    m_toolbarStyleBox->addItem(Tr::tr("Compact"), StyleHelper::ToolbarStyleCompact);
    m_toolbarStyleBox->addItem(Tr::tr("Relaxed"), StyleHelper::ToolbarStyleRelaxed);
    const int curId = m_toolbarStyleBox->findData(toolbarStylefromSettings());
    m_toolbarStyleBox->setCurrentIndex(curId);
}

void GeneralSettingsWidget::setDpiPolicy(Qt::HighDpiScaleFactorRoundingPolicy policy)
{
    QtcSettings *settings = ICore::settings();
    using Policy = Qt::HighDpiScaleFactorRoundingPolicy;
    const Policy previousPolicy = settings->value(
                settingsKeyDpiPolicy,
                int(StyleHelper::defaultHighDpiScaleFactorRoundingPolicy())).value<Policy>();
    if (policy != previousPolicy) {
        RestartDialog dialog(ICore::dialogParent(),
                             Tr::tr("The DPI rounding policy change will take effect after "
                                    "restart."));
        dialog.exec();
    }
    settings->setValueWithDefault(settingsKeyDpiPolicy, int(policy),
                                  int(StyleHelper::defaultHighDpiScaleFactorRoundingPolicy()));
}

void GeneralSettings::applyToolbarStyleFromSettings()
{
    StyleHelper::setToolbarStyle(toolbarStylefromSettings());
}

// GeneralSettingsPage

class GeneralSettingsPage final : public IOptionsPage
{
public:
    GeneralSettingsPage()
    {
        setId(Constants::SETTINGS_ID_INTERFACE);
        setDisplayName(Tr::tr("Interface"));
        setCategory(Constants::SETTINGS_CATEGORY_CORE);
        setDisplayCategory(Tr::tr("Environment"));
        setCategoryIconPath(":/core/images/settingscategory_core.png");
        setWidgetCreator([] { return new GeneralSettingsWidget; });
    }
};

const GeneralSettingsPage settingsPage;

} // Core::Internal
