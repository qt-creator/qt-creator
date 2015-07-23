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

#include <clangcodemodel/clangcompletioncontextanalyzer.h>
#include <clangcodemodel/clangcompletionassistinterface.h>

#include <utils/qtcassert.h>

#include <QTextDocument>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

namespace ClangCodeModel {
namespace Internal {

void PrintTo(const ClangCompletionContextAnalyzer::CompletionAction &completionAction,
             ::std::ostream* os)
{
    using CCA = ClangCompletionContextAnalyzer;

    switch (completionAction) {
        case CCA::PassThroughToLibClang: *os << "PassThroughToLibClang"; break;
        case CCA::PassThroughToLibClangAfterLeftParen: *os << "PassThroughToLibClangAfterLeftParen"; break;
        case CCA::CompleteDoxygenKeyword: *os << "CompleteDoxygenKeyword"; break;
        case CCA::CompleteIncludePath: *os << "CompleteIncludePath"; break;
        case CCA::CompletePreprocessorDirective: *os << "CompletePreprocessorDirective"; break;
        case CCA::CompleteSignal: *os << "CompleteSignal"; break;
        case CCA::CompleteSlot: *os << "CompleteSlot"; break;
    }
}

} // Internal
} // ClangCodeModel

namespace {

using ::testing::PrintToString;
using ClangCodeModel::Internal::ClangCompletionAssistInterface;
using CCA = ClangCodeModel::Internal::ClangCompletionContextAnalyzer;

class TestDocument
{
public:
    TestDocument(const QByteArray &theSource)
        : source(theSource),
          position(theSource.lastIndexOf('@')) // Use 'lastIndexOf' due to doxygen: "//! @keyword"
    {
        source.remove(position, 1);
    }

    QByteArray source;
    int position;
};

bool isPassThrough(CCA::CompletionAction completionAction)
{
    return completionAction != CCA::PassThroughToLibClang
        && completionAction != CCA::PassThroughToLibClangAfterLeftParen;
}

MATCHER(IsPassThroughToClang, std::string(negation ? "isn't" : "is") + " passed through to Clang")
{
    const auto completionAction = arg.completionAction();
    if (isPassThrough(completionAction)) {
        *result_listener << "completion action is " << PrintToString(completionAction);
        return false;
    }

    return true;
}

// Offsets are relative to positionInText
MATCHER_P4(HasResult,
           completionAction,
           positionForClangOffset,
           positionForProposalOffset,
           positionInText,
           std::string(negation ? "hasn't" : "has")
           + " result of completion action " + PrintToString(completionAction)
           + " and offset for clang " + PrintToString(positionForClangOffset)
           + " and offset for proprosal " + PrintToString(positionForProposalOffset))
{
    const int actualPositionForClangOffset = arg.positionForClang() - positionInText;
    const int actualPositionForProposalOffset = arg.positionForProposal() - positionInText;

    if (arg.completionAction() != completionAction
            || actualPositionForClangOffset != positionForClangOffset
            || actualPositionForProposalOffset != positionForProposalOffset) {
        *result_listener << "completion action is " << PrintToString(arg.completionAction())
                         << " and offset for clang is " <<  PrintToString(actualPositionForClangOffset)
                         << " and offset for proprosal is " <<  PrintToString(actualPositionForProposalOffset);
        return false;
    }

    return true;
}

// Offsets are relative to positionInText
MATCHER_P4(HasResultWithoutClangDifference,
           completionAction,
           positionForClangOffset,
           positionForProposalOffset,
           positionInText,
           std::string(negation ? "hasn't" : "has")
           + " result of completion action " + PrintToString(completionAction)
           + " and offset for clang " + PrintToString(positionForClangOffset)
           + " and offset for proprosal " + PrintToString(positionForProposalOffset))
{
    const int actualPositionForProposalOffset = arg.positionForProposal() - positionInText;

    if (arg.completionAction() != completionAction
            || arg.positionForClang() != positionForClangOffset
            || actualPositionForProposalOffset != positionForProposalOffset) {
        *result_listener << "completion action is " << PrintToString(arg.completionAction())
                         << " and offset for clang is " <<  PrintToString(arg.positionForClang())
                         << " and offset for proprosal is " <<  PrintToString(actualPositionForProposalOffset);
        return false;
    }

    return true;
}

class ClangCompletionContextAnalyzer : public ::testing::Test
{
protected:
    CCA runAnalyzer(const char *text);

protected:
    int positionInText = 0;
};

CCA ClangCompletionContextAnalyzer::runAnalyzer(const char *text)
{
    const TestDocument testDocument(text);
    ClangCompletionAssistInterface assistInterface(testDocument.source, testDocument.position);
    CCA analyzer(&assistInterface, CPlusPlus::LanguageFeatures::defaultFeatures());

    positionInText = testDocument.position;

    analyzer.analyze();

    return analyzer;
}

TEST_F(ClangCompletionContextAnalyzer, WordsBeforeCursor)
{
    auto analyzer = runAnalyzer("foo bar@");

    ASSERT_THAT(analyzer, HasResult(CCA::PassThroughToLibClang, -3, -3, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, AfterSpace)
{
    auto analyzer = runAnalyzer("foo @");

    ASSERT_THAT(analyzer, HasResult(CCA::PassThroughToLibClang, 0, 0, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, AtEndOfDotMember)
{
    auto analyzer = runAnalyzer("o.mem@");

    ASSERT_THAT(analyzer, HasResult(CCA::PassThroughToLibClang, -3, -3, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, AtEndOfDotMemberWithSpaceInside)
{
    auto analyzer = runAnalyzer("o. mem@");

    ASSERT_THAT(analyzer, HasResult(CCA::PassThroughToLibClang, -3, -3, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, AtBeginOfDotMember)
{
    auto analyzer = runAnalyzer("o.@mem");

    ASSERT_THAT(analyzer, HasResult(CCA::PassThroughToLibClang, 0, 0, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, AtBeginOfDotMemberWithSpaceInside)
{
    auto analyzer = runAnalyzer("o. @mem");

    ASSERT_THAT(analyzer, HasResult(CCA::PassThroughToLibClang, 0, 0, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, AtEndOfArrow)
{
    auto analyzer = runAnalyzer("o->mem@");

    ASSERT_THAT(analyzer, HasResult(CCA::PassThroughToLibClang, -3, -3, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, AtEndOfArrowWithSpaceInside)
{
    auto analyzer = runAnalyzer("o-> mem@");

    ASSERT_THAT(analyzer, HasResult(CCA::PassThroughToLibClang, -3, -3, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, AtBeginOfArrow)
{
    auto analyzer = runAnalyzer("o->@mem");

    ASSERT_THAT(analyzer, HasResult(CCA::PassThroughToLibClang, 0, 0, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, AtBeginOfArrowWithSpaceInside)
{
    auto analyzer = runAnalyzer("o-> @mem");

    ASSERT_THAT(analyzer, HasResult(CCA::PassThroughToLibClang, 0, 0, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, ParameteOneAtCall)
{
    auto analyzer = runAnalyzer("f(@");

    ASSERT_THAT(analyzer, HasResult(CCA::PassThroughToLibClangAfterLeftParen, -2, 0, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, ParameteTwoAtCall)
{
    auto analyzer = runAnalyzer("f(1,@");

    ASSERT_THAT(analyzer, HasResult(CCA::PassThroughToLibClangAfterLeftParen, -4, -2, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, ParameteTwoWithSpaceAtCall)
{
    auto analyzer = runAnalyzer("f(1, @");

    ASSERT_THAT(analyzer, HasResult(CCA::PassThroughToLibClangAfterLeftParen, -5, -3, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, ParameteOneAtSignal)
{
    auto analyzer = runAnalyzer("SIGNAL(@");

    ASSERT_THAT(analyzer, HasResult(CCA::CompleteSignal, 0, 0, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, ParameteOneWithLettersAtSignal)
{
    auto analyzer = runAnalyzer("SIGNAL(foo@");

    ASSERT_THAT(analyzer, HasResult(CCA::CompleteSignal, -3, -3, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, ParameteOneAtSlot)
{
    auto analyzer = runAnalyzer("SLOT(@");

    ASSERT_THAT(analyzer, HasResult(CCA::CompleteSlot, -0, 0, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, ParameteOneWithLettersAtSlot)
{
    auto analyzer = runAnalyzer("SLOT(foo@");

    ASSERT_THAT(analyzer, HasResult(CCA::CompleteSlot, -3, -3, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, DoxygenWithBackslash)
{
    auto analyzer = runAnalyzer("//! \\@");

    ASSERT_THAT(analyzer, HasResultWithoutClangDifference(CCA::CompleteDoxygenKeyword, -1, 0, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, DoxygenWithAt)
{
    auto analyzer = runAnalyzer("//! @@");

    ASSERT_THAT(analyzer, HasResultWithoutClangDifference(CCA::CompleteDoxygenKeyword, -1, 0, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, DoxygenWithParameter)
{
    auto analyzer = runAnalyzer("//! \\par@");

    ASSERT_THAT(analyzer, HasResultWithoutClangDifference(CCA::CompleteDoxygenKeyword, -1, -3, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, Preprocessor)
{
    auto analyzer = runAnalyzer("#@");

    ASSERT_THAT(analyzer, HasResultWithoutClangDifference(CCA::CompletePreprocessorDirective, -1, 0, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, PreprocessorIf)
{
    auto analyzer = runAnalyzer("#if@");

    ASSERT_THAT(analyzer, HasResultWithoutClangDifference(CCA::CompletePreprocessorDirective, -1, -2, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, LocalInclude)
{
    auto analyzer = runAnalyzer("#include \"foo@\"");

    ASSERT_THAT(analyzer, HasResultWithoutClangDifference(CCA::CompleteIncludePath, -1, -3, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, GlobalInclude)
{
    auto analyzer = runAnalyzer("#include <foo@>");

    ASSERT_THAT(analyzer, HasResultWithoutClangDifference(CCA::CompleteIncludePath, -1, -3, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, GlocalIncludeWithDirectory)
{
    auto analyzer = runAnalyzer("#include <foo/@>");

    ASSERT_THAT(analyzer, HasResultWithoutClangDifference(CCA::CompleteIncludePath, -1, 0, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, AfterQuote)
{
    auto analyzer = runAnalyzer("\"@");

    ASSERT_THAT(analyzer, IsPassThroughToClang());
}

TEST_F(ClangCompletionContextAnalyzer, AfterSpaceQuote)
{
    auto analyzer = runAnalyzer(" \"@");

    ASSERT_THAT(analyzer, IsPassThroughToClang());
}

TEST_F(ClangCompletionContextAnalyzer, AfterQuotedText)
{
    auto analyzer = runAnalyzer("\"text\"@");

    ASSERT_THAT(analyzer, IsPassThroughToClang());
}

TEST_F(ClangCompletionContextAnalyzer, InQuotedText)
{
    auto analyzer = runAnalyzer("\"hello cruel@ world\"");

    ASSERT_THAT(analyzer, IsPassThroughToClang());
}

TEST_F(ClangCompletionContextAnalyzer, SingleQuote)
{
    auto analyzer = runAnalyzer("'@'");

    ASSERT_THAT(analyzer, IsPassThroughToClang());
}

TEST_F(ClangCompletionContextAnalyzer, AfterLetterInSingleQuoted)
{
    auto analyzer = runAnalyzer("'a@'");

    ASSERT_THAT(analyzer, IsPassThroughToClang());
}

TEST_F(ClangCompletionContextAnalyzer, CommaOperator)
{
    auto analyzer = runAnalyzer("a = b,@\"");

    ASSERT_THAT(analyzer, IsPassThroughToClang());
}

TEST_F(ClangCompletionContextAnalyzer, DoxygenMarkerInNonDoxygenComment)
{
    auto analyzer = runAnalyzer("@@");

    ASSERT_THAT(analyzer, IsPassThroughToClang());
}

TEST_F(ClangCompletionContextAnalyzer, DoxygenMarkerInNonDoxygenComment2)
{
    auto analyzer = runAnalyzer("\\@");

    ASSERT_THAT(analyzer, IsPassThroughToClang());
}

TEST_F(ClangCompletionContextAnalyzer, AtEndOfOneLineComment)
{
    auto analyzer = runAnalyzer("// comment@");

    ASSERT_THAT(analyzer, IsPassThroughToClang());
}

TEST_F(ClangCompletionContextAnalyzer, AfterOneLineCommentLine)
{
    auto analyzer = runAnalyzer("// comment\n"
                                "@");

    ASSERT_THAT(analyzer, HasResult(CCA::PassThroughToLibClang, 0, 0, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, AfterEmptyOneLineComment)
{
    auto analyzer = runAnalyzer("//\n"
                                "@");

    ASSERT_THAT(analyzer, HasResult(CCA::PassThroughToLibClang, 0, 0, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, AfterOneLineDoxygenComment1)
{
    auto analyzer = runAnalyzer("/// comment\n"
                                "@");

    ASSERT_THAT(analyzer, HasResult(CCA::PassThroughToLibClang, 0, 0, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, AfterOneLineDoxygenComment2)
{
    auto analyzer = runAnalyzer("//! comment \n"
                                "@");

    ASSERT_THAT(analyzer, HasResult(CCA::PassThroughToLibClang, 0, 0, positionInText));
}

TEST_F(ClangCompletionContextAnalyzer, BeginEndComment)
{
    auto analyzer = runAnalyzer("/* text@ */");

    ASSERT_THAT(analyzer, IsPassThroughToClang());
}

TEST_F(ClangCompletionContextAnalyzer, Slash)
{
    auto analyzer = runAnalyzer("5 /@");

    ASSERT_THAT(analyzer, IsPassThroughToClang());
}

TEST_F(ClangCompletionContextAnalyzer, LeftParen)
{
    auto analyzer = runAnalyzer("(@");

    ASSERT_THAT(analyzer, IsPassThroughToClang());
}

TEST_F(ClangCompletionContextAnalyzer, TwoLeftParen)
{
    auto analyzer = runAnalyzer("((@");

    ASSERT_THAT(analyzer, IsPassThroughToClang());
}

TEST_F(ClangCompletionContextAnalyzer, AsteriskLeftParen)
{
    auto analyzer = runAnalyzer("*(@");

    ASSERT_THAT(analyzer, IsPassThroughToClang());
}

}
