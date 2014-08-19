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

#ifndef BASEEDITORDOCUMENTPROCESSOR_H
#define BASEEDITORDOCUMENTPROCESSOR_H

#include "baseeditordocumentparser.h"
#include "cppsemanticinfo.h"
#include "cpptools_global.h"

#include <texteditor/basetexteditor.h>

#include <cplusplus/CppDocument.h>

#include <QTextEdit>

namespace CppTools {

class CPPTOOLS_EXPORT BaseEditorDocumentProcessor : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseEditorDocumentProcessor)
    BaseEditorDocumentProcessor();

public:
    BaseEditorDocumentProcessor(TextEditor::BaseTextDocument *document);
    virtual ~BaseEditorDocumentProcessor();

    TextEditor::BaseTextDocument *baseTextDocument() const;

    // Function interface to implement
    virtual void run() = 0;
    virtual void semanticRehighlight(bool force) = 0;
    virtual CppTools::SemanticInfo recalculateSemanticInfo() = 0;
    virtual BaseEditorDocumentParser *parser() = 0;
    virtual bool isParserRunning() const = 0;

public:
    static BaseEditorDocumentProcessor *get(const QString &filePath);

signals:
    // Signal interface to implement
    void codeWarningsUpdated(unsigned revision,
                             const QList<QTextEdit::ExtraSelection> selections);

    void ifdefedOutBlocksUpdated(unsigned revision,
                                 const QList<TextEditor::BlockRange> ifdefedOutBlocks);

    void cppDocumentUpdated(const CPlusPlus::Document::Ptr document);    // TODO: Remove me
    void semanticInfoUpdated(const CppTools::SemanticInfo semanticInfo); // TODO: Remove me

protected:
    static QList<QTextEdit::ExtraSelection> toTextEditorSelections(
            const QList<CPlusPlus::Document::DiagnosticMessage> &diagnostics,
            QTextDocument *textDocument);

    static void runParser(QFutureInterface<void> &future,
                          CppTools::BaseEditorDocumentParser *parser,
                          CppTools::WorkingCopy workingCopy);

    // Convenience
    QString filePath() const { return m_baseTextDocument->filePath(); }
    unsigned revision() const { return static_cast<unsigned>(textDocument()->revision()); }
    QTextDocument *textDocument() const { return m_baseTextDocument->document(); }

private:
    TextEditor::BaseTextDocument *m_baseTextDocument;
};

} // namespace CppTools

#endif // BASEEDITORDOCUMENTPROCESSOR_H

