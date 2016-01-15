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

#include "clangfunctionhintmodel.h"

#include "clangcompletionchunkstotextconverter.h"

#include <cplusplus/SimpleLexer.h>

namespace ClangCodeModel {
namespace Internal {

using namespace CPlusPlus;

ClangFunctionHintModel::ClangFunctionHintModel(const ClangBackEnd::CodeCompletions &functionSymbols)
    : m_functionSymbols(functionSymbols)
    , m_currentArgument(-1)
{
}

void ClangFunctionHintModel::reset()
{
}

int ClangFunctionHintModel::size() const
{
    return m_functionSymbols.size();
}

QString ClangFunctionHintModel::text(int index) const
{
    const ClangBackEnd::CodeCompletionChunks chunks = m_functionSymbols.at(index).chunks();
    const QString signatureWithEmphasizedCurrentParameter
        = CompletionChunksToTextConverter::convertToFunctionSignatureWithHtml(chunks,
                                                                              m_currentArgument + 1);

    return signatureWithEmphasizedCurrentParameter;
}

int ClangFunctionHintModel::activeArgument(const QString &prefix) const
{
    int activeArgumentNumber = 0;

    int unbalancedParens = 0; // expressions
    int unbalancedBraces = 0; // initializer lists
    int unbalancedBrackets = 0; // lambda-capture
    int unbalancedLessGreater = 0; // template arguments

    SimpleLexer tokenize;
    const Tokens tokens = tokenize(prefix);
    for (const Token &token : tokens) {
        if (token.is(T_LPAREN)) {
            ++unbalancedParens;
        } else if (token.is(T_RPAREN)) {
            --unbalancedParens;
        } else if (token.is(T_LBRACE)) {
            ++unbalancedBraces;
        } else if (token.is(T_RBRACE)) {
            --unbalancedBraces;
        } else if (token.is(T_LBRACKET)) {
            ++unbalancedBrackets;
        } else if (token.is(T_RBRACKET)) {
            --unbalancedBrackets;
        } else if (token.is(T_LESS)) {
            ++unbalancedLessGreater;
        } else if (token.is(T_GREATER)) {
            --unbalancedLessGreater;
        } else if (!unbalancedParens
                   && !unbalancedBraces
                   && !unbalancedBrackets
                   && !unbalancedLessGreater
                   && token.is(T_COMMA)) {
            ++activeArgumentNumber;
        }
    }

    if (unbalancedParens < 0
            || unbalancedBraces < 0
            || unbalancedBrackets < 0
            || unbalancedLessGreater < 0) {
        return -1;
    }

    if (activeArgumentNumber != m_currentArgument)
        m_currentArgument = activeArgumentNumber;

    return activeArgumentNumber;
}

} // namespace Internal
} // namespace ClangCodeModel

