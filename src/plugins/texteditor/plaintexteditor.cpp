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
#include "manager.h"
#include "normalindenter.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

#include <QtCore/QSharedPointer>
#include <QtCore/QFileInfo>

using namespace TextEditor;
using namespace TextEditor::Internal;

PlainTextEditorEditable::PlainTextEditorEditable(PlainTextEditor *editor)
  : BaseTextEditorEditable(editor)
{
    Core::UniqueIDManager *uidm = Core::UniqueIDManager::instance();
    m_context << uidm->uniqueIdentifier(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID);
    m_context << uidm->uniqueIdentifier(TextEditor::Constants::C_TEXTEDITOR);
}

PlainTextEditor::PlainTextEditor(QWidget *parent)
  : BaseTextEditor(parent)
{
    setRevisionsVisible(true);
    setMarksVisible(true);
    setRequestMarkEnabled(false);
    setLineSeparatorsAllowed(true);

    setMimeType(QLatin1String(TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_TEXT));
    setDisplayName(tr(Core::Constants::K_DEFAULT_TEXT_EDITOR_DISPLAY_NAME));

    m_commentDefinition.clearCommentStyles();

    connect(file(), SIGNAL(changed()), this, SLOT(configure()));
}

PlainTextEditor::~PlainTextEditor()
{}

QList<int> PlainTextEditorEditable::context() const
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

void PlainTextEditor::setFontSettings(const FontSettings & fs)
{
    BaseTextEditor::setFontSettings(fs);

    if (baseTextDocument()->syntaxHighlighter()) {
        Highlighter *highlighter =
                static_cast<Highlighter *>(baseTextDocument()->syntaxHighlighter());
        highlighter->configureFormats(fs);
        highlighter->rehighlight();
    }
}

void PlainTextEditor::configure()
{
    configure(Core::ICore::instance()->mimeDatabase()->findByFile(file()->fileName()));
}

void PlainTextEditor::configure(const Core::MimeType &mimeType)
{
    if (mimeType.isNull())
        return;

    const QString &type = mimeType.type();
    setMimeType(type);

    QString definitionId = Manager::instance()->definitionIdByMimeType(type);
    if (definitionId.isEmpty())
        definitionId = findDefinitionId(mimeType, true);

    if (!definitionId.isEmpty()) {
        try {
            const QSharedPointer<HighlightDefinition> &definition =
                    Manager::instance()->definition(definitionId);

            Highlighter *highlighter = new Highlighter(definition->initialContext());
            highlighter->configureFormats(TextEditorSettings::instance()->fontSettings());

            baseTextDocument()->setSyntaxHighlighter(highlighter);

            m_commentDefinition.setAfterWhiteSpaces(definition->isCommentAfterWhiteSpaces());
            m_commentDefinition.setSingleLine(definition->singleLineComment());
            m_commentDefinition.setMultiLineStart(definition->multiLineCommentStart());
            m_commentDefinition.setMultiLineEnd(definition->multiLineCommentEnd());
        } catch (const HighlighterException &) {
        }
    }

    // @todo: Indentation specification through the definition files is not really being used
    // because Kate recommends to configure indentation  through another feature. Maybe we should
    // provide something similar in Creator? For now, only normal indentation is supported.
    m_indenter.reset(new NormalIndenter);
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
