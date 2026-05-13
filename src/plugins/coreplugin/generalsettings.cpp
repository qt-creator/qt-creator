// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "generalsettings.h"
#include "coreconstants.h"
#include "coreplugintr.h"
#include "dialogs/ioptionspage.h"
#include "icore.h"
#include "themechooser.h"


#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/infobar.h>
#include <utils/layoutbuilder.h>
#include <utils/shutdownguard.h>
#include <utils/stylehelper.h>
#include <utils/textcodec.h>

#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QLibraryInfo>
#include <QPushButton>
#include <QStandardItem>
#include <QStyleHints>

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

static bool hasQmFilesForLocale(const QString &locale, const QString &creatorTrPath)
{
    static const QString qtTrPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);

    const QString trFile = QLatin1String("/qt_") + locale + QLatin1String(".qm");
    return QFileInfo::exists(qtTrPath + trFile) || QFileInfo::exists(creatorTrPath + trFile);
}

static void fillLanguageItems(const StringSelectionAspect::ResultCallback &cb)
{
    QList<QStandardItem *> items;

    auto systemItem = new QStandardItem(Tr::tr("<System Language>"));
    systemItem->setData(LanguageSelectionAspect::kSystemLanguage);
    items.append(systemItem);

    struct LangItem
    {
        QString display;
        QString locale;
        QString comparisonId;
    };

    QList<LangItem> langItems;
    // need to add this explicitly, since there is no qm file for English
    const QString english = QLocale::languageToString(QLocale::English);
    langItems.append({english, QString("en"), english});

    const FilePath creatorTrPath = ICore::resourcePath("translations");
    const FilePaths languageFiles = creatorTrPath.dirEntries(
        QStringList(QLatin1String("qtcreator*.qm")));
    for (const FilePath &languageFile : languageFiles) {
        const QString name = languageFile.fileName();
        // Ignore english ts file that is for temporary spelling fixes only.
        // We have the "English" item that is explicitly added at the top.
        if (name == "qtcreator_en.qm")
            continue;
        int start = name.indexOf('_') + 1;
        int end = name.lastIndexOf('.');
        const QString locale = name.mid(start, end - start);
        // no need to show a language that creator will not load anyway
        if (hasQmFilesForLocale(locale, creatorTrPath.toUrlishString())) {
            QLocale tmpLocale(locale);
            const auto display = QString("%1 (%2) - %3 (%4)")
                                     .arg(tmpLocale.nativeLanguageName(),
                                          tmpLocale.nativeTerritoryName(),
                                          QLocale::languageToString(tmpLocale.language()),
                                          QLocale::territoryToString(tmpLocale.territory()));
            // Create a fancy comparison string.
            // We cannot use a "locale aware comparison" because we are comparing
            // different locales. The probably "optimal solution" would be to compare
            // by "latinized native name", but that's hard. Instead
            // - for non-Latin-script locales use the english name, otherwise the native name
            // - get rid of fancy characters like '\u010d' by decomposing them (e.g. to 'c')
            QString comparisonId
                = tmpLocale.script() == QLocale::LatinScript
                      ? (tmpLocale.nativeLanguageName() + " "
                         + tmpLocale.nativeTerritoryName())
                      : (QLocale::languageToString(tmpLocale.language()) + " "
                         + QLocale::territoryToString(tmpLocale.territory()));
            for (int i = 0; i < comparisonId.size(); ++i) {
                QChar &c = comparisonId[i];
                if (c.decomposition().isEmpty())
                    continue;
                c = c.decomposition().at(0);
            }
            langItems.append({display, locale, comparisonId});
        }
    }

    Utils::sort(langItems, [](const LangItem &a, const LangItem &b) {
        return a.comparisonId.compare(b.comparisonId, Qt::CaseInsensitive) < 0;
    });
    for (const LangItem &i : std::as_const(langItems)) {
        auto item = new QStandardItem(i.display);
        item->setData(i.locale);
        items.append(item);
    }

    cb(items);
}

GeneralSettings &generalSettings()
{
    static GuardedObject<GeneralSettings> theSettings;
    return theSettings;
}

static void fillThemeItems(const StringSelectionAspect::ResultCallback &cb)
{
    const QList<ThemeEntry> themes = ThemeEntry::availableThemes();
    QList<QStandardItem *> items;
    for (const ThemeEntry &t : themes) {
        auto item = new QStandardItem(t.displayName());
        item->setData(t.id().toString());
        items.append(item);
    }
    cb(items);
}

class ResetWarningsButton : public QPushButton
{
public:
    ResetWarningsButton()
    {
        setText(Tr::tr("Reset Warnings", "Button text"));
        setToolTip(
            Tr::tr("Re-enable warnings that were suppressed by selecting \"Do Not "
                   "Show Again\" (for example, missing highlighter)."));
        setEnabled(InfoBar::anyGloballySuppressed()
                   || CheckableMessageBox::hasSuppressedQuestions());
        connect(this, &QAbstractButton::clicked, this, [this] {
            InfoBar::clearGloballySuppressed();
            CheckableMessageBox::resetAllDoNotAskAgainQuestions();
            setEnabled(InfoBar::anyGloballySuppressed()
                       || CheckableMessageBox::hasSuppressedQuestions());
        });
    }
};

InfoLabel *createEnvVarInfoLabel()
{
    static const bool showDpiPolicy = StyleHelper::defaultHighDpiScaleFactorRoundingPolicy()
                                      != Qt::HighDpiScaleFactorRoundingPolicy::Unset;

    if (!showDpiPolicy)
        return nullptr;

    static const QList<const char *> envVars = {
        StyleHelper::C_QT_SCALE_FACTOR_ROUNDING_POLICY,
        "QT_ENABLE_HIGHDPI_SCALING",
        "QT_FONT_DPI",
        "QT_SCALE_FACTOR",
        "QT_SCREEN_SCALE_FACTORS",
        "QT_USE_PHYSICAL_DPI",
    };
    auto setVars = Utils::filtered(envVars, &qEnvironmentVariableIsSet);

    if (setVars.isEmpty())
        return nullptr;

    QString toolTip
        = Tr::tr(
              "The following environment variables are set and can "
              "influence the UI scaling behavior of %1:")
              .arg(QGuiApplication::applicationDisplayName())
          + "\n\n"
          + Utils::transform<QStringList>(setVars, [](const char *var) {
                return QString("%1=%2").arg(QString::fromUtf8(var)).arg(qEnvironmentVariable(var));
            }).join("\n");
    auto envVarInfo = new InfoLabel(Tr::tr("Environment influences UI scaling behavior."));
    envVarInfo->setAdditionalToolTip(toolTip);
    return envVarInfo;
}

GeneralSettings::GeneralSettings()
{
    setAutoApply(false);

    showShortcutsInContextMenus.setSettingsKey("General/ShowShortcutsInContextMenu");
    showShortcutsInContextMenus.setDefaultValue(defaultShowShortcutsInContextMenu());
    showShortcutsInContextMenus.setLabelText(
        Tr::tr("Show keyboard shortcuts in context menus (default: %1)")
            .arg(defaultShowShortcutsInContextMenu() ? Tr::tr("on") : Tr::tr("off")));
    showShortcutsInContextMenus.addOnChanged(this, [this] {
        QCoreApplication::setAttribute(Qt::AA_DontShowShortcutsInContextMenus,
                                       !showShortcutsInContextMenus());
    });

    provideSplitterCursors.setSettingsKey("General/OverrideSplitterCursors");
    provideSplitterCursors.setDefaultValue(false);
    provideSplitterCursors.setLabelText(Tr::tr("Override cursors for views"));
    provideSplitterCursors.setToolTip(
        Tr::tr("Provide cursors for resizing views.\nIf the system cursors for resizing views are "
               "not displayed properly, you can use the cursors provided by %1.")
            .arg(QGuiApplication::applicationDisplayName()));

    preferInfoBarOverPopup.setSettingsKey("General/PreferInfoBarOverPopup");
    preferInfoBarOverPopup.setDefaultValue(false);
    preferInfoBarOverPopup.setLabelText(Tr::tr("Prefer banner style info bars over pop-ups"));

    useTabsInEditorViews.setSettingsKey("General/UseTabsInEditorViews");
    useTabsInEditorViews.setDefaultValue(false);
    useTabsInEditorViews.setLabelText(Tr::tr("Use tabbed editors"));

    showOkAndCancelInSettingsMode.setSettingsKey("General/ShowOkAndCancelInSettingsMode");
    showOkAndCancelInSettingsMode.setDefaultValue(false);
    showOkAndCancelInSettingsMode.setLabelText(Tr::tr("Show OK and Cancel buttons in Preferences mode"));

    codecForLocale.setSettingsKey(settingsKeyCodecForLocale);
    codecForLocale.setLabelText(Tr::tr("Text codec for tools:"));
    codecForLocale.setComboBoxEditable(false);
    codecForLocale.setFillCallback([](const StringSelectionAspect::ResultCallback &cb) {
        const auto codecs = TextEncoding::availableEncodings();
        QList<QStandardItem *> codecNames;
        for (const auto &codec : codecs) {
            auto item = new QStandardItem(codec.displayName());
            item->setData(QString::fromUtf8(codec.name()));
            codecNames.append(item);
        }
        cb(codecNames);
    });

    connect(&codecForLocale, &BaseAspect::changed, this, [this] {
        TextEncoding::setEncodingForLocale(codecForLocale.value().toUtf8());
    });

    language.setSettingsKey("General/OverrideLanguage");
    language.setDefaultValue(LanguageSelectionAspect::kSystemLanguage);
    language.setLabelText(Tr::tr("Language:"));
    language.setComboBoxEditable(false);
    language.setFillCallback(&fillLanguageItems);

    connect(&language, &BaseAspect::changed, this, [] {
        ICore::askForRestart(Tr::tr("The language change will take effect after restart."));
    });

    color.setSettingsKey("MainWindow/Color");
    color.setLabelText(Tr::tr("Color:"));
    color.setDefaultValue(QColor(StyleHelper::DEFAULT_BASE_COLOR));
    color.setAlphaAllowed(false);
    color.addOnChanged(this, [this] { StyleHelper::setBaseColor(color()); });

    static const SelectionAspect::Option options[] = {
        {Tr::tr("Round Up for .5 and Above"), {}, int(Qt::HighDpiScaleFactorRoundingPolicy::Round)},
        {Tr::tr("Always Round Up"), {}, int(Qt::HighDpiScaleFactorRoundingPolicy::Ceil)},
        {Tr::tr("Always Round Down"), {}, int(Qt::HighDpiScaleFactorRoundingPolicy::Floor)},
        {Tr::tr("Round Up for .75 and Above"),
         {},
         int(Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor)},
        {Tr::tr("Don't Round"), {}, int(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough)},
    };

    highDpiScaleFactorRoundingPolicy.setSettingsKey(settingsKeyDpiPolicy);
    highDpiScaleFactorRoundingPolicy.setLabelText(Tr::tr("DPI rounding policy"));
    highDpiScaleFactorRoundingPolicy.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    highDpiScaleFactorRoundingPolicy.setUseDataAsSavedValue();

    for (const auto &option : options)
        highDpiScaleFactorRoundingPolicy.addOption(option);

    const int defaultDpiValue = int(StyleHelper::defaultHighDpiScaleFactorRoundingPolicy());
    highDpiScaleFactorRoundingPolicy.setDefaultValue(int(defaultDpiValue));

    connect(&highDpiScaleFactorRoundingPolicy, &BaseAspect::changed, this, [] {
        ICore::askForRestart(
            Tr::tr("The DPI rounding policy change will take effect after restart."));
    });

    toolbarStyle.setSettingsKey(settingsKeyToolbarStyle);
    toolbarStyle.setLabelText(Tr::tr("Toolbar style:"));
    toolbarStyle.addOption({Tr::tr("Compact"), {}, int(StyleHelper::ToolbarStyle::Compact)});
    toolbarStyle.addOption({Tr::tr("Relaxed"), {}, int(StyleHelper::ToolbarStyle::Relaxed)});
    toolbarStyle.setDefaultValue(int(StyleHelper::defaultToolbarStyle()));
    toolbarStyle.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    connect(&toolbarStyle, &BaseAspect::changed, this, [this] {
        auto newStyle = StyleHelper::ToolbarStyle(toolbarStyle());

        if (newStyle != StyleHelper::toolbarStyle()) {
            StyleHelper::setToolbarStyle(newStyle);
            QStyle *applicationStyle = QApplication::style();
            for (QWidget *widget : QApplication::allWidgets())
                applicationStyle->polish(widget);
        }
    });

    theme.setSettingsKey(Constants::SETTINGS_THEME);
    theme.setLabelText(Tr::tr("Theme:"));
    theme.setComboBoxEditable(false);
    theme.setFillCallback(&fillThemeItems);
    theme.setDefaultValue(ThemeEntry::defaultThemeId().toString());
    connect(&theme, &BaseAspect::changed, this, [] {
        ICore::askForRestart(Tr::tr("The theme change will take effect after restart."));
    });

    setLayouter([this]() -> Layouting::Layout {
        static const bool showDpiPolicy = StyleHelper::defaultHighDpiScaleFactorRoundingPolicy()
                                          != Qt::HighDpiScaleFactorRoundingPolicy::Unset;

        auto envVarInfo = createEnvVarInfoLabel();

        // clang-format off
        return Column {
            Form {
                color, st, br,
                theme, st, br,
                toolbarStyle, st, br,
                language, st, br,
                If (showDpiPolicy) >> Then {
                    highDpiScaleFactorRoundingPolicy, st, br,
                    If (envVarInfo) >> Then { envVarInfo, br } >> Else { st, br },
                },
                codecForLocale, st, br,
                showShortcutsInContextMenus, st, br,
                provideSplitterCursors, st, br,
                preferInfoBarOverPopup, st, br,
                useTabsInEditorViews, st, br,
                showOkAndCancelInSettingsMode, st, br,
                Row { new ResetWarningsButton, st },
            }
        };
        // clang-format on
    });

    readSettings();

    StyleHelper::setToolbarStyle(StyleHelper::ToolbarStyle(toolbarStyle()));
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
        setSettingsProvider([] { return &generalSettings(); });
    }
};

const GeneralSettingsPage settingsPage;

} // namespace Core::Internal
