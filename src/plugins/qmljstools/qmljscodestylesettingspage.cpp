// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljscodestylesettingspage.h"

#include "qmlformatsettings.h"
#include "qmlformatsettingswidget.h"
#include "qmljscodestylepreferenceswidget.h"
#include "qmljscodestylesettings.h"
#include "qmljscustomformatterwidget.h"
#include "qmljsformatterselectionwidget.h"
#include "qmljsqtstylecodeformatter.h"
#include "qmljstoolsconstants.h"
#include "qmljstoolssettings.h"
#include "qmljstoolstr.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <texteditor/codestyleeditor.h>
#include <texteditor/command.h>
#include <texteditor/displaysettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/formattexteditor.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/simplecodestylepreferenceswidget.h>
#include <texteditor/snippets/snippeteditor.h>
#include <texteditor/snippets/snippetprovider.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>
#include <utils/commandline.h>
#include <utils/filepath.h>
#include <utils/layoutbuilder.h>

#include <QTextStream>
#include <QVBoxLayout>
#include <QStandardPaths>

using namespace TextEditor;
using namespace QmlJSTools;
namespace QmlJSTools::Internal {

constexpr int BuiltinFormatterIndex = QmlCodeStyleWidgetBase::Builtin;
constexpr int QmlFormatIndex = QmlCodeStyleWidgetBase::QmlFormat;
constexpr int CustomFormatterIndex = QmlCodeStyleWidgetBase::Custom;

// QmlJSCodeStylePreferencesWidget

QmlJSCodeStylePreferencesWidget::QmlJSCodeStylePreferencesWidget(
    const QString &previewText, QWidget *parent)
    : TextEditor::CodeStyleEditorWidget(parent)
    , m_formatterSelectionWidget(new FormatterSelectionWidget(this))
    , m_formatterSettingsStack(new QStackedWidget(this))
{
    m_formatterSettingsStack->insertWidget(BuiltinFormatterIndex, new BuiltinFormatterSettingsWidget(this, m_formatterSelectionWidget));
    m_formatterSettingsStack->insertWidget(QmlFormatIndex, new QmlFormatSettingsWidget(this, m_formatterSelectionWidget));
    m_formatterSettingsStack->insertWidget(CustomFormatterIndex, new CustomFormatterWidget(this, m_formatterSelectionWidget));

    for (const auto &formatterWidget :
         m_formatterSettingsStack->findChildren<QmlCodeStyleWidgetBase *>()) {
        connect(
            formatterWidget,
            &QmlCodeStyleWidgetBase::settingsChanged,
            this,
            &QmlJSCodeStylePreferencesWidget::slotSettingsChanged);
    }

    const int index = m_formatterSelectionWidget->selection().value();
    m_formatterSettingsStack->setCurrentIndex(index);

    m_previewTextEdit = new SnippetEditorWidget(this);
    m_previewTextEdit->setPlainText(previewText);
    QSizePolicy sp(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    sp.setHorizontalStretch(1);
    m_previewTextEdit->setSizePolicy(sp);
    decorateEditor(TextEditorSettings::fontSettings());

    connect(
        TextEditorSettings::instance(),
        &TextEditorSettings::fontSettingsChanged,
        this,
        &QmlJSCodeStylePreferencesWidget::decorateEditor);

    connect(
        m_formatterSelectionWidget,
        &FormatterSelectionWidget::settingsChanged,
        [this](const QmlJSCodeStyleSettings &settings) {
            int index = m_formatterSelectionWidget->selection().volatileValue();
            if (index < 0 || index >= static_cast<int>(m_formatterSettingsStack->count()))
                return;

            m_formatterSettingsStack->setCurrentIndex(index);
            if (auto *current = dynamic_cast<QmlCodeStyleWidgetBase *>(
                    m_formatterSettingsStack->widget(index))) {
                current->slotCurrentPreferencesChanged(m_preferences);
            }
            slotSettingsChanged(settings);
        });

    using namespace Layouting;
    Row {
        Column {
            m_formatterSelectionWidget, br,
            m_formatterSettingsStack,
            st
        },
        m_previewTextEdit,
        noMargin
    }.attachTo(this);

    setVisualizeWhitespace(true);

    updatePreview();
}

void QmlJSCodeStylePreferencesWidget::setPreferences(QmlJSCodeStylePreferences *preferences)
{
    m_preferences = preferences;
    m_formatterSelectionWidget->setPreferences(preferences);
    for (const auto &formatterWidget :
         m_formatterSettingsStack->findChildren<QmlCodeStyleWidgetBase *>()) {
        formatterWidget->setPreferences(preferences);
    }
    if (m_preferences)
    {
        connect(m_preferences, &ICodeStylePreferences::currentTabSettingsChanged,
                this, &QmlJSCodeStylePreferencesWidget::updatePreview);
        connect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged,
                [this]{
                    m_formatterSettingsStack->setCurrentIndex(m_formatterSelectionWidget->selection().value());
                    updatePreview();
                });
    }
    updatePreview();
}

void QmlJSCodeStylePreferencesWidget::decorateEditor(const FontSettings &fontSettings)
{
    m_previewTextEdit->textDocument()->setFontSettings(fontSettings);
    SnippetProvider::decorateEditor(m_previewTextEdit,
                                    QmlJSEditor::Constants::QML_SNIPPETS_GROUP_ID);
}

void QmlJSCodeStylePreferencesWidget::setVisualizeWhitespace(bool on)
{
    DisplaySettings displaySettings = m_previewTextEdit->displaySettings();
    displaySettings.m_visualizeWhitespace = on;
    m_previewTextEdit->setDisplaySettings(displaySettings);
}

void QmlJSCodeStylePreferencesWidget::slotSettingsChanged(const QmlJSCodeStyleSettings &settings)
{
    if (!m_preferences)
        return;

    QmlJSCodeStylePreferences *current = dynamic_cast<QmlJSCodeStylePreferences*>(m_preferences->currentPreferences());
    if (!current)
        return;

    current->setCodeStyleSettings(settings);

    updatePreview();
}

void QmlJSCodeStylePreferencesWidget::updatePreview()
{
    switch (m_formatterSelectionWidget->selection().value()) {
    case QmlCodeStyleWidgetBase::Builtin:
        builtInFormatterPreview();
        break;
    case QmlCodeStyleWidgetBase::QmlFormat:
        qmlformatPreview();
        break;
    case QmlCodeStyleWidgetBase::Custom:
        customFormatterPreview();
        break;
    }
}

void QmlJSCodeStylePreferencesWidget::builtInFormatterPreview()
{
    QTextDocument *doc = m_previewTextEdit->document();

    const TabSettings &ts = m_preferences
            ? m_preferences->currentTabSettings()
            : TextEditorSettings::codeStyle()->tabSettings();
    m_previewTextEdit->textDocument()->setTabSettings(ts);
    CreatorCodeFormatter formatter(ts);
    formatter.invalidateCache(doc);

    QTextBlock block = doc->firstBlock();
    QTextCursor tc = m_previewTextEdit->textCursor();
    tc.beginEditBlock();
    while (block.isValid()) {
        m_previewTextEdit->textDocument()->indenter()->indentBlock(block, QChar::Null, ts);
        block = block.next();
    }
    tc.endEditBlock();
}

void QmlJSCodeStylePreferencesWidget::qmlformatPreview()
{
    using namespace Core;
    const Utils::FilePath &qmlFormatPath = QmlFormatSettings::instance().latestQmlFormatPath();
    if (qmlFormatPath.isEmpty()) {
        MessageManager::writeSilently("QmlFormat not found.");
        return;
    }
    const Utils::CommandLine commandLine(qmlFormatPath);
    TextEditor::Command command;
    command.setExecutable(commandLine.executable());
    command.setProcessing(TextEditor::Command::FileProcessing);
    command.addOptions(commandLine.splitArguments());
    command.addOption("--inplace");
    command.addOption("%file");
    if (!command.isValid())
        return;
    TextEditor::TabSettings tabSettings;
    tabSettings.m_tabSize = 4;
    QSettings settings(
        QmlJSTools::QmlFormatSettings::globalQmlFormatIniFile().toUrlishString(),
        QSettings::IniFormat);
    if (settings.contains("IndentWidth"))
        tabSettings.m_indentSize = settings.value("IndentWidth").toInt();
    if (settings.contains("UseTabs"))
        tabSettings.m_tabPolicy = settings.value("UseTabs").toBool()
                                        ? TextEditor::TabSettings::TabPolicy::TabsOnlyTabPolicy
                                        : TextEditor::TabSettings::TabPolicy::SpacesOnlyTabPolicy;
    QString dummyFilePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/dummy.qml";
    m_previewTextEdit->textDocument()->setFilePath(Utils::FilePath::fromString(dummyFilePath));
    m_previewTextEdit->textDocument()->setTabSettings(tabSettings);
    TextEditor::formatEditor(m_previewTextEdit, command);
}

void QmlJSCodeStylePreferencesWidget::customFormatterPreview()
{
    Utils::FilePath path = m_preferences->currentCodeStyleSettings().customFormatterPath;
    QStringList args = m_preferences->currentCodeStyleSettings()
                           .customFormatterArguments.split(" ", Qt::SkipEmptyParts);
    if (path.isEmpty()) {
        Core::MessageManager::writeSilently("Custom formatter not found.");
        return;
    }
    const Utils::CommandLine commandLine(path, args);
    TextEditor::Command command;
    command.setExecutable(commandLine.executable());
    command.setProcessing(TextEditor::Command::FileProcessing);
    command.addOptions(commandLine.splitArguments());
    command.addOption("--inplace");
    command.addOption("%file");
    if (!command.isValid())
        return;

    QString dummyFilePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/dummy.qml";
    m_previewTextEdit->textDocument()->setFilePath(Utils::FilePath::fromString(dummyFilePath));
    TextEditor::formatEditor(m_previewTextEdit, command);
}

// QmlJSCodeStyleSettingsPageWidget

class QmlJSCodeStyleSettingsPageWidget : public Core::IOptionsPageWidget
{
public:
    QmlJSCodeStyleSettingsPageWidget()
    {

        QmlJSCodeStylePreferences *originalPreferences
                = QmlJSToolsSettings::globalCodeStyle();
        m_preferences.setDelegatingPool(originalPreferences->delegatingPool());
        m_preferences.setCodeStyleSettings(originalPreferences->codeStyleSettings());
        m_preferences.setTabSettings(originalPreferences->tabSettings());
        m_preferences.setCurrentDelegate(originalPreferences->currentDelegate());
        m_preferences.setId(originalPreferences->id());

        auto vbox = new QVBoxLayout(this);
        vbox->addWidget(
            TextEditorSettings::codeStyleFactory(QmlJSTools::Constants::QML_JS_SETTINGS_ID)
                ->createCodeStyleEditor({}, &m_preferences));
    }

    void apply() final
    {
        QmlJSCodeStylePreferences *originalPreferences = QmlJSToolsSettings::globalCodeStyle();
        if (originalPreferences->codeStyleSettings() != m_preferences.codeStyleSettings()) {
            originalPreferences->setCodeStyleSettings(m_preferences.codeStyleSettings());
            originalPreferences->toSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
        }
        if (originalPreferences->tabSettings() != m_preferences.tabSettings()) {
            originalPreferences->setTabSettings(m_preferences.tabSettings());
            originalPreferences->toSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
        }
        if (originalPreferences->currentDelegate() != m_preferences.currentDelegate()) {
            originalPreferences->setCurrentDelegate(m_preferences.currentDelegate());
            originalPreferences->toSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
        }
    }

      QmlJSCodeStylePreferences m_preferences;
};

// QmlJSCodeStyleSettingsPage

QmlJSCodeStyleSettingsPage::QmlJSCodeStyleSettingsPage()
{
    setId(Constants::QML_JS_CODE_STYLE_SETTINGS_ID);
    setDisplayName(Tr::tr(Constants::QML_JS_CODE_STYLE_SETTINGS_NAME));
    setCategory(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML);
    setWidgetCreator([] { return new QmlJSCodeStyleSettingsPageWidget; });
}

} // QmlJSTools::Internal
