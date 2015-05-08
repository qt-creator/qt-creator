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

#ifndef CLANGCOMPLETIONCONTEXTANALYZER_H
#define CLANGCOMPLETIONCONTEXTANALYZER_H

#include <cplusplus/Token.h>

#include <QString>

namespace TextEditor { class AssistInterface; }

namespace ClangCodeModel {
namespace Internal {

class ClangCompletionContextAnalyzer
{
public:
    ClangCompletionContextAnalyzer(const TextEditor::AssistInterface *assistInterface,
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

    struct FunctionInfo { int functionNamePosition; QString functionName; };
    FunctionInfo analyzeFunctionCall(int endOfExpression) const;

    int findStartOfName(int position = -1) const;
    int skipPrecedingWhitespace(int position) const;
    int startOfOperator(int position, unsigned *kind, bool wantFunctionCall) const;

    void setActionAndClangPosition(CompletionAction action, int position);

    const TextEditor::AssistInterface * const m_interface; // Not owned
    const CPlusPlus::LanguageFeatures m_languageFeatures; // TODO: Get from assistInterface?!

    // Results
    CompletionAction m_completionAction = PassThroughToLibClang;
    unsigned m_completionOperator = CPlusPlus::T_EOF_SYMBOL;
    int m_positionForProposal = -1;
    int m_positionForClang = -1;
    int m_positionEndOfExpression = -1;
    QString m_functionName;
};

} // namespace Internal
} // namespace ClangCodeModel

#endif // CLANGCOMPLETIONCONTEXTANALYZER_H
