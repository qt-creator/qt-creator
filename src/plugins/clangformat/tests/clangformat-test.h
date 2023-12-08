// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    void testIndentInitializeVector();
    void testIndentFunctionArgumentOnNewLine();
    void testIndentCommentOnNewLine();
    void testUtf8SymbolLine();

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
