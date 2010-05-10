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

#include "editor.h"
#include "genericeditorconstants.h"
#include "genericeditorplugin.h"
#include "highlightdefinition.h"
#include "highlighter.h"
#include "highlighterexception.h"

#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/normalindenter.h>

#include <QtCore/QSharedPointer>
#include <QtCore/QFileInfo>

#include <QDebug>

using namespace GenericEditor;
using namespace Internal;

Editor::Editor(QWidget *parent) : TextEditor::BaseTextEditor(parent)
{
    connect(file(), SIGNAL(changed()), this, SLOT(configure()));
}

Editor::~Editor()
{}

void Editor::unCommentSelection()
{
    Utils::unCommentSelection(this, m_commentDefinition);
}

TextEditor::BaseTextEditorEditable *Editor::createEditableInterface()
{
    EditorEditable *editable = new EditorEditable(this);
    return editable;
}

void Editor::setFontSettings(const TextEditor::FontSettings & fs)
{
    TextEditor::BaseTextEditor::setFontSettings(fs);

    Highlighter *highlighter = static_cast<Highlighter *>(baseTextDocument()->syntaxHighlighter());
    if (!highlighter)
        return;

    highlighter->configureFormats(fs);
    highlighter->rehighlight();
}

void Editor::indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar)
{
    m_indenter->indentBlock(doc, block, typedChar, tabSettings());
}

void Editor::configure()
{
    const QString &mimeType = Core::ICore::instance()->mimeDatabase()->findByFile(
            QFileInfo(file()->fileName())).type();
    baseTextDocument()->setMimeType(mimeType);

    try {
        const QString &definitionId =
                GenericEditorPlugin::instance()->definitionIdByMimeType(mimeType);
        QSharedPointer<HighlightDefinition> definition =
                GenericEditorPlugin::instance()->definition(definitionId);

        Highlighter *highlighter = new Highlighter(definition->initialContext());
        highlighter->configureFormats(TextEditor::TextEditorSettings::instance()->fontSettings());

        baseTextDocument()->setSyntaxHighlighter(highlighter);

        m_commentDefinition.setAfterWhiteSpaces(definition->isCommentAfterWhiteSpaces());
        m_commentDefinition.setSingleLine(definition->singleLineComment());
        m_commentDefinition.setMultiLineStart(definition->multiLineCommentStart());
        m_commentDefinition.setMultiLineEnd(definition->multiLineCommentEnd());

        //@todo: It's possible to specify an indenter style in the definition file. However, this
        // is not really being used because Kate recommends to configure indentation through
        // another editor feature. Maybe we should provide something similar in Creator?
        // For now the normal indenter is used.
        m_indenter.reset(new TextEditor::NormalIndenter);
    } catch (const HighlighterException &) {
        // No highlighter will be set.
    }
}

EditorEditable::EditorEditable(Editor *editor) :
    TextEditor::BaseTextEditorEditable(editor)
{
    Core::UniqueIDManager *uidm = Core::UniqueIDManager::instance();
    m_context << uidm->uniqueIdentifier(GenericEditor::Constants::GENERIC_EDITOR);
    m_context << uidm->uniqueIdentifier(TextEditor::Constants::C_TEXTEDITOR);
}

QString EditorEditable::id() const
{ return QLatin1String(GenericEditor::Constants::GENERIC_EDITOR); }

QList<int> EditorEditable::context() const
{ return m_context; }

bool EditorEditable::isTemporary() const
{ return false; }

bool EditorEditable::duplicateSupported() const
{ return true; }

Core::IEditor *EditorEditable::duplicate(QWidget *parent)
{
    Editor *newEditor = new Editor(parent);
    newEditor->duplicateFrom(editor());
    return newEditor->editableInterface();
}
