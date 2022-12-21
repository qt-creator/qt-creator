// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangcompletioncontextanalyzer.h"

#include "clangactivationsequencecontextprocessor.h"
#include "clangactivationsequenceprocessor.h"

#include <texteditor/codeassist/assistinterface.h>

#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/SimpleLexer.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

using namespace CPlusPlus;

namespace {

bool isTokenForIncludePathCompletion(unsigned tokenKind)
{
    return tokenKind == T_STRING_LITERAL
        || tokenKind == T_ANGLE_STRING_LITERAL
        || tokenKind == T_SLASH;
}

bool isTokenForPassThrough(unsigned tokenKind)
{
    return tokenKind == T_EOF_SYMBOL
        || tokenKind == T_DOT
        || tokenKind == T_COLON_COLON
        || tokenKind == T_ARROW
        || tokenKind == T_DOT_STAR;
}

} // anonymous namespace

namespace ClangCodeModel {
namespace Internal {

ClangCompletionContextAnalyzer::ClangCompletionContextAnalyzer(
        QTextDocument *document, int position, bool isFunctionHint,
        CPlusPlus::LanguageFeatures languageFeatures)
    : m_document(document), m_position(position), m_isFunctionHint(isFunctionHint),
      m_languageFeatures(languageFeatures)
{
}

void ClangCompletionContextAnalyzer::analyze()
{
    QTC_ASSERT(m_document, return);
    setActionAndClangPosition(PassThroughToLibClang, -1);

    ActivationSequenceContextProcessor activationSequenceContextProcessor(
                m_document, m_position, m_languageFeatures);
    m_completionOperator = activationSequenceContextProcessor.completionKind();
    int afterOperatorPosition = activationSequenceContextProcessor.startOfNamePosition();
    m_positionEndOfExpression = activationSequenceContextProcessor.operatorStartPosition();
    m_positionForProposal = activationSequenceContextProcessor.startOfNamePosition();

    const bool actionIsSet = !m_isFunctionHint && handleNonFunctionCall(afterOperatorPosition);
    if (!actionIsSet) {
        handleCommaInFunctionCall();
        handleFunctionCall(afterOperatorPosition);
    }
}

int ClangCompletionContextAnalyzer::startOfFunctionCall(int endOfOperator) const
{
    int index = ActivationSequenceContextProcessor::skipPrecedingWhitespace(m_document,
                                                                            endOfOperator);
    QTextCursor textCursor(m_document);
    textCursor.setPosition(index);

    ExpressionUnderCursor euc(m_languageFeatures);
    index = euc.startOfFunctionCall(textCursor);
    index = ActivationSequenceContextProcessor::skipPrecedingWhitespace(m_document, index);
    const int functionNameStart = ActivationSequenceContextProcessor::findStartOfName(
        m_document, index, ActivationSequenceContextProcessor::NameCategory::Function);
    if (functionNameStart == -1)
        return -1;

    QTextCursor functionNameSelector(m_document);
    functionNameSelector.setPosition(functionNameStart);
    functionNameSelector.setPosition(index, QTextCursor::KeepAnchor);
    const QString functionName = functionNameSelector.selectedText().trimmed();
    if (functionName.isEmpty() && m_completionOperator == T_LBRACE)
        return endOfOperator;

    return functionName.isEmpty() ? -1 : functionNameStart;
}

void ClangCompletionContextAnalyzer::setActionAndClangPosition(CompletionAction action,
                                                               int position,
                                                               int functionNameStart)
{
    QTC_CHECK(position >= -1);
    m_completionAction = action;
    m_positionForClang = position;
    m_functionNameStart = functionNameStart;
}

void
ClangCompletionContextAnalyzer::setAction(ClangCompletionContextAnalyzer::CompletionAction action)
{
    setActionAndClangPosition(action, -1);
}

void ClangCompletionContextAnalyzer::handleCommaInFunctionCall()
{
    if (m_completionOperator == T_COMMA) {
        ExpressionUnderCursor expressionUnderCursor(m_languageFeatures);
        QTextCursor textCursor(m_document);
        textCursor.setPosition(m_positionEndOfExpression);
        const int start = expressionUnderCursor.startOfFunctionCall(textCursor);
        m_positionEndOfExpression = start;
        m_positionForProposal = start + 1; // After '(' of function call
        if (m_document->characterAt(start) == '(')
            m_completionOperator = T_LPAREN;
        else
            m_completionOperator = T_LBRACE;
    }
}

void ClangCompletionContextAnalyzer::handleFunctionCall(int afterOperatorPosition)
{
    if (m_isFunctionHint) {
        const int functionNameStart = startOfFunctionCall(afterOperatorPosition);
        if (functionNameStart >= 0) {
            m_addSnippets = functionNameStart == afterOperatorPosition;
            setActionAndClangPosition(PassThroughToLibClangAfterLeftParen,
                                      m_positionForProposal,
                                      functionNameStart);
        } else {
            m_completionAction = CompleteNone;
        }
        return;
    }

    if (m_completionOperator == T_LPAREN || m_completionOperator == T_LBRACE) {
        ExpressionUnderCursor expressionUnderCursor(m_languageFeatures);
        QTextCursor textCursor(m_document);
        textCursor.setPosition(m_positionEndOfExpression);
        const QString expression = expressionUnderCursor(textCursor);
        const QString trimmedExpression = expression.trimmed();
        const QChar lastExprChar = trimmedExpression.isEmpty()
                ? QChar() : trimmedExpression.at(trimmedExpression.length() - 1);
        const bool mightBeConstructorCall = lastExprChar != ')';

        if (expression.endsWith(QLatin1String("SIGNAL"))) {
            setActionAndClangPosition(CompleteSignal, afterOperatorPosition);
        } else if (expression.endsWith(QLatin1String("SLOT"))) {
            setActionAndClangPosition(CompleteSlot, afterOperatorPosition);
        } else if (m_position != afterOperatorPosition
                   || (m_completionOperator == T_LBRACE && !mightBeConstructorCall)) {
            // No function completion if cursor is not after '(' or ','
            m_addSnippets = true;
            m_positionForProposal = afterOperatorPosition;
            setActionAndClangPosition(PassThroughToLibClang, afterOperatorPosition);
        } else {
            const int functionNameStart = startOfFunctionCall(afterOperatorPosition);
            if (functionNameStart >= 0) {
                m_addSnippets = functionNameStart == afterOperatorPosition;
                setActionAndClangPosition(PassThroughToLibClangAfterLeftParen,
                                          afterOperatorPosition,
                                          functionNameStart);
            } else { // e.g. "(" without any function name in front
                m_addSnippets = true;
                m_positionForProposal = afterOperatorPosition;
                setActionAndClangPosition(PassThroughToLibClang, afterOperatorPosition);
            }
        }
    }
}

bool ClangCompletionContextAnalyzer::handleNonFunctionCall(int position)
{
    if (isTokenForPassThrough(m_completionOperator)) {
        if (m_completionOperator == T_EOF_SYMBOL)
            m_addSnippets = true;
        setActionAndClangPosition(PassThroughToLibClang, position);
        return true;
    } else if (m_completionOperator == T_DOXY_COMMENT) {
        setAction(CompleteDoxygenKeyword);
        return true;
    } else if (m_completionOperator == T_POUND) {
        // TODO: Check if libclang can complete preprocessor directives
        setAction(CompletePreprocessorDirective);
        return true;
    } else if (isTokenForIncludePathCompletion(m_completionOperator)) {
        setAction(CompleteIncludePath);
        return true;
    }

    return false;
}

} // namespace Internal
} // namespace ClangCodeModel
