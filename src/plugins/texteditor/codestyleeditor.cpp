/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "codestyleeditor.h"

#include "textdocument.h"
#include "icodestylepreferencesfactory.h"
#include "icodestylepreferences.h"
#include "codestyleselectorwidget.h"
#include "texteditorsettings.h"
#include "displaysettings.h"
#include "tabsettings.h"
#include "indenter.h"
#include "snippets/snippeteditor.h"
#include "snippets/isnippetprovider.h"
#include <QVBoxLayout>
#include <QTextBlock>
#include <QLabel>

using namespace TextEditor;

CodeStyleEditor::CodeStyleEditor(ICodeStylePreferencesFactory *factory,
                                 ICodeStylePreferences *codeStyle, QWidget *parent)
    : QWidget(parent),
      m_factory(factory),
      m_codeStyle(codeStyle)
{
    m_layout = new QVBoxLayout(this);
    CodeStyleSelectorWidget *selector = new CodeStyleSelectorWidget(factory, this);
    selector->setCodeStyle(codeStyle);
    m_preview = new SnippetEditorWidget(this);
    DisplaySettings displaySettings = m_preview->displaySettings();
    displaySettings.m_visualizeWhitespace = true;
    m_preview->setDisplaySettings(displaySettings);
    ISnippetProvider *provider = factory->snippetProvider();
    if (provider)
        provider->decorateEditor(m_preview);
    QLabel *label = new QLabel(
                tr("Edit preview contents to see how the current settings "
                "are applied to custom code snippets. Changes in the preview "
                "do not affect the current settings."), this);
    QFont font = label->font();
    font.setItalic(true);
    label->setFont(font);
    label->setWordWrap(true);
    m_layout->addWidget(selector);
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

void CodeStyleEditor::clearMargins()
{
    m_layout->setContentsMargins(QMargins());
}

void CodeStyleEditor::updatePreview()
{
    QTextDocument *doc = m_preview->document();

    m_preview->textDocument()->indenter()->invalidateCache(doc);

    QTextBlock block = doc->firstBlock();
    QTextCursor tc = m_preview->textCursor();
    tc.beginEditBlock();
    while (block.isValid()) {
        m_preview->textDocument()->indenter()
                ->indentBlock(doc, block, QChar::Null, m_codeStyle->currentTabSettings());
        block = block.next();
    }
    tc.endEditBlock();
}
