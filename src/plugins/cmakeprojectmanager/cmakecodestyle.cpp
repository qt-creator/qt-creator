// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakecodestyle.h"

#include "cmakeformatter.h"
#include "cmakeindenter.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <texteditor/codestyleeditor.h>
#include <texteditor/codestylepool.h>
#include <texteditor/displaysettings.h>
#include <texteditor/icodestylepreferences.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/snippets/snippeteditor.h>
#include <texteditor/snippets/snippetprovider.h>
#include <texteditor/tabsettings.h>
#include <texteditor/textdocument.h>

#include <cmakelang/cmakeformatter.h>

#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeconstants.h>
#include <utils/shutdownguard.h>

#include <QCheckBox>
#include <QScopedValueRollback>
#include <QTextDocument>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace TextEditor;
using namespace Utils;

namespace CMakeProjectManager::Internal {

const Key spaceBeforeControlParenKey = "SpaceBeforeControlParen";
const Key spaceBeforeCommandParenKey = "SpaceBeforeCommandParen";
const Key indentKeywordValuesKey = "IndentKeywordValues";
const Key keepCommentColumnKey = "KeepCommentColumn";

void CMakeCodeStyleSettings::toMap(Store &map) const
{
    map.insert(spaceBeforeControlParenKey, spaceBeforeControlParen);
    map.insert(spaceBeforeCommandParenKey, spaceBeforeCommandParen);
    map.insert(indentKeywordValuesKey, indentKeywordValues);
    map.insert(keepCommentColumnKey, keepCommentColumn);
}

void CMakeCodeStyleSettings::fromMap(const Store &map)
{
    spaceBeforeControlParen
        = map.value(spaceBeforeControlParenKey, spaceBeforeControlParen).toBool();
    spaceBeforeCommandParen
        = map.value(spaceBeforeCommandParenKey, spaceBeforeCommandParen).toBool();
    indentKeywordValues = map.value(indentKeywordValuesKey, indentKeywordValues).toBool();
    keepCommentColumn = map.value(keepCommentColumnKey, keepCommentColumn).toBool();
}

Id CMakeCodeStyleSettings::settingsId()
{
    return Constants::CMAKE_CODE_STYLE_SETTINGS_ID;
}

using CMakeCodeStylePreferences = TypedCodeStylePreferences<CMakeCodeStyleSettings>;

static const char codeStylePreview[]
    = "cmake_minimum_required(VERSION 3.16)\n"
      "project(hello LANGUAGES CXX)\n"
      "\n"
      "set(APP_NAME hello)     # the comments of these two\n"
      "set(APP_VERSION 1.0)    # lines stand in one column\n"
      "\n"
      "add_executable(${APP_NAME}\n"
      "    main.cpp\n"
      "    window.cpp\n"
      ")\n"
      "\n"
      "target_link_libraries(${APP_NAME}\n"
      "    PRIVATE\n"
      "        Qt6::Widgets\n"
      "        Qt6::Network\n"
      ")\n"
      "\n"
      "if(WIN32)\n"
      "    target_include_directories(${APP_NAME} PRIVATE win32)\n"
      "endif()\n";

// The value editor of the CMake style: the tab settings plus what the
// indenter does beyond them. The selector and the preview around it come from
// the hosting CodeStyleAspect, which owns the deferred commit.
class CMakeCodeStyleWidget final : public QWidget
{
public:
    explicit CMakeCodeStyleWidget(ICodeStylePreferences *preferences)
        : m_preferences(preferences)
    {
        // Nothing in a CMake file is ever aligned with what stands above it,
        // so the setting that governs that has nothing to govern here.
        m_tabSettings.continuationAlignBehavior.setVisible(false);
        m_tabSettings.setPreferences(preferences);

        m_formatterNote = new InfoLabel(
            Tr::tr("The cmake-format command lays a file out through its own configuration "
                   "files, not through the style below. The style still indents what you type "
                   "and the files the wizards write."));
        m_formatterNote->setAlignment(Qt::AlignTop);
        m_formatterNote->setFilled(true);
        m_formatterNote->setElideMode(Qt::ElideNone);
        m_formatterNote->setWordWrap(true);

        m_spaceBeforeControlParen = addOption(
            Tr::tr("Space between a control keyword and \"(\""),
            Tr::tr("Writes if (WIN32) rather than if(WIN32), and the same for elseif(), "
                   "else(), endif(), foreach(), while(), function(), macro() and block()."));
        m_spaceBeforeCommandParen = addOption(
            Tr::tr("Space between a command name and \"(\""),
            Tr::tr("Writes message (STATUS \"...\") rather than message(STATUS \"...\")."));
        m_indentKeywordValues = addOption(
            Tr::tr("Indent the values below a keyword of its own"),
            Tr::tr("Gives the values that follow a line holding nothing but a keyword of the "
                   "command a level of their own, the way the CMake files of Qt lay out "
                   "SOURCES and the other lists."));
        m_keepCommentColumn = addOption(
            Tr::tr("Keep the column of a comment at the end of a line"),
            Tr::tr("Leaves the whitespace before such a comment as it is, so that comments "
                   "lined up in a column stay lined up. Off, one space stands before each."));

        using namespace Layouting;
        Row {
            Column {
                m_formatterNote,
                &m_tabSettings,
                Group {
                    title(Tr::tr("Calls")),
                    Column {
                        m_spaceBeforeControlParen,
                        m_spaceBeforeCommandParen,
                        m_indentKeywordValues,
                        m_keepCommentColumn
                    }
                },
                st
            },
            Column {
                createPreview(preferences),
                createCodeStylePreviewNote()
            },
            noMargin
        }.attachTo(this);

        connect(preferences, &ICodeStylePreferences::currentValueChanged,
                this, &CMakeCodeStyleWidget::load);
        connect(preferences, &ICodeStylePreferences::currentValueChanged,
                this, &CMakeCodeStyleWidget::updatePreview);
        connect(preferences, &ICodeStylePreferences::currentTabSettingsChanged,
                this, &CMakeCodeStyleWidget::updatePreview);
        connect(preferences, &ICodeStylePreferences::currentPreferencesChanged,
                this, &CMakeCodeStyleWidget::updateEnabled);
        connect(preferences, &ICodeStylePreferences::currentPreferencesChanged,
                this, &CMakeCodeStyleWidget::updatePreview);
        onFormatterChanged(this, [this] { updateFormatterNote(); });
        load();
        updateEnabled();
        updateFormatterNote();
        updatePreview();
    }

private:
    QWidget *createPreview(ICodeStylePreferences *preferences)
    {
        m_preview = new SnippetEditorWidget;
        DisplaySettingsData display = m_preview->displaySettings();
        display.m_visualizeWhitespace = true;
        m_preview->setDisplaySettings(display);
        SnippetProvider::decorateEditor(m_preview, Constants::CMAKE_SNIPPETS_GROUP_ID);

        Indenter *indenter = createCMakeIndenter(m_preview->document());
        indenter->setOverriddenPreferences(preferences);
        m_preview->textDocument()->setIndenter(indenter);

        // What the user types into the preview becomes the text the settings
        // are shown on from then on.
        connect(m_preview->document(), &QTextDocument::contentsChanged, this, [this] {
            if (!m_rendering)
                m_previewSource = m_preview->toPlainText();
        });
        return m_preview;
    }

    // The preview is the formatter's answer for the text it stands on, laid
    // out afresh every time. Formatting what is already shown would not do:
    // most of these settings govern the whitespace between the tokens of a
    // line rather than the whitespace before the first of them, and one of
    // them says to leave whitespace alone, which cannot put back a column
    // that an earlier run took away.
    void updatePreview()
    {
        const QScopedValueRollback<bool> rendering(m_rendering, true);

        TextDocument *document = m_preview->textDocument();
        m_preview->setPlainText(m_previewSource);
        document->indenter()->invalidateCache();
        document->indenter()->format({{1, document->document()->blockCount()}});
    }

    QCheckBox *addOption(const QString &text, const QString &toolTip)
    {
        auto box = new QCheckBox(text);
        box->setToolTip(toolTip);
        connect(box, &QCheckBox::toggled, this, &CMakeCodeStyleWidget::store);
        m_options.append(box);
        return box;
    }

    void load()
    {
        const CMakeCodeStyleSettings settings
            = m_preferences->currentValue().value<CMakeCodeStyleSettings>();

        // Each setChecked() below reaches store(), which would write a mix of
        // the value being loaded and the ones the boxes still hold back into an
        // editable style. Blocking the signals of this widget would not help:
        // it is the boxes that emit them.
        const QScopedValueRollback<bool> loading(m_loading, true);
        m_spaceBeforeControlParen->setChecked(settings.spaceBeforeControlParen);
        m_spaceBeforeCommandParen->setChecked(settings.spaceBeforeCommandParen);
        m_indentKeywordValues->setChecked(settings.indentKeywordValues);
        m_keepCommentColumn->setChecked(settings.keepCommentColumn);
    }

    void store()
    {
        if (m_loading)
            return;

        ICodeStylePreferences *current = m_preferences->currentPreferences();
        if (!current || current->isReadOnly())
            return;

        CMakeCodeStyleSettings settings;
        settings.spaceBeforeControlParen = m_spaceBeforeControlParen->isChecked();
        settings.spaceBeforeCommandParen = m_spaceBeforeCommandParen->isChecked();
        settings.indentKeywordValues = m_indentKeywordValues->isChecked();
        settings.keepCommentColumn = m_keepCommentColumn->isChecked();

        QVariant value;
        value.setValue(settings);
        current->setValue(value);
    }

    void updateFormatterNote()
    {
        m_formatterNote->setVisible(cmakeFormatIsFormatter());
    }

    void updateEnabled()
    {
        ICodeStylePreferences *current = m_preferences->currentPreferences();
        const bool editable = current && !current->isReadOnly();
        for (QCheckBox *box : std::as_const(m_options))
            box->setEnabled(editable);
    }

    ICodeStylePreferences * const m_preferences;
    TabSettings m_tabSettings;
    InfoLabel *m_formatterNote = nullptr;
    SnippetEditorWidget *m_preview = nullptr;
    QString m_previewSource = QLatin1String(codeStylePreview);
    bool m_rendering = false;
    bool m_loading = false;
    QList<QCheckBox *> m_options;
    QCheckBox *m_spaceBeforeControlParen = nullptr;
    QCheckBox *m_spaceBeforeCommandParen = nullptr;
    QCheckBox *m_indentKeywordValues = nullptr;
    QCheckBox *m_keepCommentColumn = nullptr;
};

class CMakeCodeStylePreferencesFactory final : public ICodeStylePreferencesFactory
{
public:
    CMakeCodeStylePreferencesFactory()
        : ICodeStylePreferencesFactory(Constants::CMAKE_LANGUAGE_ID)
    {
        setDisplayName(Tr::tr("CMake"));
        setSnippetGroupId(Constants::CMAKE_SNIPPETS_GROUP_ID);
        setPreviewText(QLatin1String(codeStylePreview));
        setIndenterCreator(&createCMakeIndenter);
        setCodeStyleCreator([] { return new CMakeCodeStylePreferences; });
        setValueEditorCreator([](ICodeStylePreferences *codeStyle) {
            return new CMakeCodeStyleWidget(codeStyle);
        });
        setValueEditorHasPreview(true);

        setGlobalCodeStyleId(Constants::CMAKE_GLOBAL_CODE_STYLE_ID);
        setDefaultCodeStyleId("qt");
        setBuiltInCodeStyles([this](CodeStylePool *pool) {
            TabSettingsData tabSettings;
            tabSettings.m_tabPolicy = TabSettingsData::SpacesOnlyTabPolicy;
            tabSettings.m_tabSize = 4;
            tabSettings.m_indentSize = 4;
            tabSettings.m_continuationAlignBehavior = TabSettingsData::NoContinuationAlign;

            m_qtCodeStyle.setId("qt");
            m_qtCodeStyle.setDisplayName(Tr::tr("Qt"));
            m_qtCodeStyle.setReadOnly(true);
            m_qtCodeStyle.setTabSettings(tabSettings);
            m_qtCodeStyle.setCodeStyleSettings({});
            pool->addCodeStyle(&m_qtCodeStyle);
        });
        setupCodeStyles();

        registerMimeTypeForLanguageId(Utils::Constants::CMAKE_MIMETYPE,
                                      Constants::CMAKE_LANGUAGE_ID);
        registerMimeTypeForLanguageId(Utils::Constants::CMAKE_PROJECT_MIMETYPE,
                                      Constants::CMAKE_LANGUAGE_ID);
    }

private:
    CMakeCodeStylePreferences m_qtCodeStyle;
};

class CMakeCodeStyleSettingsPage final : public Core::IOptionsPage
{
public:
    CMakeCodeStyleSettingsPage()
    {
        setId(Constants::Settings::CODE_STYLE_ID);
        setDisplayName(Tr::tr("Code Style"));
        setCategory(Constants::Settings::CATEGORY);
        setSettingsProvider([] {
            static CodeStyleAspect settings(
                codeStyleFactory(Constants::CMAKE_LANGUAGE_ID)->globalCodeStyle(),
                Constants::CMAKE_LANGUAGE_ID);
            return &settings;
        });
    }
};

void setupCMakeCodeStyle()
{
    static GuardedObject<CMakeCodeStylePreferencesFactory> theCMakeCodeStylePreferencesFactory;
    static GuardedObject<CMakeCodeStyleSettingsPage> theCMakeCodeStyleSettingsPage;
}

#ifdef WITH_TESTS

class CMakeCodeStyleTest final : public QObject
{
    Q_OBJECT

private slots:
    // The preview stands for what the editor does with a file, so the
    // formatter has to leave it as it is spelled: one it would change shows a
    // layout that the editor never writes.
    void previewShowsWhatTheSettingsDo()
    {
        // Only the keywords of the commands CMake itself brings, which is all
        // a preferences page can count on: no project is open behind it.
        const auto isKeyword = [](const QString &command, const QString &argument) {
            static const QStringList scopes{"PRIVATE", "PUBLIC", "INTERFACE"};
            return (command == "target_link_libraries"
                    || command == "target_include_directories")
                   && scopes.contains(argument);
        };

        CMakeLang::Style style;
        style.isKeyword = isKeyword;

        const QString preview = QLatin1String(codeStylePreview);
        QVERIFY(CMakeLang::formattingEdits(preview, style).isEmpty());

        // And every setting has to tell on it, or the preview does not show
        // the user what the setting is for.
        const QList<bool CMakeLang::Style::*> settings{
            &CMakeLang::Style::indentKeywordValues,
            &CMakeLang::Style::spaceBeforeControlParen,
            &CMakeLang::Style::spaceBeforeCommandParen,
            &CMakeLang::Style::keepCommentColumn,
        };
        for (bool CMakeLang::Style::*setting : settings) {
            CMakeLang::Style other = style;
            other.*setting = !(style.*setting);
            QVERIFY(!CMakeLang::formattingEdits(preview, other).isEmpty());
        }
    }
};

QObject *createCMakeCodeStyleTest()
{
    return new CMakeCodeStyleTest;
}

#endif

} // CMakeProjectManager::Internal

#ifdef WITH_TESTS
#include <cmakecodestyle.moc>
#endif
