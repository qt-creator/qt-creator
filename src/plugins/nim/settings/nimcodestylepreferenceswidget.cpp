// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimcodestylepreferenceswidget.h"

#include "../nimconstants.h"
#include "../editor/nimeditorfactory.h"

#include <extensionsystem/pluginmanager.h>

#include <texteditor/displaysettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/icodestylepreferences.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/indenter.h>
#include <texteditor/snippets/snippeteditor.h>
#include <texteditor/snippets/snippetprovider.h>
#include <texteditor/simplecodestylepreferenceswidget.h>
#include <texteditor/tabsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorsettings.h>

#include <utils/layoutbuilder.h>

using namespace TextEditor;

namespace Nim {

NimCodeStylePreferencesWidget::NimCodeStylePreferencesWidget(ICodeStylePreferences *preferences, QWidget *parent)
    : TextEditor::CodeStyleEditorWidget(parent)
    , m_preferences(preferences)
{
    auto tabPreferencesWidget = new SimpleCodeStylePreferencesWidget;
    tabPreferencesWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    tabPreferencesWidget->setPreferences(preferences);

    m_previewTextEdit = new SnippetEditorWidget;
    m_previewTextEdit->setPlainText(Nim::Constants::C_NIMCODESTYLEPREVIEWSNIPPET);

    using namespace Layouting;
    Row {
        Column {
            tabPreferencesWidget,
            st,
        },
        m_previewTextEdit,
        noMargin,
    }.attachTo(this);

    decorateEditor(TextEditorSettings::fontSettings());
    connect(TextEditorSettings::instance(), &TextEditorSettings::fontSettingsChanged,
       this, &NimCodeStylePreferencesWidget::decorateEditor);

    connect(m_preferences, &ICodeStylePreferences::currentTabSettingsChanged,
            this, &NimCodeStylePreferencesWidget::updatePreview);

    setVisualizeWhitespace(true);

    updatePreview();
}

NimCodeStylePreferencesWidget::~NimCodeStylePreferencesWidget() = default;

void NimCodeStylePreferencesWidget::decorateEditor(const FontSettings &fontSettings)
{
    m_previewTextEdit->textDocument()->setFontSettings(fontSettings);
    NimEditorFactory::decorateEditor(m_previewTextEdit);
}

void NimCodeStylePreferencesWidget::setVisualizeWhitespace(bool on)
{
    DisplaySettings displaySettings = m_previewTextEdit->displaySettings();
    displaySettings.m_visualizeWhitespace = on;
    m_previewTextEdit->setDisplaySettings(displaySettings);
}

void NimCodeStylePreferencesWidget::updatePreview()
{
    QTextDocument *doc = m_previewTextEdit->document();

    const TabSettings &ts = m_preferences
            ? m_preferences->currentTabSettings()
            : TextEditorSettings::codeStyle()->tabSettings();
    m_previewTextEdit->textDocument()->setTabSettings(ts);

    QTextBlock block = doc->firstBlock();
    QTextCursor tc = m_previewTextEdit->textCursor();
    tc.beginEditBlock();
    while (block.isValid()) {
        m_previewTextEdit->textDocument()->indenter()->indentBlock(block, QChar::Null, ts);
        block = block.next();
    }
    tc.endEditBlock();
}

} // Nim
