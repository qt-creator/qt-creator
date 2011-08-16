/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "codestyleeditor.h"

#include "icodestylepreferencesfactory.h"
#include "icodestylepreferences.h"
#include "codestyleselectorwidget.h"
#include "texteditorsettings.h"
#include "displaysettings.h"
#include "tabsettings.h"
#include "indenter.h"
#include "snippets/snippeteditor.h"
#include "snippets/isnippetprovider.h"
#include <QtGui/QVBoxLayout>
#include <QtGui/QTextBlock>

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
    TextEditor::TextEditorSettings *settings = TextEditorSettings::instance();
    m_preview->setFontSettings(settings->fontSettings());
    DisplaySettings displaySettings = m_preview->displaySettings();
    displaySettings.m_visualizeWhitespace = true;
    m_preview->setDisplaySettings(displaySettings);
    ISnippetProvider *provider = factory->snippetProvider();
    if (provider)
        provider->decorateEditor(m_preview);
    m_layout->addWidget(selector);
    m_layout->addWidget(m_preview);
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

    m_preview->indenter()->invalidateCache(doc);

    QTextBlock block = doc->firstBlock();
    QTextCursor tc = m_preview->textCursor();
    tc.beginEditBlock();
    while (block.isValid()) {
        m_preview->indenter()->indentBlock(doc, block, QChar::Null, m_codeStyle->currentTabSettings());

        block = block.next();
    }
    tc.endEditBlock();
}
