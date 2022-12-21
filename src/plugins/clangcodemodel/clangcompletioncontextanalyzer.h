// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/Token.h>

#include <QString>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace TextEditor { class AssistInterface; }

namespace ClangCodeModel {
namespace Internal {

class ClangCompletionContextAnalyzer
{
public:
    ClangCompletionContextAnalyzer(QTextDocument *document, int position, bool isFunctionHint,
                                   CPlusPlus::LanguageFeatures languageFeatures);
    void analyze();

    enum CompletionAction {
        PassThroughToLibClang,
        PassThroughToLibClangAfterLeftParen,
        CompleteDoxygenKeyword,
        CompleteIncludePath,
        CompletePreprocessorDirective,
        CompleteSignal,
        CompleteSlot,
        CompleteNone
    };
    CompletionAction completionAction() const { return m_completionAction; }
    unsigned completionOperator() const { return m_completionOperator; }
    int positionForProposal() const { return m_positionForProposal; }
    int positionForClang() const { return m_positionForClang; }
    int functionNameStart() const { return m_functionNameStart; }
    int positionEndOfExpression() const { return m_positionEndOfExpression; }
    bool addSnippets() const { return m_addSnippets; }

private:
    int startOfFunctionCall(int endOfExpression) const;

    void setActionAndClangPosition(CompletionAction action,
                                   int position,
                                   int functionNameStart = -1);
    void setAction(CompletionAction action);

    bool handleNonFunctionCall(int position);
    void handleCommaInFunctionCall();
    void handleFunctionCall(int endOfOperator);

private:
    QTextDocument * const m_document;
    const int m_position;
    const bool m_isFunctionHint;
    const CPlusPlus::LanguageFeatures m_languageFeatures;

    // Results
    CompletionAction m_completionAction = PassThroughToLibClang;
    CPlusPlus::Kind m_completionOperator = CPlusPlus::T_EOF_SYMBOL;
    int m_positionForProposal = -1;
    int m_positionForClang = -1;
    int m_functionNameStart = -1;
    int m_positionEndOfExpression = -1;
    bool m_addSnippets = false;
};

} // namespace Internal
} // namespace ClangCodeModel
