/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <texteditor/generichighlighter/highlightdefinition.h>
#include <texteditor/generichighlighter/keywordlist.h>
#include <texteditor/generichighlighter/specificrules.h>
#include <texteditor/generichighlighter/progressdata.h>

#include <QtTest>

//TESTED_COMPONENT=src/plugins/texteditor/generichighlighter
using namespace TextEditor;
using namespace Internal;

class tst_SpecificRules : public QObject
{
    Q_OBJECT
public:
    tst_SpecificRules() : m_definition(new HighlightDefinition) {}

private slots:
    void initTestCase();

    void testDetectChar();
    void testDetectChar_data();

    void testDetect2Char();
    void testDetect2Char_data();

    void testAnyChar();
    void testAnyChar_data();

    void testStringDetect();
    void testStringDetect_data();

    void testRegExpr();
    void testRegExpr_data();
    void testRegExprOffsetIncremented();
    void testRegExprOffsetIncremented_data();

    void testKeywordGlobalSensitiveLocalSensitive();
    void testKeywordGlobalSensitiveLocalSensitive_data();
    void testKeywordGlobalSensitiveLocalInsensitive();
    void testKeywordGlobalSensitiveLocalInsensitive_data();
    void testKeywordGlobalInsensitiveLocalInsensitive();
    void testKeywordGlobalInsensitiveLocalInsensitive_data();
    void testKeywordGlobalInsensitiveLocalSensitive();
    void testKeywordGlobalInsensitiveLocalSensitive_data();

    void testInt();
    void testInt_data();

    void testFloat();
    void testFloat_data();

    void testCOctal();
    void testCOctal_data();

    void testCHex();
    void testCHex_data();

    void testCString();
    void testCString_data();

    void testCChar();
    void testCChar_data();

    void testRangeDetect();
    void testRangeDetect_data();

    void testLineContinue();
    void testLineContinue_data();

    void testDetectSpaces();
    void testDetectSpaces_data();

    void testDetectIdentifier();
    void testDetectIdentifier_data();

private:
    void addCommonColumns() const;
    void testMatch(Rule *rule);
    void testMatch(Rule *rule, ProgressData *progress);

    void noMatchForInt() const;
    void noMatchForFloat() const;
    void noMatchForCOctal() const;
    void noMatchForCHex() const;
    void noMatchForNumber() const;

    void commonCasesForKeywords() const;

    QSharedPointer<HighlightDefinition> m_definition;
};

void tst_SpecificRules::initTestCase()
{
    QSharedPointer<KeywordList> list = m_definition->createKeywordList("keywords");
    list->addKeyword("for");
    list->addKeyword("while");
    list->addKeyword("BEGIN");
    list->addKeyword("END");
    list->addKeyword("WeIrD");
}

void tst_SpecificRules::addCommonColumns() const
{
    QTest::addColumn<QString>("s");
    QTest::addColumn<bool>("match");
    QTest::addColumn<int>("offset");
    QTest::addColumn<bool>("only spaces");
    QTest::addColumn<bool>("will continue");
}

void tst_SpecificRules::testMatch(Rule *rule)
{
    ProgressData progress;
    testMatch(rule, &progress);
}

void tst_SpecificRules::testMatch(Rule *rule, ProgressData *progress)
{    
    QFETCH(QString, s);

    QTEST(rule->matchSucceed(s, s.length(), progress), "match");
    QTEST(progress->offset(), "offset");
    QTEST(progress->isOnlySpacesSoFar(), "only spaces");
    QTEST(progress->isWillContinueLine(), "will continue");
}

void tst_SpecificRules::testDetectChar()
{
    QFETCH(QString, c);
    DetectCharRule rule;
    rule.setChar(c);

    testMatch(&rule);
}

void tst_SpecificRules::testDetectChar_data()
{
    QTest::addColumn<QString>("c");
    addCommonColumns();

    QTest::newRow("[#] against [#]") << "#" << "#" << true << 1 << false << false;
    QTest::newRow("[#] against [##]") << "#" << "##" << true << 1 << false << false;
    QTest::newRow("[#] against [ ]") << "#" << " " << false << 0 << true << false;
    QTest::newRow("[#] against [a]") << "#" << "a" << false << 0 << true << false;
    QTest::newRow("[#] against [abc]") << "#" << "abc" << false << 0 << true << false;
    QTest::newRow("[#] against [x#]") << "#" << "x#" << false << 0 << true << false;
    QTest::newRow("[ ] against [a]") << " " << "a" << false << 0 << true << false;
    //QTest::newRow("[ ] against [ ]") << " " << " " << true << 1 << true << false;
}

void tst_SpecificRules::testDetect2Char()
{
    QFETCH(QString, c);
    QFETCH(QString, c1);
    Detect2CharsRule rule;
    rule.setChar(c);
    rule.setChar1(c1);

    testMatch(&rule);
}

void tst_SpecificRules::testDetect2Char_data()
{
    QTest::addColumn<QString>("c");
    QTest::addColumn<QString>("c1");
    addCommonColumns();

    QTest::newRow("[//] against [//]") << "/" << "/" << "//" << true << 2 << false << false;
    QTest::newRow("[//] against [///]") << "/" << "/" << "///" << true << 2 << false << false;
    QTest::newRow("[//] against [// ]") << "/" << "/" << "// " << true << 2 << false << false;
    QTest::newRow("[//] against [ //]") << "/" << "/" << " //" << false << 0 << true << false;
    QTest::newRow("[//] against [a]") << "/" << "/" << "a" << false << 0 << true << false;
    QTest::newRow("[//] against [ a]") << "/" << "/" << " a" << false << 0 << true << false;
    QTest::newRow("[//] against [abc]") << "/" << "/" << "abc" << false << 0 << true << false;
    QTest::newRow("[//] against [/a]") << "/" << "/" << "/a" << false << 0 << true << false;
    QTest::newRow("[//] against [a/]") << "/" << "/" << "a/" << false << 0 << true << false;
    QTest::newRow("[  ] against [xx]") << " " << " " << "xx" << false << 0 << true << false;
    //QTest::newRow("[  ] against [  ]") << " " << " " << " " << true << 3 << true << false;
}

void tst_SpecificRules::testAnyChar()
{
    QFETCH(QString, chars);
    AnyCharRule rule;
    rule.setCharacterSet(chars);

    testMatch(&rule);
}

void tst_SpecificRules::testAnyChar_data()
{
    QTest::addColumn<QString>("chars");
    addCommonColumns();

    QTest::newRow("[:!<>?] against [:]") << ":!<>?" << ":" << true << 1 << false << false;
    QTest::newRow("[:!<>?] against [!]") << ":!<>?" << "!" << true << 1 << false << false;
    QTest::newRow("[:!<>?] against [<]") << ":!<>?" << "<" << true << 1 << false << false;
    QTest::newRow("[:!<>?] against [>]") << ":!<>?" << ">" << true << 1 << false << false;
    QTest::newRow("[:!<>?] against [?]") << ":!<>?" << "?" << true << 1 << false << false;
    QTest::newRow("[:!<>?] against [:]") << ":!<>?" << ":" << true << 1 << false << false;
    QTest::newRow("[:!<>?] against [ ]") << ":!<>?" << " " << false << 0 << true << false;
    QTest::newRow("[:!<>?] against [#]") << ":!<>?" << "#" << false << 0 << true << false;
    QTest::newRow("[:!<>?] against [!#]") << ":!<>?" << "!#" << true << 1 << false << false;
    QTest::newRow("[:!<>?] against [#!]") << ":!<>?" << "#!" << false << 0 << true << false;
    QTest::newRow("[:] against [:]") << ":" << ":" << true << 1 << false << false;
    QTest::newRow("[:] against [#]") << ":" << "#" << false << 0 << true << false;
    //QTest::newRow("[   ] against [ ]") << "   " << " " << true << 1 << true << false;
}

void tst_SpecificRules::testStringDetect()
{
    QFETCH(QString, referenceString);
    QFETCH(QString, insensitive);
    StringDetectRule rule;
    rule.setString(referenceString);
    rule.setInsensitive(insensitive);

    testMatch(&rule);
}

void tst_SpecificRules::testStringDetect_data()
{
    QTest::addColumn<QString>("referenceString");
    QTest::addColumn<QString>("insensitive");
    addCommonColumns();

    QTest::newRow("[LL] against [LL]") << "LL" << "0" << "LL" << true << 2 << false << false;
    QTest::newRow("[LL] against [ll]") << "LL" << "0" << "ll" << false << 0 << true << false;
    QTest::newRow("[LL] against [ll] i") << "LL" << "1" << "ll" << true << 2 << false << false;
    QTest::newRow("[ll] against [ll] i") << "LL" << "1" << "LL" << true << 2 << false << false;
    QTest::newRow("[LL] against [5LL]") << "LL" << "0" << "5LL" << false << 0 << true << false;
    QTest::newRow("[LL] against [L]") << "LL" << "0" << "L" << false << 0 << true << false;
    QTest::newRow("[LL] against [LLL]") << "LL" << "0" << "LLL" << true << 2 << false << false;
    QTest::newRow("[LL] against [ ]") << "LL" << "0" << " " << false << 0 << true << false;
    QTest::newRow("[LL] against [xLLx]") << "LL" << "0" << "xLLx" << false << 0 << true << false;
    QTest::newRow("[\"\"\"] against [\"\"\"]") << "\"\"\"" << "0" << "\"\"\"" << true << 3
            << false << false;
}

void tst_SpecificRules::testRegExpr()
{
    QFETCH(QString, pattern);
    QFETCH(QString, insensitive);
    QFETCH(QString, minimal);
    RegExprRule rule;
    rule.setPattern(pattern);
    rule.setInsensitive(insensitive);
    rule.setMinimal(minimal);

    testMatch(&rule);
}

void tst_SpecificRules::testRegExpr_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("insensitive");
    QTest::addColumn<QString>("minimal");
    addCommonColumns();

    QTest::newRow("[#[a-z]+\\s+\\d] against [#as 9]") << "#[a-z]+\\s+\\d" << "0" << "0"
            << "#as 9" << true << 5 << false << false;
    QTest::newRow("[#[a-z]+\\s+\\d] against [#As 9]") << "#[a-z]+\\s+\\d" << "0" << "0"
            << "#As 9" << false << 0 << true << false;
    QTest::newRow("[#[a-z]+\\s+\\d] against [#As 9] i") << "#[a-z]+\\s+\\d" << "1" << "0"
            << "#As 9" << true << 5 << false << false;
    QTest::newRow("[#[a-z]+\\s+\\d] against [as 9]") << "#[a-z]+\\s+\\d" << "0" << "0"
            << "as 9" << false << 0 << true << false;
    QTest::newRow("[#[a-z]+\\s+\\d] against [w#as 9]") << "#[a-z]+\\s+\\d" << "0" << "0"
            << "w#as 9" << false << 0 << true << false;
    QTest::newRow("[^\\s+[a-z]] against [x]") << "^\\s+[a-z]" << "0" << "0"
            << "x" << false << 0 << true << false;
    QTest::newRow("[^\\s+[a-z]] against [  x]") << "^\\s+[a-z]" << "0" << "0"
            << "  x" << true << 3 << false << false;
    QTest::newRow("[0+] against [1001]") << "0+" << "0" << "0"
            << "1001" << false << 0 << true << false;
    QTest::newRow("[0+] against [001]") << "0+" << "0" << "0"
            << "001" << true << 2 << false << false;
    QTest::newRow("[0+] against [001]") << "0+" << "0" << "1"
            << "001" << true << 1 << false << false;
    QTest::newRow("[\\s*] against []") << "\\s*" << "0" << "0"
            << "" << false << 0 << true << false;
    //QTest::newRow("[\\s*] against []") << "\\s*" << "0" << "0"
    //      << " " << true << 1 << true << false;
}

void tst_SpecificRules::testRegExprOffsetIncremented()
{
    QFETCH(QString, pattern);
    RegExprRule rule;
    rule.setPattern(pattern);

    ProgressData progress;
    progress.setOffset(1);

    testMatch(&rule, &progress);
}

void tst_SpecificRules::testRegExprOffsetIncremented_data()
{
    QTest::addColumn<QString>("pattern");
    addCommonColumns();

    // To make sure that QRegExp::CaretAtZero is set.
    QTest::newRow("[^\\s+[a-z]] against [  x]") << "^\\s+[a-z]" << "  x" << false << 1
            << true << false;
}

void tst_SpecificRules::commonCasesForKeywords() const
{
    QTest::newRow("[for]") << "for" << true << 3 << false << false;
    QTest::newRow("[while]") << "while" << true << 5 << false << false;
    QTest::newRow("[BEGIN]") << "BEGIN" << true << 5 << false << false;
    QTest::newRow("[END]") << "END" << true << 3 << false << false;
    QTest::newRow("[WeIrD]") << "WeIrD" << true << 5 << false << false;
    QTest::newRow("[forr]") << "forr" << false << 0 << true << false;
    QTest::newRow("[for#]") << "for#" << false << 0 << true << false;
    QTest::newRow("[abc]") << "abc" << false << 0 << true << false;
    QTest::newRow("[ ]") << " " << false << 0 << true << false;
    QTest::newRow("[foe]") << "foe" << false << 0 << true << false;
    QTest::newRow("[sor]") << "sor" << false << 0 << true << false;
    QTest::newRow("[ffor]") << "ffor" << false << 0 << true << false;

    // Valid default delimiters.
    QTest::newRow("[for ]") << "for " << true << 3 << false << false;
    QTest::newRow("[for.for]") << "for.for" << true << 3 << false << false;
    QTest::newRow("[for(]") << "for(" << true << 3 << false << false;
    QTest::newRow("[for)]") << "for)" << true << 3 << false << false;
    QTest::newRow("[for:]") << "for:" << true << 3 << false << false;
    QTest::newRow("[for!]") << "for!" << true << 3 << false << false;
    QTest::newRow("[for+]") << "for+" << true << 3 << false << false;
    QTest::newRow("[for,]") << "for," << true << 3 << false << false;
    QTest::newRow("[for-]") << "for-" << true << 3 << false << false;
    QTest::newRow("[for<]") << "for>" << true << 3 << false << false;
    QTest::newRow("[for=]") << "for=" << true << 3 << false << false;
    QTest::newRow("[for>]") << "for>" << true << 3 << false << false;
    QTest::newRow("[for%]") << "for%" << true << 3 << false << false;
    QTest::newRow("[for&]") << "for&" << true << 3 << false << false;
    QTest::newRow("[for/]") << "for/" << true << 3 << false << false;
    QTest::newRow("[for;]") << "for;" << true << 3 << false << false;
    QTest::newRow("[for?]") << "for?" << true << 3 << false << false;
    QTest::newRow("[for[]") << "for[" << true << 3 << false << false;
    QTest::newRow("[for]]") << "for]" << true << 3 << false << false;
    QTest::newRow("[for^]") << "for^" << true << 3 << false << false;
    QTest::newRow("[for{]") << "for{" << true << 3 << false << false;
    QTest::newRow("[for|]") << "for|" << true << 3 << false << false;
    QTest::newRow("[for}]") << "for}" << true << 3 << false << false;
    QTest::newRow("[for~]") << "for~" << true << 3 << false << false;
    QTest::newRow("[for\\]") << "for\\" << true << 3 << false << false;
    QTest::newRow("[for*]") << "for*" << true << 3 << false << false;
    QTest::newRow("[for,for]") << "for,for" << true << 3 << false << false;
    QTest::newRow("[for\t]") << "for\t" << true << 3 << false << false;
}

void tst_SpecificRules::testKeywordGlobalSensitiveLocalSensitive()
{
    m_definition->setKeywordsSensitive("true");
    KeywordRule rule(m_definition);
    rule.setInsensitive("false");
    rule.setList("keywords");

    testMatch(&rule);
}

void tst_SpecificRules::testKeywordGlobalSensitiveLocalSensitive_data()
{
    addCommonColumns();

    commonCasesForKeywords();
    QTest::newRow("[fOr]") << "fOr" << false << 0 << true << false;
    QTest::newRow("[whilE") << "whilE" << false << 0 << true << false;
    QTest::newRow("[bEGIN]") << "bEGIN" << false << 0 << true << false;
    QTest::newRow("[end]") << "end" << false << 0 << true << false;
    QTest::newRow("[weird]") << "weird" << false << 0 << true << false;
}

void tst_SpecificRules::testKeywordGlobalSensitiveLocalInsensitive()
{
    m_definition->setKeywordsSensitive("true");
    KeywordRule rule(m_definition);
    rule.setInsensitive("true");
    rule.setList("keywords");

    testMatch(&rule);
}

void tst_SpecificRules::testKeywordGlobalSensitiveLocalInsensitive_data()
{
    addCommonColumns();

    commonCasesForKeywords();
    QTest::newRow("[fOr]") << "fOr" << true << 3 << false << false;
    QTest::newRow("[whilE") << "whilE" << true << 5 << false << false;
    QTest::newRow("[bEGIN]") << "bEGIN" << true << 5 << false << false;
    QTest::newRow("[end]") << "end" << true << 3 << false << false;
    QTest::newRow("[weird]") << "weird" << true << 5 << false << false;
}

void tst_SpecificRules::testKeywordGlobalInsensitiveLocalInsensitive()
{
    m_definition->setKeywordsSensitive("false");
    KeywordRule rule(m_definition);
    rule.setInsensitive("true");
    rule.setList("keywords");

    testMatch(&rule);
}

void tst_SpecificRules::testKeywordGlobalInsensitiveLocalInsensitive_data()
{
    testKeywordGlobalSensitiveLocalInsensitive_data();
}

void tst_SpecificRules::testKeywordGlobalInsensitiveLocalSensitive()
{
    m_definition->setKeywordsSensitive("false");
    KeywordRule rule(m_definition);
    rule.setInsensitive("false");
    rule.setList("keywords");

    testMatch(&rule);
}

void tst_SpecificRules::testKeywordGlobalInsensitiveLocalSensitive_data()
{
    testKeywordGlobalSensitiveLocalSensitive_data();
}

void tst_SpecificRules::noMatchForInt() const
{
    QTest::newRow("[1]") << "1" << false << 0 << true << false;
    QTest::newRow("[1299]") << "1299" << false << 0 << true << false;
    QTest::newRow("[10]") << "10" << false << 0 << true << false;
    QTest::newRow("[9]") << "9" << false << 0 << true << false;
}

void tst_SpecificRules::noMatchForFloat() const
{
    QTest::newRow("[4e-11]") << "4e-11" << false << 0 << true << false;
    QTest::newRow("[1e+5]") << "1e+5" << false << 0 << true << false;
    QTest::newRow("[7.321E-3]") << "7.321E-3" << false << 0 << true << false;
    QTest::newRow("[3.2E+4]") << "3.2E+4" << false << 0 << true << false;
    QTest::newRow("[0.5e-6]") << "0.5e-6" << false << 0 << true << false;
    QTest::newRow("[0.45]") << "0.45" << false << 0 << true << false;
    QTest::newRow("[6.e10]") << "6.e10" << false << 0 << true << false;
    QTest::newRow("[.2e23]") << ".2e23" << false << 0 << true << false;
    QTest::newRow("[23.]") << "23." << false << 0 << true << false;
    QTest::newRow("[2.e23]") << "2.e23" << false << 0 << true << false;
    QTest::newRow("[23e2]") << "23e2" << false << 0 << true << false;
    QTest::newRow("[4.3e]") << "4.3e" << false << 0 << true << false;
    QTest::newRow("[4.3ef]") << "4.3ef" << false << 0 << true << false;
}

void tst_SpecificRules::noMatchForCOctal() const
{
    QTest::newRow("[0]") << "0" << false << 0 << true << false;
    QTest::newRow("[07]") << "07" << false << 0 << true << false;
    QTest::newRow("[01234567]") << "01234567" << false << 0 << true << false;
}

void tst_SpecificRules::noMatchForCHex() const
{
    QTest::newRow("[0X934AF]") << "0X934AF" << false << 0 << true << false;
    QTest::newRow("[0x934af]") << "0x934af" << false << 0 << true << false;
}

void tst_SpecificRules::noMatchForNumber() const
{
    QTest::newRow("[a]") << "a" << false << 0 << true << false;
    QTest::newRow("[#]") << "#" << false << 0 << true << false;
    QTest::newRow("[ ]") << " " << false << 0 << true << false;
    QTest::newRow("[a1]") << "a1" << false << 0 << true << false;
    QTest::newRow("[.e23]") << ".e23" << false << 0 << true << false;
    QTest::newRow("[.e23]") << ".e23" << false << 0 << true << false;

    // + and - are not directly matched by number rules.
    QTest::newRow("[+1]") << "+1" << false << 0 << true << false;
    QTest::newRow("[-1]") << "-1" << false << 0 << true << false;
}

void tst_SpecificRules::testInt()
{
    IntRule rule;
    testMatch(&rule);
}

void tst_SpecificRules::testInt_data()
{
    addCommonColumns();

    noMatchForCOctal();
    noMatchForCHex();
    noMatchForNumber();

    QTest::newRow("[1]") << "1" << true << 1 << false << false;
    QTest::newRow("[1299]") << "1299" << true << 4 << false << false;
    QTest::newRow("[10]") << "10" << true << 2 << false << false;
    QTest::newRow("[9]") << "9" << true << 1 << false << false;

    // LL, U, and others are matched through child rules.
    QTest::newRow("[234U]") << "234U" << true << 3 << false << false;
    QTest::newRow("[234LL]") << "234LL" << true << 3 << false << false;
}

void tst_SpecificRules::testFloat()
{
    FloatRule rule;
    testMatch(&rule);
}

void tst_SpecificRules::testFloat_data()
{
    addCommonColumns();

    noMatchForInt();
    noMatchForCOctal();
    noMatchForCHex();
    noMatchForNumber();

    QTest::newRow("[4e-11]") << "4e-11" << true << 5 << false << false;
    QTest::newRow("[1e+5]") << "1e+5" << true << 4 << false << false;
    QTest::newRow("[7.321E-3]") << "7.321E-3" << true << 8 << false << false;
    QTest::newRow("[3.2E+4]") << "3.2E+4" << true << 6 << false << false;
    QTest::newRow("[0.5e-6]") << "0.5e-6" << true << 6 << false << false;
    QTest::newRow("[0.45]") << "0.45" << true << 4 << false << false;
    QTest::newRow("[6.e10]") << "6.e10" << true << 5 << false << false;
    QTest::newRow("[.2e23]") << ".2e23" << true << 5 << false << false;
    QTest::newRow("[23.]") << "23." << true << 3 << false << false;
    QTest::newRow("[2.e23]") << "2.e23" << true << 5 << false << false;
    QTest::newRow("[23e2]") << "23e2" << true << 4 << false << false;

    // F, L, and others are matched through child rules.
    QTest::newRow("[6.e10f]") << "6.e10f" << true << 5 << false << false;
    QTest::newRow("[0.5e-6F]") << "0.5e-6F" << true << 6 << false << false;
    QTest::newRow("[23.f]") << "23.f" << true << 3 << false << false;
    QTest::newRow("[.2L]") << ".2L" << true << 2 << false << false;
}

void tst_SpecificRules::testCOctal()
{
    HlCOctRule rule;
    testMatch(&rule);
}

void tst_SpecificRules::testCOctal_data()
{
    addCommonColumns();

    noMatchForInt();
    noMatchForCHex();
    noMatchForNumber();

    QTest::newRow("[0]") << "0" << true << 1 << false << false;
    QTest::newRow("[07]") << "07" << true << 2 << false << false;
    QTest::newRow("[01234567]") << "01234567" << true << 8 << false << false;
    QTest::newRow("[012345678]") << "012345678" << true << 8 << false << false;
    QTest::newRow("[0888]") << "0888" << true << 1 << false << false;
}

void tst_SpecificRules::testCHex()
{
    HlCHexRule rule;
    testMatch(&rule);
}

void tst_SpecificRules::testCHex_data()
{
    addCommonColumns();

    noMatchForInt();
    noMatchForFloat();
    noMatchForCOctal();
    noMatchForNumber();

    QTest::newRow("[0X934AF]") << "0X934AF" << true << 7 << false << false;
    QTest::newRow("[0x934af]") << "0x934af" << true << 7 << false << false;
    QTest::newRow("[0x2ik]") << "0x2ik" << true << 3 << false << false;
}

void tst_SpecificRules::testCString()
{
    HlCStringCharRule rule;
    testMatch(&rule);
}

void tst_SpecificRules::testCString_data()
{
    addCommonColumns();

    // Escape sequences
    QTest::newRow("[\\a]") << "\\a" << true << 2 << false << false;
    QTest::newRow("[\\b]") << "\\b" << true << 2 << false << false;
    QTest::newRow("[\\e]") << "\\e" << true << 2 << false << false;
    QTest::newRow("[\\f]") << "\\f" << true << 2 << false << false;
    QTest::newRow("[\\n]") << "\\n" << true << 2 << false << false;
    QTest::newRow("[\\r]") << "\\r" << true << 2 << false << false;
    QTest::newRow("[\\t]") << "\\t" << true << 2 << false << false;
    QTest::newRow("[\\v]") << "\\v" << true << 2 << false << false;
    QTest::newRow("[\\?]") << "\\?" << true << 2 << false << false;
    QTest::newRow("[\\']") << "\\'" << true << 2 << false << false;
    QTest::newRow("[\\\"]") << "\\\"" << true << 2 << false << false;
    QTest::newRow("[\\\\]") << "\\\\" << true << 2 << false << false;
    QTest::newRow("[\\c]") << "\\c" << false << 0 << true << false;
    QTest::newRow("[x]") << "x" << false << 0 << true << false;
    QTest::newRow("[ ]") << " " << false << 0 << true << false;
    QTest::newRow("[a]") << "x" << false << 0 << true << false;
    QTest::newRow("[r]") << "r" << false << 0 << true << false;
    QTest::newRow("[//]") << "//" << false << 0 << true << false;

    // Octal values
    QTest::newRow("[\\1]") << "\\1" << true << 2 << false << false;
    QTest::newRow("[\\12]") << "\\12" << true << 3 << false << false;
    QTest::newRow("[\\123]") << "\\123" << true << 4 << false << false;
    QTest::newRow("[\\1234]") << "\\1234" << true << 4 << false << false;
    QTest::newRow("[\\123x]") << "\\123x" << true << 4 << false << false;

    // Hex values
    QTest::newRow("[\\xa]") << "\\xa" << true << 3 << false << false;
    QTest::newRow("[\\xA]") << "\\xA" << true << 3 << false << false;
    QTest::newRow("[\\Xa]") << "\\Xa" << false << 0 << true << false;
    QTest::newRow("[\\xA10]") << "\\xA10" << true << 5 << false << false;
    QTest::newRow("[\\xA0123456789]") << "\\xA0123456789" << true << 13 << false << false;
    QTest::newRow("[\\xABCDEF]") << "\\xABCDEF" << true << 8 << false << false;
    QTest::newRow("[\\xABCDEFGHI]") << "\\xABCDEFGHI" << true << 8 << false << false;
}

void tst_SpecificRules::testCChar()
{
    HlCCharRule rule;
    testMatch(&rule);
}

void tst_SpecificRules::testCChar_data()
{
    addCommonColumns();

    // Escape sequences
    QTest::newRow("[\'\\a\']") << "\'\\a\'" << true << 4 << false << false;
    QTest::newRow("[\'\\b\']") << "\'\\b\'" << true << 4 << false << false;
    QTest::newRow("[\'\\e\']") << "\'\\e\'" << true << 4 << false << false;
    QTest::newRow("[\'\\f\']") << "\'\\f\'" << true << 4 << false << false;
    QTest::newRow("[\'\\n\']") << "\'\\n\'" << true << 4 << false << false;
    QTest::newRow("[\'\\r\']") << "\'\\r\'" << true << 4 << false << false;
    QTest::newRow("[\'\\t\']") << "\'\\t\'" << true << 4 << false << false;
    QTest::newRow("[\'\\v\']") << "\'\\v\'" << true << 4 << false << false;
    QTest::newRow("[\'\\?\']") << "\'\\?\'" << true << 4 << false << false;
    QTest::newRow("[\'\\'\']") << "\'\\'\'" << true << 4 << false << false;
    QTest::newRow("[\'\\\"\']") << "\'\\\"\'" << true << 4 << false << false;
    QTest::newRow("[\'\\\\\']") << "\'\\\\\'" << true << 4 << false << false;
    QTest::newRow("[\'\\c\']") << "\'\\c\'" << false << 0 << true << false;
    QTest::newRow("[x]") << "x" << false << 0 << true << false;
    QTest::newRow("[ ]") << " " << false << 0 << true << false;
    QTest::newRow("[a]") << "x" << false << 0 << true << false;
    QTest::newRow("[r]") << "r" << false << 0 << true << false;
    QTest::newRow("[//]") << "//" << false << 0 << true << false;
}

void tst_SpecificRules::testRangeDetect()
{
    QFETCH(QString, c);
    QFETCH(QString, c1);
    RangeDetectRule rule;
    rule.setChar(c);
    rule.setChar1(c1);

    testMatch(&rule);
}

void tst_SpecificRules::testRangeDetect_data()
{
    QTest::addColumn<QString>("c");
    QTest::addColumn<QString>("c1");
    addCommonColumns();

    QTest::newRow("[<>] against [<QString>]") << "<" << ">" << "<QString>"
            << true << 9 << false << false;
    QTest::newRow("[<>] against <x>") << "<" << ">" << "<x>" << true << 3 << false << false;
    QTest::newRow("[<>] against <   >") << "<" << ">" << "<   >" << true << 5 << false << false;
    QTest::newRow("[<>] against <x>abc") << "<" << ">" << "<x>abc" << true << 3 << false << false;
    QTest::newRow("[<>] against <x> ") << "<" << ">" << "<x> " << true << 3 << false << false;
    QTest::newRow("[<>] against abc") << "<" << ">" << "abc" << false << 0 << true << false;
    QTest::newRow("[<>] against <abc") << "<" << ">" << "<abc" << false << 0 << true << false;
    QTest::newRow("[<>] against abc<") << "<" << ">" << "abc<" << false << 0 << true << false;
    QTest::newRow("[<>] against a<bc") << "<" << ">" << "a<bc" << false << 0 << true << false;
    QTest::newRow("[<>] against abc<") << "<" << ">" << "abc<" << false << 0 << true << false;
    QTest::newRow("[\"\"] against \"test.h\"") << "\"" << "\"" << "\"test.h\""
            << true << 8 << false << false;
}

void tst_SpecificRules::testLineContinue()
{
    LineContinueRule rule;
    testMatch(&rule);
}

void tst_SpecificRules::testLineContinue_data()
{
    addCommonColumns();

    QTest::newRow("\\") << "\\" << true << 1 << false << true;
    QTest::newRow("\\\\") << "\\\\" << false << 0 << true << false;
    QTest::newRow("\\x") << "\\x" << false << 0 << true << false;
    QTest::newRow("x\\") << "x\\" << false << 0 << true << false;
    QTest::newRow("x") << "x" << false << 0 << true << false;
    QTest::newRow("/") << "/" << false << 0 << true << false;
}

void tst_SpecificRules::testDetectSpaces()
{
    DetectSpacesRule rule;
    testMatch(&rule);
}

void tst_SpecificRules::testDetectSpaces_data()
{
    addCommonColumns();

    QTest::newRow(" ") << " " << true << 1 << true << false;
    QTest::newRow("    ") << "    " << true << 4 << true << false;
    QTest::newRow("\t") << "\t" << true << 1 << true << false;
    QTest::newRow("x") << "x" << false << 0 << true << false;
    QTest::newRow("#") << "#" << false << 0 << true << false;
}

void tst_SpecificRules::testDetectIdentifier()
{
    DetectIdentifierRule rule;
    testMatch(&rule);
}

void tst_SpecificRules::testDetectIdentifier_data()
{
    addCommonColumns();

    QTest::newRow("name") << "name" << true << 4 << false << false;
    QTest::newRow("x") << "x" << true << 1 << false << false;
    QTest::newRow("x1") << "x1" << true << 2 << false << false;
    QTest::newRow("1x") << "1x" << false << 0 << true << false;
    QTest::newRow("_x") << "_x" << true << 2 << false << false;
    QTest::newRow("___x") << "___x" << true << 4 << false << false;
    QTest::newRow("-x") << "-x" << false << 0 << true << false;
    QTest::newRow("@x") << "@x" << false << 0 << true << false;
    QTest::newRow("+x") << "+x" << false << 0 << true << false;
    QTest::newRow("#x") << "#x" << false << 0 << true << false;
    QTest::newRow("x_x") << "x_x" << true << 3 << false << false;
    QTest::newRow("x1x") << "x1x" << true << 3 << false << false;
    QTest::newRow("x#x") << "x#x" << true << 1 << false << false;
    QTest::newRow("x-x") << "x-x" << true << 1 << false << false;
    QTest::newRow("abc_") << "abc_" << true << 4 << false << false;
    QTest::newRow("abc____") << "abc____" << true << 7 << false << false;
    QTest::newRow("abc-") << "abc-" << true << 3 << false << false;
    QTest::newRow("abc$") << "abc$" << true << 3 << false << false;
}

QTEST_MAIN(tst_SpecificRules)
#include "tst_specificrules.moc"
