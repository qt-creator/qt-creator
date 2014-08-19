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

#include "baseeditordocumentprocessor.h"
#include "cppworkingcopy.h"

#include "editordocumenthandle.h"

#include <utils/qtcassert.h>

#include <QTextBlock>

namespace CppTools {

/*!
    \class CppTools::BaseEditorDocumentProcessor

    \brief The BaseEditorDocumentProcessor class controls and executes all
           document relevant actions (reparsing, semantic highlighting, additional
           semantic calculations) after a text document has changed.
*/

BaseEditorDocumentProcessor::BaseEditorDocumentProcessor(
        TextEditor::BaseTextDocument *document)
    : m_baseTextDocument(document)
{
    QTC_CHECK(document);
}

BaseEditorDocumentProcessor::~BaseEditorDocumentProcessor()
{
}

TextEditor::BaseTextDocument *BaseEditorDocumentProcessor::baseTextDocument() const
{
    return m_baseTextDocument;
}

BaseEditorDocumentProcessor *BaseEditorDocumentProcessor::get(const QString &filePath)
{
    CppModelManagerInterface *cmmi = CppModelManagerInterface::instance();
    if (EditorDocumentHandle *editorDocument = cmmi->editorDocument(filePath))
        return editorDocument->processor();
    return 0;
}

QList<QTextEdit::ExtraSelection> BaseEditorDocumentProcessor::toTextEditorSelections(
        const QList<CPlusPlus::Document::DiagnosticMessage> &diagnostics,
        QTextDocument *textDocument)
{
    // Format for errors
    QTextCharFormat errorFormat;
    errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    errorFormat.setUnderlineColor(Qt::red);

    // Format for warnings
    QTextCharFormat warningFormat;
    warningFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    warningFormat.setUnderlineColor(Qt::darkYellow);

    QList<QTextEdit::ExtraSelection> result;
    foreach (const CPlusPlus::Document::DiagnosticMessage &m, diagnostics) {
        QTextEdit::ExtraSelection sel;
        if (m.isWarning())
            sel.format = warningFormat;
        else
            sel.format = errorFormat;

        QTextCursor c(textDocument->findBlockByNumber(m.line() - 1));
        const QString text = c.block().text();
        if (m.length() > 0 && m.column() + m.length() < (unsigned)text.size()) {
            int column = m.column() > 0 ? m.column() - 1 : 0;
            c.setPosition(c.position() + column);
            c.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, m.length());
        } else {
            for (int i = 0; i < text.size(); ++i) {
                if (!text.at(i).isSpace()) {
                    c.setPosition(c.position() + i);
                    break;
                }
            }
            c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        }
        sel.cursor = c;
        sel.format.setToolTip(m.text());
        result.append(sel);
    }

    return result;
}

void BaseEditorDocumentProcessor::runParser(QFutureInterface<void> &future,
                                            BaseEditorDocumentParser *parser,
                                            WorkingCopy workingCopy)
{
    future.setProgressRange(0, 1);
    if (future.isCanceled()) {
        future.setProgressValue(1);
        return;
    }

    parser->update(workingCopy);
    CppModelManagerInterface::instance()
        ->finishedRefreshingSourceFiles(QStringList(parser->filePath()));

    future.setProgressValue(1);
}

} // namespace CppTools
