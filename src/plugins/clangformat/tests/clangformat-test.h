/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <QObject>
#include <vector>

QT_BEGIN_NAMESPACE
class QString;
class QTextCursor;
class QTextDocument;
QT_END_NAMESPACE

namespace ClangFormat {
class ClangFormatBaseIndenter;

namespace Internal {

class ClangFormatTest : public QObject
{
    Q_OBJECT
public:
    ClangFormatTest();
    ~ClangFormatTest();

private slots:
    void testIndentBasicFile();
    void testIndentEmptyLine();
    void testIndentLambda();
    void testIndentNestedIfElse();
    void testIndentInitializerListInArguments();
    void testIndentLambdaWithReturnType();
    void testIndentFunctionArgumentLambdaWithNextLineScope();
    void testIndentScopeAsFunctionArgument();
    void testIndentInsideStructuredBinding();
    void testIndentMacrosWithoutSemicolon();
    void testIndentAfterSquareBracesInsideBraceInitialization();
    void testIndentStringLiteralContinuation();
    void testIndentTemplateparameters();
    void testNoExtraIndentAfterStatementInsideSquareBraces();
    void testNoExtraIndentAfterBraceInitialization();
    void testIndentMultipleEmptyLines();
    void testIndentEmptyLineAndKeepPreviousEmptyLines();
    void testIndentOnElectricCharacterButNotRemoveEmptyLinesBefore();
    void testIndentAfterExtraSpaceInpreviousLine();
    void testIndentEmptyLineInsideParentheses();
    void testIndentInsideIf();
    void testIndentInsideIf2();
    void testIndentInsideIf3();
    void testEmptyLineInInitializerList();
    void testIndentClosingBraceAfterComma();
    void testDoNotIndentClosingBraceAfterSemicolon();
    void testIndentAfterIf();
    void testIndentAfterElse();
    void testSameIndentAfterSecondNewLineAfterIf();
    void testIndentAfterNewLineInsideIfWithFunctionCall();
    void testSameIndentAfterSecondNewLineInsideIfWithFunctionCall();
    void testSameIndentAfterSecondNonEmptyNewLineInsideIfWithFunctionCall();
    void testSameIndentsOnNewLinesAfterComments();
    void testIndentAfterEmptyLineAfterAngledIncludeDirective();
    void testIndentAfterEmptyLineAfterQuotedIncludeDirective();
    void testIndentAfterLineComment();
    void testIndentAfterBlockComment();
    void testIndentAfterIfdef();
    void testIndentAfterEmptyLineInTheFileBeginning();
    void testIndentFunctionBodyButNotFormatBeforeIt();
    void testIndentAfterFunctionBodyAndNotFormatBefore();
    void testReformatToEmptyFunction();
    void testReformatToNonEmptyFunction();
    void testIndentClosingScopeAndFormatBeforeIt();
    void testDoNotFormatAfterTheFirstColon();
    void testOnlyIndentIncompleteStatementOnElectricalCharacter();
    void testIndentAndFormatCompleteStatementOnSemicolon();
    void testIndentAndFormatCompleteStatementOnClosingScope();
    void testOnlyIndentClosingParenthesis();
    void testEquallyIndentInsideParenthesis();
    void testFormatBasicFile();
    void testFormatEmptyLine();
    void testFormatLambda();
    void testFormatInitializerListInArguments();
    void testFormatFunctionArgumentLambdaWithScope();
    void testFormatScopeAsFunctionArgument();
    void testFormatStructuredBinding();
    void testFormatStringLiteralContinuation();
    void testFormatTemplateparameters();
    void testSortIncludes();
    void testChainedMemberFunctionCalls();
    void testCommentBlock();
    void testClassIndentStructure();

private:
    void insertLines(const std::vector<QString> &lines);
    std::vector<QString> documentLines() const;

    QTextDocument * const m_doc;
    QTextCursor * const m_cursor;
    ClangFormatBaseIndenter * const m_indenter;
    ClangFormatBaseIndenter * const m_extendedIndenter;
};

} // namespace Internal
} // namespace ClangFormat
