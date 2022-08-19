// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "nimcodestylepreferenceswidget.h"
#include "ui_nimcodestylepreferenceswidget.h"

#include "../nimconstants.h"
#include "../editor/nimeditorfactory.h"

#include <extensionsystem/pluginmanager.h>
#include <texteditor/displaysettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/icodestylepreferences.h>
#include <texteditor/indenter.h>
#include <texteditor/tabsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/snippets/snippetprovider.h>

using namespace TextEditor;

namespace Nim {

NimCodeStylePreferencesWidget::NimCodeStylePreferencesWidget(ICodeStylePreferences *preferences, QWidget *parent)
    : TextEditor::CodeStyleEditorWidget(parent)
    , m_preferences(preferences)
    , m_ui(new Ui::NimCodeStylePreferencesWidget())
{
    m_ui->setupUi(this);
    m_ui->tabPreferencesWidget->setPreferences(preferences);
    m_ui->previewTextEdit->setPlainText(Nim::Constants::C_NIMCODESTYLEPREVIEWSNIPPET);

    decorateEditor(TextEditorSettings::fontSettings());
    connect(TextEditorSettings::instance(), &TextEditorSettings::fontSettingsChanged,
       this, &NimCodeStylePreferencesWidget::decorateEditor);

    connect(m_preferences, &ICodeStylePreferences::currentTabSettingsChanged,
            this, &NimCodeStylePreferencesWidget::updatePreview);

    setVisualizeWhitespace(true);

    updatePreview();
}

NimCodeStylePreferencesWidget::~NimCodeStylePreferencesWidget()
{
    delete m_ui;
    m_ui = nullptr;
}

void NimCodeStylePreferencesWidget::decorateEditor(const FontSettings &fontSettings)
{
    m_ui->previewTextEdit->textDocument()->setFontSettings(fontSettings);
    NimEditorFactory::decorateEditor(m_ui->previewTextEdit);
}

void NimCodeStylePreferencesWidget::setVisualizeWhitespace(bool on)
{
    DisplaySettings displaySettings = m_ui->previewTextEdit->displaySettings();
    displaySettings.m_visualizeWhitespace = on;
    m_ui->previewTextEdit->setDisplaySettings(displaySettings);
}

void NimCodeStylePreferencesWidget::updatePreview()
{
    QTextDocument *doc = m_ui->previewTextEdit->document();

    const TabSettings &ts = m_preferences
            ? m_preferences->currentTabSettings()
            : TextEditorSettings::codeStyle()->tabSettings();
    m_ui->previewTextEdit->textDocument()->setTabSettings(ts);

    QTextBlock block = doc->firstBlock();
    QTextCursor tc = m_ui->previewTextEdit->textCursor();
    tc.beginEditBlock();
    while (block.isValid()) {
        m_ui->previewTextEdit->textDocument()->indenter()->indentBlock(block, QChar::Null, ts);
        block = block.next();
    }
    tc.endEditBlock();
}

} // namespace Nim

