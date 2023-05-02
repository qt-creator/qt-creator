// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codestyleeditor.h"

#include "textdocument.h"
#include "icodestylepreferencesfactory.h"
#include "icodestylepreferences.h"
#include "codestyleselectorwidget.h"
#include "texteditortr.h"
#include "displaysettings.h"
#include "tabsettings.h"
#include "indenter.h"
#include "snippets/snippeteditor.h"
#include "snippets/snippetprovider.h"

#include <QVBoxLayout>
#include <QTextBlock>
#include <QLabel>

namespace TextEditor {

CodeStyleEditor::CodeStyleEditor(ICodeStylePreferencesFactory *factory,
                                 ICodeStylePreferences *codeStyle,
                                 ProjectExplorer::Project *project,
                                 QWidget *parent)
    : CodeStyleEditorWidget(parent)
    , m_factory(factory)
    , m_codeStyle(codeStyle)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    auto selector = new CodeStyleSelectorWidget(factory, project, this);
    selector->setCodeStyle(codeStyle);
    m_additionalGlobalSettingsWidget = factory->createAdditionalGlobalSettings(codeStyle,
                                                                               project,
                                                                               parent);

    if (m_additionalGlobalSettingsWidget)
        m_layout->addWidget(m_additionalGlobalSettingsWidget);

    m_layout->addWidget(selector);
    if (!project) {
        m_widget = factory->createEditor(codeStyle, project, parent);

        if (m_widget)
            m_layout->addWidget(m_widget);
        return;
    }

    QLabel *label = new QLabel(
        Tr::tr("Edit preview contents to see how the current settings "
               "are applied to custom code snippets. Changes in the preview "
               "do not affect the current settings."), this);
    QFont font = label->font();
    font.setItalic(true);
    label->setFont(font);
    label->setWordWrap(true);

    m_preview = new SnippetEditorWidget(this);
    DisplaySettings displaySettings = m_preview->displaySettings();
    displaySettings.m_visualizeWhitespace = true;
    m_preview->setDisplaySettings(displaySettings);
    QString groupId = factory->snippetProviderGroupId();
    SnippetProvider::decorateEditor(m_preview, groupId);

    m_layout->addWidget(m_preview);
    m_layout->addWidget(label);
    connect(codeStyle, &ICodeStylePreferences::currentTabSettingsChanged,
            this, &CodeStyleEditor::updatePreview);
    connect(codeStyle, &ICodeStylePreferences::currentValueChanged,
            this, &CodeStyleEditor::updatePreview);
    connect(codeStyle, &ICodeStylePreferences::currentPreferencesChanged,
            this, &CodeStyleEditor::updatePreview);
    m_preview->setCodeStyle(m_codeStyle);
    m_preview->setPlainText(factory->previewText());

    updatePreview();
}

void CodeStyleEditor::updatePreview()
{
    QTextDocument *doc = m_preview->document();

    m_preview->textDocument()->indenter()->invalidateCache();

    QTextBlock block = doc->firstBlock();
    QTextCursor tc = m_preview->textCursor();
    tc.beginEditBlock();
    while (block.isValid()) {
        m_preview->textDocument()->indenter()->indentBlock(block,
                                                           QChar::Null,
                                                           m_codeStyle->currentTabSettings());
        block = block.next();
    }
    tc.endEditBlock();
}

void CodeStyleEditor::apply()
{
    if (m_widget)
        m_widget->apply();

    if (m_additionalGlobalSettingsWidget)
        m_additionalGlobalSettingsWidget->apply();
}

void CodeStyleEditor::finish()
{
    if (m_widget)
        m_widget->finish();

    if (m_additionalGlobalSettingsWidget)
        m_additionalGlobalSettingsWidget->finish();
}

} // TextEditor
