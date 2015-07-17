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

#include "baseeditordocumentprocessor.h"
#include "cppworkingcopy.h"

#include "cpptoolsreuse.h"
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
        TextEditor::TextDocument *document)
    : m_baseTextDocument(document)
{
    QTC_CHECK(document);
}

BaseEditorDocumentProcessor::~BaseEditorDocumentProcessor()
{
}

TextEditor::TextDocument *BaseEditorDocumentProcessor::baseTextDocument() const
{
    return m_baseTextDocument;
}

BaseEditorDocumentProcessor *BaseEditorDocumentProcessor::get(const QString &filePath)
{
    CppModelManager *cmmi = CppModelManager::instance();
    if (CppEditorDocumentHandle *cppEditorDocument = cmmi->cppEditorDocument(filePath))
        return cppEditorDocument->processor();
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
        const int startPos = m.column() > 0 ? m.column() - 1 : 0;
        if (m.length() > 0 && startPos + m.length() <= (unsigned)text.size()) {
            c.setPosition(c.position() + startPos);
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
                                            BaseEditorDocumentParser::InMemoryInfo info)
{
    future.setProgressRange(0, 1);
    if (future.isCanceled()) {
        future.setProgressValue(1);
        return;
    }

    parser->update(info);
    CppModelManager::instance()
        ->finishedRefreshingSourceFiles(QSet<QString>() << parser->filePath());

    future.setProgressValue(1);
}

} // namespace CppTools
