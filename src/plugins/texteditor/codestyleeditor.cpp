/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    connect(codeStyle, SIGNAL(currentTabSettingsChanged(TextEditor::TabSettings)),
            this, SLOT(updatePreview()));
    connect(codeStyle, SIGNAL(currentValueChanged(QVariant)),
            this, SLOT(updatePreview()));
    connect(codeStyle, SIGNAL(currentPreferencesChanged(TextEditor::ICodeStylePreferences*)),
            this, SLOT(updatePreview()));
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
