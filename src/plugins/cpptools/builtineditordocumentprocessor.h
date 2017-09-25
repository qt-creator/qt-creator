/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "baseeditordocumentprocessor.h"
#include "builtineditordocumentparser.h"
#include "cppsemanticinfoupdater.h"
#include "cpptools_global.h"
#include "semantichighlighter.h"

namespace CppTools {

class CPPTOOLS_EXPORT BuiltinEditorDocumentProcessor : public BaseEditorDocumentProcessor
{
    Q_OBJECT

public:
    BuiltinEditorDocumentProcessor(TextEditor::TextDocument *document,
                                   bool enableSemanticHighlighter = true);
    ~BuiltinEditorDocumentProcessor();

    // BaseEditorDocumentProcessor interface
    void runImpl(const BaseEditorDocumentParser::UpdateParams &updateParams) override;
    void recalculateSemanticInfoDetached(bool force) override;
    void semanticRehighlight() override;
    CppTools::SemanticInfo recalculateSemanticInfo() override;
    BaseEditorDocumentParser::Ptr parser() override;
    CPlusPlus::Snapshot snapshot() override;
    bool isParserRunning() const override;

    QFuture<CursorInfo> cursorInfo(const CursorInfoParams &params) override;
    QFuture<SymbolInfo> requestFollowSymbol(int, int) override
    { return QFuture<SymbolInfo>(); }

private:
    void onParserFinished(CPlusPlus::Document::Ptr document, CPlusPlus::Snapshot snapshot);
    void onSemanticInfoUpdated(const CppTools::SemanticInfo semanticInfo);
    void onCodeWarningsUpdated(CPlusPlus::Document::Ptr document,
                               const QList<CPlusPlus::Document::DiagnosticMessage> &codeWarnings);

    SemanticInfo::Source createSemanticInfoSource(bool force) const;

private:
    BuiltinEditorDocumentParser::Ptr m_parser;
    QFuture<void> m_parserFuture;

    CPlusPlus::Snapshot m_documentSnapshot;
    QList<QTextEdit::ExtraSelection> m_codeWarnings;
    bool m_codeWarningsUpdated;

    SemanticInfoUpdater m_semanticInfoUpdater;
    QScopedPointer<SemanticHighlighter> m_semanticHighlighter;
};

} // namespace CppTools
