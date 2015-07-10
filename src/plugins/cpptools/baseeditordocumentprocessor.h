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

#ifndef BASEEDITORDOCUMENTPROCESSOR_H
#define BASEEDITORDOCUMENTPROCESSOR_H

#include "baseeditordocumentparser.h"
#include "cppsemanticinfo.h"
#include "cpptools_global.h"

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

#include <cplusplus/CppDocument.h>

#include <QTextEdit>

namespace TextEditor { class TextDocument; }

namespace CppTools {

class CPPTOOLS_EXPORT BaseEditorDocumentProcessor : public QObject
{
    Q_OBJECT

public:
    BaseEditorDocumentProcessor(TextEditor::TextDocument *document);
    virtual ~BaseEditorDocumentProcessor();

    TextEditor::TextDocument *baseTextDocument() const;

    // Function interface to implement
    virtual void run() = 0;
    virtual void semanticRehighlight() = 0;
    virtual void recalculateSemanticInfoDetached(bool force) = 0;
    virtual CppTools::SemanticInfo recalculateSemanticInfo() = 0;
    virtual CPlusPlus::Snapshot snapshot() = 0;
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
                          BaseEditorDocumentParser::InMemoryInfo info);

    // Convenience
    QString filePath() const { return m_baseTextDocument->filePath().toString(); }
    unsigned revision() const { return static_cast<unsigned>(textDocument()->revision()); }
    QTextDocument *textDocument() const { return m_baseTextDocument->document(); }

private:
    TextEditor::TextDocument *m_baseTextDocument;
};

} // namespace CppTools

#endif // BASEEDITORDOCUMENTPROCESSOR_H

