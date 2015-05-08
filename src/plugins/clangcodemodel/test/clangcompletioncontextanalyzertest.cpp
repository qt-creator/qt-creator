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

#include "clangcompletioncontextanalyzertest.h"

#include <clangcodemodel/clangcompletioncontextanalyzer.h>
#include <texteditor/codeassist/assistinterface.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QTest>
#include <QTextDocument>

using namespace CPlusPlus;
using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;
using namespace ClangCodeModel::Internal::Tests;

Q_DECLARE_METATYPE(ClangCodeModel::Internal::ClangCompletionContextAnalyzer::CompletionAction)

namespace QTest {

template<> char *toString(const ClangCompletionContextAnalyzer::CompletionAction &action)
{
    using CCA = ClangCompletionContextAnalyzer;

    switch (action) {
    case CCA::PassThroughToLibClang:
        return qstrdup("PassThroughToLibClang");
    case CCA::PassThroughToLibClangAfterLeftParen:
        return qstrdup("PassThroughToLibClangAfterLeftParen");
    case CCA::CompleteDoxygenKeyword:
        return qstrdup("CompleteDoxygenKeyword");
    case CCA::CompleteIncludePath:
        return qstrdup("CompleteIncludePath");
    case CCA::CompletePreprocessorDirective:
        return qstrdup("CompletePreprocessorDirective");
    case CCA::CompleteSignal:
        return qstrdup("CompleteSignal");
    case CCA::CompleteSlot:
        return qstrdup("CompleteSlot");
    }
    return qstrdup("Unexpected Value");
}

} // namespace QTest

namespace {

typedef QByteArray _;

class DummyAssistInterface : public TextEditor::AssistInterface
{
public:
    DummyAssistInterface(const QByteArray &text, int position)
        : AssistInterface(new QTextDocument(QString::fromUtf8(text)),
                          position,
                          QLatin1String("<testdocument>"),
                          TextEditor::ActivationCharacter)
    {}
    ~DummyAssistInterface() { delete textDocument(); }
};

class TestDocument
{
public:
    TestDocument(const QByteArray &theSource)
        : source(theSource)
        , position(theSource.lastIndexOf('@')) // Use 'lastIndexOf' due to doxygen: "//! @keyword"
    {
        QTC_CHECK(position != -1);
        source.remove(position, 1);
    }

    QByteArray source;
    int position;
};

bool isAPassThroughToLibClangAction(ClangCompletionContextAnalyzer::CompletionAction action)
{
    return action == ClangCompletionContextAnalyzer::PassThroughToLibClang
        || action == ClangCompletionContextAnalyzer::PassThroughToLibClangAfterLeftParen;
}

ClangCompletionContextAnalyzer runAnalyzer(const TestDocument &testDocument)
{
    DummyAssistInterface assistInterface(testDocument.source, testDocument.position);
    ClangCompletionContextAnalyzer analyzer(&assistInterface, LanguageFeatures::defaultFeatures());
    analyzer.analyze();
    return analyzer;
}

} // anonymous namespace

void ClangCompletionContextAnalyzerTest::testPassThroughToClangAndSignalSlotRecognition_data()
{
    QTest::addColumn<QByteArray>("givenSource");
    QTest::addColumn<ClangCompletionContextAnalyzer::CompletionAction>("expectedCompletionAction");
    QTest::addColumn<int>("expectedDiffBetweenCursorAndCalculatedClangPosition");
    QTest::addColumn<int>("expectedDiffBetweenCursorAndCalculatedProposalPosition");

    using CCA = ClangCompletionContextAnalyzer;

    QTest::newRow("members - dot 1") << _("o.mem@") << CCA::PassThroughToLibClang << -3 << -3;
    QTest::newRow("members - dot 2") << _("o. mem@") << CCA::PassThroughToLibClang << -4 << -3;
    QTest::newRow("members - dot 3") << _("o.@mem") << CCA::PassThroughToLibClang << 0 << 0;
    QTest::newRow("members - dot 4") << _("o. @ mem") << CCA::PassThroughToLibClang << -1 << 0;
    QTest::newRow("members - arrow 1") << _("o->mem@") << CCA::PassThroughToLibClang << -3 << -3;
    QTest::newRow("members - arrow 2") << _("o-> mem@") << CCA::PassThroughToLibClang << -4 << -3;
    QTest::newRow("members - arrow 3") << _("o->@mem") << CCA::PassThroughToLibClang << 0 << 0;
    QTest::newRow("members - arrow 4") << _("o-> @ mem") << CCA::PassThroughToLibClang << -1 << 0;

    QTest::newRow("call 1") << _("f(@") << CCA::PassThroughToLibClangAfterLeftParen << -2 << 0;
    QTest::newRow("call 2") << _("f(1,@") << CCA::PassThroughToLibClangAfterLeftParen << -4 << -2;
    QTest::newRow("call 3") << _("f(1, @") << CCA::PassThroughToLibClang << -1 << 0;

    QTest::newRow("qt4 signals 1") << _("SIGNAL(@") << CCA::CompleteSignal << 0 << 0;
    QTest::newRow("qt4 signals 2") << _("SIGNAL(foo@") << CCA::CompleteSignal << -3 << -3;
    QTest::newRow("qt4 slots 1") << _("SLOT(@") << CCA::CompleteSlot << 0 << 0;
    QTest::newRow("qt4 slots 2") << _("SLOT(foo@") << CCA::CompleteSlot << -3 << -3;
}

void ClangCompletionContextAnalyzerTest::testPassThroughToClangAndSignalSlotRecognition()
{
    QFETCH(QByteArray, givenSource);
    QFETCH(ClangCompletionContextAnalyzer::CompletionAction, expectedCompletionAction);
    QFETCH(int, expectedDiffBetweenCursorAndCalculatedClangPosition);
    QFETCH(int, expectedDiffBetweenCursorAndCalculatedProposalPosition);

    const TestDocument testDocument(givenSource);
    ClangCompletionContextAnalyzer analyzer = runAnalyzer(testDocument);

    QCOMPARE(analyzer.completionAction(), expectedCompletionAction);
    QCOMPARE(analyzer.positionForClang() - testDocument.position,
             expectedDiffBetweenCursorAndCalculatedClangPosition);
    QCOMPARE(analyzer.positionForProposal() - testDocument.position,
             expectedDiffBetweenCursorAndCalculatedProposalPosition);
}

void ClangCompletionContextAnalyzerTest::testSpecialCompletionRecognition_data()
{
    QTest::addColumn<QByteArray>("givenSource");
    QTest::addColumn<ClangCompletionContextAnalyzer::CompletionAction>("expectedCompletionAction");
    QTest::addColumn<int>("expectedDiffBetweenCursorAndCalculatedProposalPosition");

    using CCA = ClangCompletionContextAnalyzer;

    QTest::newRow("doxygen keywords 1") << _("//! \\@") << CCA::CompleteDoxygenKeyword << 0;
    QTest::newRow("doxygen keywords 3") << _("//! @@") << CCA::CompleteDoxygenKeyword << 0;
    QTest::newRow("doxygen keywords 2") << _("//! \\par@") << CCA::CompleteDoxygenKeyword << -3;

    QTest::newRow("pp directives 1") << _("#@") << CCA::CompletePreprocessorDirective << 0;
    QTest::newRow("pp directives 2") << _("#if@") << CCA::CompletePreprocessorDirective << -2;

    QTest::newRow("pp include path 1") << _("#include \"foo@\"") << CCA::CompleteIncludePath << -3;
    QTest::newRow("pp include path 2") << _("#include <foo@>") << CCA::CompleteIncludePath << -3;
    QTest::newRow("pp include path 3") << _("#include <foo/@>") << CCA::CompleteIncludePath << 0;
}

void ClangCompletionContextAnalyzerTest::testSpecialCompletionRecognition()
{
    QFETCH(QByteArray, givenSource);
    QFETCH(ClangCompletionContextAnalyzer::CompletionAction, expectedCompletionAction);
    QFETCH(int, expectedDiffBetweenCursorAndCalculatedProposalPosition);

    const TestDocument testDocument(givenSource);
    ClangCompletionContextAnalyzer analyzer = runAnalyzer(testDocument);

    QCOMPARE(analyzer.completionAction(), expectedCompletionAction);
    QCOMPARE(analyzer.positionForClang(), -1);
    QCOMPARE(analyzer.positionForProposal() - testDocument.position,
             expectedDiffBetweenCursorAndCalculatedProposalPosition);
}

void ClangCompletionContextAnalyzerTest::testAvoidSpecialCompletionRecognition_data()
{
    QTest::addColumn<QByteArray>("givenSource");

    QTest::newRow("no special completion for literals 1") << _("\"@");
    QTest::newRow("no special completion for literals 2") << _(" \"@");
    QTest::newRow("no special completion for literals 3") << _("\"text\"@");
    QTest::newRow("no special completion for literals 4") << _("\"hello cruel@ world\"");
    QTest::newRow("no special completion for literals 5") << _("'@'");
    QTest::newRow("no special completion for literals 6") << _("'a@'");
    QTest::newRow("no special completion for comma operator") << _("a = b,@\"");
    QTest::newRow("no special completion for doxygen marker not in doxygen comment 1") << _("@@");
    QTest::newRow("no special completion for doxygen marker not in doxygen comment 2") << _("\\@");
    QTest::newRow("no special completion in comments 1") << _("// text@");
    QTest::newRow("no special completion in comments 2") << _("/* text@ */");
    QTest::newRow("no special completion for slash") << _("5 /@");
    QTest::newRow("no special completion for '(' 1") << _("(@");
    QTest::newRow("no special completion for '(' 2") << _("((@");
    QTest::newRow("no special completion for '(' 3") << _("*(@");
}

void ClangCompletionContextAnalyzerTest::testAvoidSpecialCompletionRecognition()
{
    QFETCH(QByteArray, givenSource);

    const TestDocument testDocument(givenSource);
    ClangCompletionContextAnalyzer analyzer = runAnalyzer(testDocument);

    QVERIFY(isAPassThroughToLibClangAction(analyzer.completionAction()));
}
