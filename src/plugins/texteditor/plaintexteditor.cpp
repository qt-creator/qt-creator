/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "plaintexteditor.h"
#include "tabsettings.h"
#include "texteditorconstants.h"
#include "texteditorplugin.h"
#include "texteditorsettings.h"
#include "basetextdocument.h"
#include "highlightdefinition.h"
#include "highlighter.h"
#include "highlighterexception.h"
#include "highlightersettings.h"
#include "manager.h"
#include "context.h"
#include "normalindenter.h"
#include "fontsettings.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

#include <QtCore/QSharedPointer>
#include <QtCore/QFileInfo>

#include <QDebug>

using namespace TextEditor;
using namespace TextEditor::Internal;

PlainTextEditorEditable::PlainTextEditorEditable(PlainTextEditor *editor)
  : BaseTextEditorEditable(editor),
    m_context(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, TextEditor::Constants::C_TEXTEDITOR)
{
}

PlainTextEditor::PlainTextEditor(QWidget *parent)
  : BaseTextEditor(parent),
  m_isMissingSyntaxDefinition(true)
{
    setRevisionsVisible(true);
    setMarksVisible(true);
    setRequestMarkEnabled(false);
    setLineSeparatorsAllowed(true);

    setMimeType(QLatin1String(TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_TEXT));
    setDisplayName(tr(Core::Constants::K_DEFAULT_TEXT_EDITOR_DISPLAY_NAME));

    m_commentDefinition.clearCommentStyles();

    connect(file(), SIGNAL(changed()), this, SLOT(fileChanged()));
}

PlainTextEditor::~PlainTextEditor()
{}

Core::Context PlainTextEditorEditable::context() const
{
    return m_context;
}

Core::IEditor *PlainTextEditorEditable::duplicate(QWidget *parent)
{
    PlainTextEditor *newEditor = new PlainTextEditor(parent);
    newEditor->duplicateFrom(editor());
    TextEditorPlugin::instance()->initializeEditor(newEditor);
    return newEditor->editableInterface();
}

QString PlainTextEditorEditable::id() const
{
    return QLatin1String(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID);
}

void PlainTextEditor::unCommentSelection()
{
    Utils::unCommentSelection(this, m_commentDefinition);
}

void PlainTextEditor::setFontSettings(const FontSettings &fs)
{
    BaseTextEditor::setFontSettings(fs);

    if (baseTextDocument()->syntaxHighlighter()) {
        Highlighter *highlighter =
            static_cast<Highlighter *>(baseTextDocument()->syntaxHighlighter());

        highlighter->configureFormat(Highlighter::VisualWhitespace, fs.toTextCharFormat(
            QLatin1String(Constants::C_VISUAL_WHITESPACE)));
        highlighter->configureFormat(Highlighter::Keyword, fs.toTextCharFormat(
            QLatin1String(Constants::C_KEYWORD)));
        highlighter->configureFormat(Highlighter::DataType, fs.toTextCharFormat(
            QLatin1String(Constants::C_TYPE)));
        highlighter->configureFormat(Highlighter::Comment, fs.toTextCharFormat(
            QLatin1String(Constants::C_COMMENT)));
        // Using C_NUMBER for all kinds of numbers.
        highlighter->configureFormat(Highlighter::Decimal, fs.toTextCharFormat(
            QLatin1String(Constants::C_NUMBER)));
        highlighter->configureFormat(Highlighter::BaseN, fs.toTextCharFormat(
            QLatin1String(Constants::C_NUMBER)));
        highlighter->configureFormat(Highlighter::Float, fs.toTextCharFormat(
            QLatin1String(Constants::C_NUMBER)));
        // Using C_STRING for strings and chars.
        highlighter->configureFormat(Highlighter::Char, fs.toTextCharFormat(
            QLatin1String(Constants::C_STRING)));
        highlighter->configureFormat(Highlighter::String, fs.toTextCharFormat(
            QLatin1String(Constants::C_STRING)));

        // Creator does not have corresponding formats for the following ones. Implement them?
        // For now I will leave hardcoded values.
        QTextCharFormat format;
        format.setForeground(Qt::blue);
        highlighter->configureFormat(Highlighter::Others, format);
        format.setForeground(Qt::red);
        highlighter->configureFormat(Highlighter::Alert, format);
        format.setForeground(Qt::darkBlue);
        highlighter->configureFormat(Highlighter::Function, format);
        format.setForeground(Qt::darkGray);
        highlighter->configureFormat(Highlighter::RegionMarker, format);
        format.setForeground(Qt::darkRed);
        highlighter->configureFormat(Highlighter::Error, format);

        highlighter->rehighlight();
    }
}

void PlainTextEditor::setTabSettings(const TextEditor::TabSettings &ts)
{
    BaseTextEditor::setTabSettings(ts);

    if (baseTextDocument()->syntaxHighlighter()) {
        Highlighter *highlighter =
            static_cast<Highlighter *>(baseTextDocument()->syntaxHighlighter());
        highlighter->setTabSettings(ts);
    }
}

void PlainTextEditor::fileChanged()
{
    configure(Core::ICore::instance()->mimeDatabase()->findByFile(file()->fileName()));
}

void PlainTextEditor::configure(const Core::MimeType &mimeType)
{
    Highlighter *highlighter = new Highlighter();
    baseTextDocument()->setSyntaxHighlighter(highlighter);

    m_isMissingSyntaxDefinition = true;
    setCodeFoldingSupported(false);
    setCodeFoldingVisible(false);

    QString definitionId;
    if (!mimeType.isNull()) {
        const QString &type = mimeType.type();
        setMimeType(type);

        definitionId = Manager::instance()->definitionIdByMimeType(type);
        if (definitionId.isEmpty())
            definitionId = findDefinitionId(mimeType, true);
    }

    if (!definitionId.isEmpty()) {
        m_isMissingSyntaxDefinition = false;
        const QSharedPointer<HighlightDefinition> &definition =
            Manager::instance()->definition(definitionId);
        if (!definition.isNull()) {
            highlighter->setDefaultContext(definition->initialContext());

            m_commentDefinition.setAfterWhiteSpaces(definition->isCommentAfterWhiteSpaces());
            m_commentDefinition.setSingleLine(definition->singleLineComment());
            m_commentDefinition.setMultiLineStart(definition->multiLineCommentStart());
            m_commentDefinition.setMultiLineEnd(definition->multiLineCommentEnd());

            setCodeFoldingSupported(true);
            setCodeFoldingVisible(true);
        }
    } else if (file()) {
        const QString &fileName = file()->fileName();
        if (TextEditorSettings::instance()->highlighterSettings().isIgnoredFilePattern(fileName))
            m_isMissingSyntaxDefinition = false;
    }

    setFontSettings(TextEditorSettings::instance()->fontSettings());

    // @todo: Indentation specification through the definition files is not really being used
    // because Kate recommends to configure indentation  through another feature. Maybe we should
    // provide something similar in Creator? For now, only normal indentation is supported.
    m_indenter.reset(new NormalIndenter);
}

bool PlainTextEditor::isMissingSyntaxDefinition() const
{
    return m_isMissingSyntaxDefinition;
}

QString PlainTextEditor::findDefinitionId(const Core::MimeType &mimeType,
                                          bool considerParents) const
{
    QString definitionId = Manager::instance()->definitionIdByAnyMimeType(mimeType.aliases());
    if (definitionId.isEmpty() && considerParents) {
        definitionId = Manager::instance()->definitionIdByAnyMimeType(mimeType.subClassesOf());
        if (definitionId.isEmpty()) {
            foreach (const QString &parent, mimeType.subClassesOf()) {
                const Core::MimeType &parentMimeType =
                    Core::ICore::instance()->mimeDatabase()->findByType(parent);
                definitionId = findDefinitionId(parentMimeType, considerParents);
            }
        }
    }
    return definitionId;
}

void PlainTextEditor::indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar)
{
    m_indenter->indentBlock(doc, block, typedChar, tabSettings());
}
