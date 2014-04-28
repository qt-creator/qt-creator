/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

/**
 * @file clangcompletion_test.cpp
 * @brief Performs test for C/C++ code completion
 *
 * All test cases given as strings with @ character that points to completion
 * location.
 */

#ifdef WITH_TESTS

// Disabled because there still no tool to detect system Objective-C headers
#define ENABLE_OBJC_TESTS 0

#include <QtTest>
#include <QDebug>
#undef interface // Canceling "#DEFINE interface struct" on Windows

#include "completiontesthelper.h"
#include "../clangcodemodelplugin.h"

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;

////////////////////////////////////////////////////////////////////////////////
// Test cases

/**
 * \defgroup Regression tests
 *
 * This group tests possible regressions in non-standard completion chunks
 * handling: for example, macro arguments and clang's code snippets.
 *
 * @{
 */

void ClangCodeModelPlugin::test_CXX_regressions()
{
    QFETCH(QString, file);
    QFETCH(QStringList, unexpected);
    QFETCH(QStringList, mustHave);

    CompletionTestHelper helper;
    helper << file;

    QStringList proposals = helper.codeCompleteTexts();

    foreach (const QString &p, unexpected)
        QTEST_ASSERT(false == proposals.contains(p));

    foreach (const QString &p, mustHave)
        QTEST_ASSERT(true == proposals.contains(p));
}

void ClangCodeModelPlugin::test_CXX_regressions_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QStringList>("unexpected");
    QTest::addColumn<QStringList>("mustHave");

    QString file;
    QStringList unexpected;
    QStringList mustHave;

    file = QLatin1String("cxx_regression_1.cpp");
    mustHave << QLatin1String("sqr");
    mustHave << QLatin1String("~Math");
    unexpected << QLatin1String("operator=");
    QTest::newRow("case 1: method call completion") << file << unexpected << mustHave;
    mustHave.clear();
    unexpected.clear();

    file = QLatin1String("cxx_regression_2.cpp");
    unexpected << QLatin1String("i_second");
    unexpected << QLatin1String("c_second");
    unexpected << QLatin1String("f_second");
    mustHave << QLatin1String("i_first");
    mustHave << QLatin1String("c_first");
    QTest::newRow("case 2: multiple anonymous structs") << file << unexpected << mustHave;
    mustHave.clear();
    unexpected.clear();

    file = QLatin1String("cxx_regression_3.cpp");
    mustHave << QLatin1String("i8");
    mustHave << QLatin1String("i64");
    mustHave << QLatin1String("~Priv");
    unexpected << QLatin1String("operator=");
    QTest::newRow("case 3: nested class resolution") << file << unexpected << mustHave;
    mustHave.clear();
    unexpected.clear();

    file = QLatin1String("cxx_regression_4.cpp");
    mustHave << QLatin1String("action");
    QTest::newRow("case 4: local (in function) class resolution") << file << unexpected << mustHave;
    mustHave.clear();
    unexpected.clear();

    file = QLatin1String("cxx_regression_5.cpp");
    mustHave << QLatin1String("doB");
    unexpected << QLatin1String("doA");
    QTest::newRow("case 5: nested template class resolution") << file << unexpected << mustHave;
    mustHave.clear();
    unexpected.clear();

    file = QLatin1String("cxx_regression_6.cpp");
    mustHave << QLatin1String("OwningPtr");
    QTest::newRow("case 6: using particular symbol from namespace") << file << unexpected << mustHave;
    mustHave.clear();
    unexpected.clear();

    file = QLatin1String("cxx_regression_7.cpp");
    mustHave << QLatin1String("dataMember");
    mustHave << QLatin1String("anotherMember");
    QTest::newRow("case 7: template class inherited from template parameter") << file << unexpected << mustHave;
    mustHave.clear();
    unexpected.clear();

    file = QLatin1String("cxx_regression_8.cpp");
    mustHave << QLatin1String("utils::");
    unexpected << QLatin1String("utils");
    QTest::newRow("case 8: namespace completion in function body") << file << unexpected << mustHave;
    mustHave.clear();
    unexpected.clear();

    file = QLatin1String("cxx_regression_9.cpp");
    mustHave << QLatin1String("EnumScoped::Value1");
    mustHave << QLatin1String("EnumScoped::Value2");
    mustHave << QLatin1String("EnumScoped::Value3");
    unexpected << QLatin1String("Value1");
    unexpected << QLatin1String("EnumScoped");
    QTest::newRow("case 9: c++11 enum class, value used in switch and 'case' completed")
            << file << unexpected << mustHave;
    mustHave.clear();
    unexpected.clear();
}

void ClangCodeModelPlugin::test_CXX_snippets()
{
    QFETCH(QString, file);
    QFETCH(QStringList, texts);
    QFETCH(QStringList, snippets);
    Q_ASSERT(texts.size() == snippets.size());

    CompletionTestHelper helper;
    helper << file;

    QList<CodeCompletionResult> proposals = helper.codeComplete();

    for (int i = 0, n = texts.size(); i < n; ++i) {
        const QString &text = texts[i];
        const QString &snippet = snippets[i];
        const QString snippetError =
                QLatin1String("Text and snippet mismatch: text '") + text
                + QLatin1String("', snippet '") + snippet
                + QLatin1String("', got snippet '%1'");

        bool hasText = false;
        foreach (const CodeCompletionResult &ccr, proposals) {
            if (ccr.text() != text)
                continue;
            hasText = true;
            QVERIFY2(snippet == ccr.snippet(), snippetError.arg(ccr.snippet()).toAscii());
        }
        const QString textError(QLatin1String("Text not found:") + text);
        QVERIFY2(hasText, textError.toAscii());
    }
}

void ClangCodeModelPlugin::test_CXX_snippets_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QStringList>("texts");
    QTest::addColumn<QStringList>("snippets");

    QString file;
    QStringList texts;
    QStringList snippets;

    file = QLatin1String("cxx_snippets_1.cpp");
    texts << QLatin1String("reinterpret_cast<type>(expression)");
    snippets << QLatin1String("reinterpret_cast<$type$>($expression$)");

    texts << QLatin1String("static_cast<type>(expression)");
    snippets << QLatin1String("static_cast<$type$>($expression$)");

    texts << QLatin1String("new type(expressions)");
    snippets << QLatin1String("new $type$($expressions$)");

    QTest::newRow("case: snippets for var declaration") << file << texts << snippets;
    texts.clear();
    snippets.clear();

    file = QLatin1String("cxx_snippets_2.cpp");
    texts << QLatin1String("private");
    snippets << QLatin1String("");

    texts << QLatin1String("protected");
    snippets << QLatin1String("");

    texts << QLatin1String("public");
    snippets << QLatin1String("");

    texts << QLatin1String("friend");
    snippets << QLatin1String("");

    texts << QLatin1String("virtual");
    snippets << QLatin1String("");

    texts << QLatin1String("typedef type name");
    snippets << QLatin1String("typedef $type$ $name$");

    QTest::newRow("case: snippets inside class declaration") << file << texts << snippets;
    texts.clear();
    snippets.clear();

    file = QLatin1String("cxx_snippets_3.cpp");
    texts << QLatin1String("List");
    snippets << QLatin1String("List<$class Item$>");

    texts << QLatin1String("Tuple");
    snippets << QLatin1String("Tuple<$class First$, $class Second$, $typename Third$>");

    QTest::newRow("case: template class insertion as snippet") << file << texts << snippets;
    texts.clear();
    snippets.clear();

    file = QLatin1String("cxx_snippets_4.cpp");
    texts << QLatin1String("clamp");
    snippets << QLatin1String("");

    texts << QLatin1String("perform");
    snippets << QLatin1String("perform<$class T$>");

    QTest::newRow("case: template function insertion as snippet") << file << texts << snippets;
    texts.clear();
    snippets.clear();
}

void ClangCodeModelPlugin::test_ObjC_hints()
{
    QFETCH(QString, file);
    QFETCH(QStringList, texts);
    QFETCH(QStringList, snippets);
    QFETCH(QStringList, hints);
    Q_ASSERT(texts.size() == snippets.size());
    Q_ASSERT(texts.size() == hints.size());

    CompletionTestHelper helper;
    helper << file;

    QList<CodeCompletionResult> proposals = helper.codeComplete();

    for (int i = 0, n = texts.size(); i < n; ++i) {
        const QString &text = texts[i];
        const QString &snippet = snippets[i];
        const QString &hint = hints[i];
        const QString snippetError =
                QLatin1String("Text and snippet mismatch: text '") + text
                + QLatin1String("', snippet '") + snippet
                + QLatin1String("', got snippet '%1'");
        const QString hintError =
                QLatin1String("Text and hint mismatch: text '") + text
                + QLatin1String("', hint\n'") + hint
                + QLatin1String(", got hint\n'%1'");

        bool hasText = false;
        QStringList texts;
        foreach (const CodeCompletionResult &ccr, proposals) {
            texts << ccr.text();
            if (ccr.text() != text)
                continue;
            hasText = true;
            QVERIFY2(snippet == ccr.snippet(), snippetError.arg(ccr.snippet()).toAscii());
            QVERIFY2(hint == ccr.hint(), hintError.arg(ccr.hint()).toAscii());
        }
        const QString textError(QString::fromLatin1("Text '%1' not found in set %2")
                                .arg(text).arg(texts.join(QLatin1String(","))));
        QVERIFY2(hasText, textError.toAscii());
    }
}

static QString makeObjCHint(const char *cHintPattern)
{
    QString hintPattern(QString::fromUtf8(cHintPattern));
    QStringList lines = hintPattern.split(QLatin1Char('\n'));
    QString hint = QLatin1String("<p>");
    bool prependNewline = false;
    foreach (const QString &line, lines) {
        if (prependNewline)
            hint += QLatin1String("<br/>");
        prependNewline = true;
        int i = 0;
        while (i < line.size() && line[i] == QLatin1Char(' ')) {
            ++i;
            hint += QLatin1String("&nbsp;");
        }
        hint += line.mid(i);
    }
    hint += QLatin1String("</p>");
    return hint;
}

void ClangCodeModelPlugin::test_ObjC_hints_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QStringList>("texts");
    QTest::addColumn<QStringList>("snippets");
    QTest::addColumn<QStringList>("hints");

    QString file;
    QStringList texts;
    QStringList snippets;
    QStringList hints;

    file = QLatin1String("objc_messages_1.mm");
    texts << QLatin1String("spectacleQuality:");
    snippets << QLatin1String("spectacleQuality:$(bool)$");
    hints << makeObjCHint("-(int) spectacleQuality:<b>(bool)</b>");
    texts << QLatin1String("desiredAmountForDramaDose:andPersonsCount:");
    snippets << QLatin1String("desiredAmountForDramaDose:$(int)$ andPersonsCount:$(int)$");
    hints << makeObjCHint("-(int) desiredAmountForDramaDose:<b>(int)</b> \n"
                          "                 andPersonsCount:<b>(int)</b>");

    QTest::newRow("case: objective-c instance messages call") << file << texts << snippets << hints;
    texts.clear();
    snippets.clear();
    hints.clear();

    file = QLatin1String("objc_messages_2.mm");
    texts << QLatin1String("eatenAmount");
    snippets << QLatin1String("(int) eatenAmount");
    hints << makeObjCHint("+(int) eatenAmount");
    texts << QLatin1String("desiredAmountForDramaDose:andPersonsCount:");
    snippets << QLatin1String("(int) desiredAmountForDramaDose:(int)dose andPersonsCount:(int)count");
    hints << makeObjCHint("+(int) desiredAmountForDramaDose:(int)dose \n"
                          "                 andPersonsCount:(int)count");

    QTest::newRow("case: objective-c class messages in @implementation") << file << texts << snippets << hints;
    texts.clear();
    snippets.clear();
    hints.clear();

    file = QLatin1String("objc_messages_3.mm");
    texts << QLatin1String("eatenAmount");
    snippets << QLatin1String("(int) eatenAmount");
    hints << makeObjCHint("-(int) eatenAmount");
    texts << QLatin1String("spectacleQuality");
    snippets << QLatin1String("(int) spectacleQuality");
    hints << makeObjCHint("-(int) spectacleQuality");
    texts << QLatin1String("desiredAmountForDramaDose:andPersonsCount:");
    snippets << QLatin1String("(int) desiredAmountForDramaDose:(int)dose andPersonsCount:(int)count");
    hints << makeObjCHint("-(int) desiredAmountForDramaDose:(int)dose \n"
                          "                 andPersonsCount:(int)count");
    texts << QLatin1String("initWithOldTracker:");
    snippets << QLatin1String("(id) initWithOldTracker:(Bbbb<Aaaa> *)aabb");
    hints << makeObjCHint("-(id) initWithOldTracker:(Bbbb&lt;Aaaa&gt; *)aabb");

    QTest::newRow("case: objective-c class messages from base class") << file << texts << snippets << hints;
    texts.clear();
    snippets.clear();
    hints.clear();
}

#endif
