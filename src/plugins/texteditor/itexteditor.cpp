/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "basetexteditor.h"
#include <coreplugin/editormanager/editormanager.h>

#include <QTextCodec>

namespace TextEditor {

/*!
    \class TextEditor::BaseTextEditorDocument
    \brief The BaseTextEditorDocument class is an abstract base for documents of text editors.

    It is the base class for documents used by implementations of the BaseTextEditor class,
    and contains basic functions for retrieving text content and markers (like bookmarks).
*/

/*!
    \class TextEditor::BaseTextEditor
    \brief The BaseTextEditor class is an abstract base class for text editors.

    It contains the basic functions for retrieving and setting cursor position and selections,
    and operations on them, like removing or inserting. It uses implementations of
    BaseTextEditorDocument as the underlying document.
*/

BaseTextEditorDocument::BaseTextEditorDocument(QObject *parent)
    : Core::TextDocument(parent)
{
}

QMap<QString, QString> BaseTextEditorDocument::openedTextDocumentContents()
{
    QMap<QString, QString> workingCopy;
    foreach (Core::IDocument *document, Core::DocumentModel::openedDocuments()) {
        BaseTextEditorDocument *textEditorDocument = qobject_cast<BaseTextEditorDocument *>(document);
        if (!textEditorDocument)
            continue;
        QString fileName = textEditorDocument->filePath();
        workingCopy[fileName] = textEditorDocument->plainText();
    }
    return workingCopy;
}

QMap<QString, QTextCodec *> BaseTextEditorDocument::openedTextDocumentEncodings()
{
    QMap<QString, QTextCodec *> workingCopy;
    foreach (Core::IDocument *document, Core::DocumentModel::openedDocuments()) {
        BaseTextEditorDocument *textEditorDocument = qobject_cast<BaseTextEditorDocument *>(document);
        if (!textEditorDocument)
            continue;
        QString fileName = textEditorDocument->filePath();
        workingCopy[fileName] = const_cast<QTextCodec *>(textEditorDocument->codec());
    }
    return workingCopy;
}

BaseTextEditorDocument *BaseTextEditor::textDocument()
{
    return qobject_cast<BaseTextEditorDocument *>(document());
}


BaseTextEditor *BaseTextEditor::currentTextEditor()
{
    return qobject_cast<BaseTextEditor *>(Core::EditorManager::currentEditor());
}

} // namespace TextEditor
