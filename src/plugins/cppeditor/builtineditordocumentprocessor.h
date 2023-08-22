// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "baseeditordocumentprocessor.h"
#include "builtineditordocumentparser.h"
#include "cppeditor_global.h"
#include "cppsemanticinfoupdater.h"
#include "semantichighlighter.h"

#include <functional>

namespace CppEditor {

class CPPEDITOR_EXPORT BuiltinEditorDocumentProcessor : public BaseEditorDocumentProcessor
{
    Q_OBJECT

public:
    BuiltinEditorDocumentProcessor(TextEditor::TextDocument *document);
    ~BuiltinEditorDocumentProcessor() override;

    // BaseEditorDocumentProcessor interface
    void runImpl(const BaseEditorDocumentParser::UpdateParams &updateParams) override;
    void recalculateSemanticInfoDetached(bool force) override;
    void semanticRehighlight() override;
    SemanticInfo recalculateSemanticInfo() override;
    BaseEditorDocumentParser::Ptr parser() override;
    CPlusPlus::Snapshot snapshot() override;
    bool isParserRunning() const override;

    QFuture<CursorInfo> cursorInfo(const CursorInfoParams &params) override;

    using SemanticHighlightingChecker = std::function<bool()>;
    void setSemanticHighlightingChecker(const SemanticHighlightingChecker &checker);

private:
    void onParserFinished(CPlusPlus::Document::Ptr document, CPlusPlus::Snapshot snapshot);
    void onSemanticInfoUpdated(const SemanticInfo semanticInfo);
    void onCodeWarningsUpdated(CPlusPlus::Document::Ptr document,
                               const QList<CPlusPlus::Document::DiagnosticMessage> &codeWarnings);

    SemanticInfo::Source createSemanticInfoSource(bool force) const;

    virtual void forceUpdate(TextEditor::TextDocument *) {}

private:
    BuiltinEditorDocumentParser::Ptr m_parser;
    QFuture<void> m_parserFuture;

    CPlusPlus::Snapshot m_documentSnapshot;
    QList<QTextEdit::ExtraSelection> m_codeWarnings;
    bool m_codeWarningsUpdated;

    SemanticInfoUpdater m_semanticInfoUpdater;
    QScopedPointer<SemanticHighlighter> m_semanticHighlighter;
    SemanticHighlightingChecker m_semanticHighlightingChecker;
};

} // namespace CppEditor
