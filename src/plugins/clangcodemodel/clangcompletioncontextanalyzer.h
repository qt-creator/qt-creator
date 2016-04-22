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

#include <cplusplus/Token.h>

#include <QString>

namespace TextEditor { class AssistInterface; }

namespace ClangCodeModel {
namespace Internal {

class ClangCompletionAssistInterface;

class ClangCompletionContextAnalyzer
{
public:
    ClangCompletionContextAnalyzer(const ClangCompletionAssistInterface *assistInterface,
                                   CPlusPlus::LanguageFeatures languageFeatures);
    void analyze();

    enum CompletionAction {
        PassThroughToLibClang,
        PassThroughToLibClangAfterLeftParen,
        CompleteDoxygenKeyword,
        CompleteIncludePath,
        CompletePreprocessorDirective,
        CompleteSignal,
        CompleteSlot
    };
    CompletionAction completionAction() const { return m_completionAction; }
    unsigned completionOperator() const { return m_completionOperator; }
    int positionForProposal() const { return m_positionForProposal; }
    int positionForClang() const { return m_positionForClang; }
    int positionEndOfExpression() const { return m_positionEndOfExpression; }

    QString functionName() const { return m_functionName; }

private:
    ClangCompletionContextAnalyzer();

    struct FunctionInfo {
        bool isValid() const { return functionNamePosition != -1 && !functionName.isEmpty(); }

        int functionNamePosition = -1;
        QString functionName;
    };
    FunctionInfo analyzeFunctionCall(int endOfExpression) const;

    void setActionAndClangPosition(CompletionAction action, int position);
    void setAction(CompletionAction action);

    bool handleNonFunctionCall(int position);
    void handleCommaInFunctionCall();
    void handleFunctionCall(int endOfOperator);

private:
    const ClangCompletionAssistInterface *m_interface; // Not owned
    const CPlusPlus::LanguageFeatures m_languageFeatures; // TODO: Get from assistInterface?!

    // Results
    CompletionAction m_completionAction = PassThroughToLibClang;
    CPlusPlus::Kind m_completionOperator = CPlusPlus::T_EOF_SYMBOL;
    int m_positionForProposal = -1;
    int m_positionForClang = -1;
    int m_positionEndOfExpression = -1;
    QString m_functionName;
};

} // namespace Internal
} // namespace ClangCodeModel
