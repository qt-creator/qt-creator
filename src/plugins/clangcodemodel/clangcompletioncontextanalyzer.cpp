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
        const ClangCompletionAssistInterface *assistInterface,
        CPlusPlus::LanguageFeatures languageFeatures)
    : m_interface(assistInterface)
    , m_languageFeatures(languageFeatures)
{
}

void ClangCompletionContextAnalyzer::analyze()
{
    QTC_ASSERT(m_interface, return);
    setActionAndClangPosition(PassThroughToLibClang, -1);

    ActivationSequenceContextProcessor activationSequenceContextProcessor(m_interface);
    m_completionOperator = activationSequenceContextProcessor.completionKind();
    int afterOperatorPosition = activationSequenceContextProcessor.startOfNamePosition();
    m_positionEndOfExpression = activationSequenceContextProcessor.operatorStartPosition();
    m_positionForProposal = activationSequenceContextProcessor.startOfNamePosition();

    const bool actionIsSet = handleNonFunctionCall(afterOperatorPosition);
    if (!actionIsSet) {
        handleCommaInFunctionCall();
        handleFunctionCall(afterOperatorPosition);
    }
}

int ClangCompletionContextAnalyzer::startOfFunctionCall(int endOfOperator) const
{
    int index = ActivationSequenceContextProcessor::skipPrecedingWhitespace(m_interface,
                                                                            endOfOperator);
    QTextCursor textCursor(m_interface->textDocument());
    textCursor.setPosition(index);

    ExpressionUnderCursor euc(m_languageFeatures);
    index = euc.startOfFunctionCall(textCursor);
    index = ActivationSequenceContextProcessor::skipPrecedingWhitespace(m_interface, index);
    const int functionNameStart = ActivationSequenceContextProcessor::findStartOfName(
        m_interface, index, ActivationSequenceContextProcessor::NameCategory::Function);
    if (functionNameStart == -1)
        return -1;

    QTextCursor functionNameSelector(m_interface->textDocument());
    functionNameSelector.setPosition(functionNameStart);
    functionNameSelector.setPosition(index, QTextCursor::KeepAnchor);
    const QString functionName = functionNameSelector.selectedText().trimmed();

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
        QTextCursor textCursor(m_interface->textDocument());
        textCursor.setPosition(m_positionEndOfExpression);
        const int start = expressionUnderCursor.startOfFunctionCall(textCursor);
        m_positionEndOfExpression = start;
        m_positionForProposal = start + 1; // After '(' of function call
        m_completionOperator = T_LPAREN;
    }
}

void ClangCompletionContextAnalyzer::handleFunctionCall(int afterOperatorPosition)
{
    if (m_completionOperator == T_LPAREN) {
        ExpressionUnderCursor expressionUnderCursor(m_languageFeatures);
        QTextCursor textCursor(m_interface->textDocument());
        textCursor.setPosition(m_positionEndOfExpression);
        const QString expression = expressionUnderCursor(textCursor);

        if (expression.endsWith(QLatin1String("SIGNAL"))) {
            setActionAndClangPosition(CompleteSignal, afterOperatorPosition);
        } else if (expression.endsWith(QLatin1String("SLOT"))) {
            setActionAndClangPosition(CompleteSlot, afterOperatorPosition);
        } else if (m_interface->position() != afterOperatorPosition) {
            // No function completion if cursor is not after '(' or ','
            m_positionForProposal = afterOperatorPosition;
            setActionAndClangPosition(PassThroughToLibClang, afterOperatorPosition);
        } else {
            const int functionNameStart = startOfFunctionCall(afterOperatorPosition);
            if (functionNameStart >= 0) {
                // Always pass the position right after '(' to libclang because
                // positions after the comma might be problematic if a preceding
                // argument is invalid code.
                setActionAndClangPosition(PassThroughToLibClangAfterLeftParen,
                                          m_positionForProposal,
                                          functionNameStart);
            } else { // e.g. "(" without any function name in front
                m_positionForProposal = afterOperatorPosition;
                setActionAndClangPosition(PassThroughToLibClang, afterOperatorPosition);
            }
        }
    }
}

bool ClangCompletionContextAnalyzer::handleNonFunctionCall(int position)
{
    if (isTokenForPassThrough(m_completionOperator)) {
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
