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

#include "genericeditor.h"
#include "generichighlighterconstants.h"
#include "genericeditorplugin.h"
#include "highlightdefinition.h"
#include "highlighter.h"
#include "highlighterexception.h"

#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/basetextdocument.h>

#include <QtCore/QSharedPointer>
#include <QtCore/QFileInfo>

using namespace Highlight;
using namespace Internal;

GenericEditorEditable::GenericEditorEditable(GenericEditor *editor) :
    TextEditor::BaseTextEditorEditable(editor)
{
    Core::UniqueIDManager *uidm = Core::UniqueIDManager::instance();
    m_context << uidm->uniqueIdentifier(Highlight::Constants::GENERIC_EDITOR);
    m_context << uidm->uniqueIdentifier(TextEditor::Constants::C_TEXTEDITOR);
}

QString GenericEditorEditable::id() const
{ return QLatin1String(Highlight::Constants::GENERIC_EDITOR); }

QList<int> GenericEditorEditable::context() const
{ return m_context; }

bool GenericEditorEditable::isTemporary() const
{ return false; }

bool GenericEditorEditable::duplicateSupported() const
{ return true; }

Core::IEditor *GenericEditorEditable::duplicate(QWidget *parent)
{
    GenericEditor *newEditor = new GenericEditor(editor()->mimeType(), parent);
    newEditor->duplicateFrom(editor());
    return newEditor->editableInterface();
}

bool GenericEditorEditable::open(const QString &fileName)
{
    if (TextEditor::BaseTextEditorEditable::open(fileName)) {
        editor()->setMimeType(
                Core::ICore::instance()->mimeDatabase()->findByFile(QFileInfo(fileName)).type());
        return true;
    }
    return false;
}

GenericEditor::GenericEditor(const QString &definitionId, QWidget *parent) :
    TextEditor::BaseTextEditor(parent)
{
    try {
        QSharedPointer<HighlightDefinition> definition =
            GenericEditorPlugin::instance()->definition(definitionId);
        baseTextDocument()->setSyntaxHighlighter(new Highlighter(definition->initialContext()));
    } catch (const HighlighterException &) {
        // No highlighter will be set.
    }
}

TextEditor::BaseTextEditorEditable *GenericEditor::createEditableInterface()
{
    GenericEditorEditable *editable = new GenericEditorEditable(this);
    return editable;
}
