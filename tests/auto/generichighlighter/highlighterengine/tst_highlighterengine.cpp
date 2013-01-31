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

#include "highlightdefinition.h"
#include "keywordlist.h"
#include "itemdata.h"
#include "context.h"
#include "specificrules.h"
#include "highlightermock.h"
#include "formats.h"

#include <QSharedPointer>
#include <QScopedPointer>
#include <QList>
#include <QMetaType>
#include <QPlainTextEdit>
#include <QtTest>

using namespace TextEditor;
using namespace Internal;

Q_DECLARE_METATYPE(HighlightSequence)
Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QList<HighlightSequence>)

//TESTED_COMPONENT=src/plugins/texteditor/generichighlighter

class tst_HighlighterEngine : public QObject
{
    Q_OBJECT
public:
    tst_HighlighterEngine();

private slots:    
    void initTestCase();
    void init();

    void testSimpleLine();
    void testSimpleLine_data();

    void testLineContinue();
    void testLineContinue_data();
    void testEditingLineContinue0();
    void testEditingLineContinue1();
    void testEditingLineContinue2();
    void testEditingLineContinue3();
    void testEditingLineContinue4();
    void testEditingLineContinue5();

    void testPersistentStates();
    void testPersistentStates_data();
    void testEditingPersistentStates0();
    void testEditingPersistentStates1();
    void testEditingPersistentStates2();

    void testDynamicContexts();
    void testDynamicContexts_data();

    void testFirstNonSpace();
    void testFirstNonSpace_data();

private:
    void createKeywords();
    void createContexts();
    void createItemDatas();

    void setExpectedData(int state, const HighlightSequence &seq);
    void setExpectedData(const QList<int> &states, const QList<HighlightSequence> &seqs);
    void createColumns();
    void test();
    void test(int state, const HighlightSequence &seq, const QString &line);
    void test(const QList<int> &states, const QList<HighlightSequence> &seqs, const QString &lines);

    void clear(QList<int> *states, QList<HighlightSequence> *sequences) const;

    QList<int> createlDefaultStatesList(int size) const;

    void addCharactersToBegin(QTextBlock block, const QString &s);
    void addCharactersToEnd(QTextBlock block, const QString &s);
    void removeFirstCharacters(const QTextBlock &block, int n);
    void removeLastCharacters(const QTextBlock &block, int n);

    void setupForEditingLineContinue();

    QSharedPointer<HighlightDefinition> m_definition;
    QScopedPointer<HighlighterMock> m_highlighterMock;
    QPlainTextEdit m_text;
};

tst_HighlighterEngine::tst_HighlighterEngine() :
    m_definition(new HighlightDefinition)
{}

void tst_HighlighterEngine::initTestCase()
{
            /***********************************************************/
            /*** Spaces are ignored when comparing text char formats ***/
            /***********************************************************/

    createKeywords();
    createContexts();
    createItemDatas();

    m_highlighterMock.reset(new HighlighterMock());
    m_highlighterMock->setDefaultContext(m_definition->initialContext());
    m_highlighterMock->setDocument(m_text.document());
    m_highlighterMock->configureFormat(Highlighter::Keyword, Formats::instance().keywordFormat());
    m_highlighterMock->configureFormat(Highlighter::DataType, Formats::instance().dataTypeFormat());
    m_highlighterMock->configureFormat(Highlighter::Decimal, Formats::instance().decimalFormat());
    m_highlighterMock->configureFormat(Highlighter::BaseN, Formats::instance().baseNFormat());
    m_highlighterMock->configureFormat(Highlighter::Float, Formats::instance().floatFormat());
    m_highlighterMock->configureFormat(Highlighter::Char, Formats::instance().charFormat());
    m_highlighterMock->configureFormat(Highlighter::String, Formats::instance().stringFormat());
    m_highlighterMock->configureFormat(Highlighter::Comment, Formats::instance().commentFormat());
    m_highlighterMock->configureFormat(Highlighter::Alert, Formats::instance().alertFormat());
    m_highlighterMock->configureFormat(Highlighter::Error, Formats::instance().errorFormat());
    m_highlighterMock->configureFormat(Highlighter::Function, Formats::instance().functionFormat());
    m_highlighterMock->configureFormat(Highlighter::RegionMarker,
                                       Formats::instance().regionMarketFormat());
    m_highlighterMock->configureFormat(Highlighter::Others, Formats::instance().othersFormat());
}

void tst_HighlighterEngine::init()
{
    m_highlighterMock->reset();
}

void tst_HighlighterEngine::createKeywords()
{
    QSharedPointer<KeywordList> keywords = m_definition->createKeywordList("keywords");
    keywords->addKeyword("int");
    keywords->addKeyword("long");
}

void tst_HighlighterEngine::createContexts()
{
    // Normal context
    QSharedPointer<Context> normal = m_definition->createContext("Normal", true);
    normal->setItemData("Normal Text");
    normal->setLineEndContext("#stay");
    normal->setDefinition(m_definition);

    // AfterHash context
    QSharedPointer<Context> afterHash = m_definition->createContext("AfterHash", false);
    afterHash->setItemData("Error");
    afterHash->setLineEndContext("#pop");
    afterHash->setDefinition(m_definition);

    // Define context
    QSharedPointer<Context> define = m_definition->createContext("Define", false);
    define->setItemData("Preprocessor");
    define->setLineEndContext("#pop");
    define->setDefinition(m_definition);

    // Preprocessor context
    QSharedPointer<Context> preprocessor = m_definition->createContext("Preprocessor", false);
    preprocessor->setItemData("Preprocessor");
    preprocessor->setLineEndContext("#pop");
    preprocessor->setDefinition(m_definition);

    // SimpleComment context
    QSharedPointer<Context> simpleComment = m_definition->createContext("SimpleComment", false);
    simpleComment->setItemData("Comment");
    simpleComment->setLineEndContext("#pop");
    simpleComment->setDefinition(m_definition);

    // MultilineComment context
    QSharedPointer<Context> multiComment = m_definition->createContext("MultilineComment", false);
    multiComment->setItemData("Comment");
    multiComment->setLineEndContext("#stay");
    multiComment->setDefinition(m_definition);

    // NestedComment context
    QSharedPointer<Context> nestedComment = m_definition->createContext("NestedComment", false);
    nestedComment->setItemData("Other Comment");
    nestedComment->setLineEndContext("#stay");
    nestedComment->setDefinition(m_definition);

    // Dummy context
    QSharedPointer<Context> dummy = m_definition->createContext("Dummy", false);
    dummy->setItemData("Dummy");
    dummy->setLineEndContext("#pop");
    dummy->setDefinition(m_definition);

    // AfterPlus context
    QSharedPointer<Context> afterPlus = m_definition->createContext("AfterPlus", false);
    afterPlus->setItemData("Char");
    afterPlus->setLineEndContext("#pop");
    afterPlus->setDefinition(m_definition);

    // AfterMinus context
    QSharedPointer<Context> afterMinus = m_definition->createContext("AfterMinus", false);
    afterMinus->setItemData("String");
    afterMinus->setLineEndContext("#pop#pop");
    afterMinus->setDefinition(m_definition);

    // AfterEqual context
    QSharedPointer<Context> afterEqual = m_definition->createContext("AfterEqual", false);
    afterEqual->setItemData("Float");
    afterEqual->setLineEndContext("#pop#pop#pop");
    afterEqual->setDefinition(m_definition);

    // Dynamic context
    QSharedPointer<Context> dynamic = m_definition->createContext("Dynamic", false);
    dynamic->setItemData("Marker");
    dynamic->setLineEndContext("#pop");
    dynamic->setDynamic("true");
    dynamic->setDefinition(m_definition);

    // Rules
    DetectCharRule *r = new DetectCharRule;
    r->setChar("?");
    r->setContext("#stay");
    r->setItemData("Decimal");
    r->setDefinition(m_definition);
    normal->addRule(QSharedPointer<Rule>(r)); // Needs to be the first one added.

    DetectCharRule *r0 = new DetectCharRule;
    r0->setChar("#");
    r0->setContext("AfterHash");
    r0->setFirstNonSpace("true");
    r0->setLookAhead("true");
    r0->setDefinition(m_definition);
    normal->addRule(QSharedPointer<Rule>(r0));

    RegExprRule *r1 = new RegExprRule;
    r1->setPattern("#\\s*define.*((?=\\\\))");
    r1->setInsensitive("true");
    r1->setContext("Define");
    r1->setItemData("Preprocessor");
    r1->setFirstNonSpace("true");
    r1->setDefinition(m_definition);
    afterHash->addRule(QSharedPointer<Rule>(r1));

    RegExprRule *r2 = new RegExprRule;
    r2->setPattern("#\\s*(?:define|undef)");
    r2->setInsensitive("true");
    r2->setContext("Preprocessor");
    r2->setItemData("Preprocessor");
    r2->setFirstNonSpace("true");
    r2->setDefinition(m_definition);
    afterHash->addRule(QSharedPointer<Rule>(r2));

    LineContinueRule *r3 = new LineContinueRule;
    r3->setItemData("Preprocessor");
    r3->setContext("#stay");
    r3->setDefinition(m_definition);
    define->addRule(QSharedPointer<Rule>(r3));

    LineContinueRule *r4 = new LineContinueRule;
    r4->setItemData("Preprocessor");
    r4->setContext("#stay");
    r4->setDefinition(m_definition);
    preprocessor->addRule(QSharedPointer<Rule>(r4));

    KeywordRule *r5 = new KeywordRule(m_definition);
    r5->setList("keywords");
    r5->setItemData("Keyword");
    r5->setContext("#stay");
    r5->setDefinition(m_definition);
    normal->addRule(QSharedPointer<Rule>(r5));

    IntRule *r6 = new IntRule;
    r6->setItemData("Decimal");
    r6->setContext("#stay");
    r6->setDefinition(m_definition);
    normal->addRule(QSharedPointer<Rule>(r6));

    StringDetectRule *r7 = new StringDetectRule;
    r7->setItemData("Decimal");
    r7->setContext("#stay");
    r7->setString("LL");
    r7->setInsensitive("true");
    r7->setDefinition(m_definition);
    r6->addChild(QSharedPointer<Rule>(r7));

    StringDetectRule *r8 = new StringDetectRule;
    r8->setItemData("Decimal");
    r8->setContext("#stay");
    r8->setString("UL");
    r8->setInsensitive("true");
    r8->setDefinition(m_definition);
    r6->addChild(QSharedPointer<Rule>(r8));

    HlCOctRule *r9 = new HlCOctRule;
    r9->setItemData("Octal");
    r9->setContext("#stay");
    r9->setDefinition(m_definition);
    normal->addRule(QSharedPointer<Rule>(r9));

    Detect2CharsRule *r10 = new Detect2CharsRule;
    r10->setChar("/");
    r10->setChar1("/");
    r10->setItemData("Comment");
    r10->setContext("SimpleComment");
    r10->setDefinition(m_definition);
    normal->addRule(QSharedPointer<Rule>(r10));

    DetectIdentifierRule *r11 = new DetectIdentifierRule;
    r11->setDefinition(m_definition);
    simpleComment->addRule(QSharedPointer<Rule>(r11));

    Detect2CharsRule *r12 = new Detect2CharsRule;
    r12->setChar("/");
    r12->setChar1("*");
    r12->setItemData("Comment");
    r12->setContext("MultilineComment");
    r12->setDefinition(m_definition);
    normal->addRule(QSharedPointer<Rule>(r12));

    Detect2CharsRule *r13 = new Detect2CharsRule;
    r13->setChar("*");
    r13->setChar1("/");
    r13->setItemData("Comment");
    r13->setContext("#pop");
    r13->setDefinition(m_definition);
    multiComment->addRule(QSharedPointer<Rule>(r13));

    Detect2CharsRule *r14 = new Detect2CharsRule;
    r14->setChar("/");
    r14->setChar1("#");
    r14->setItemData("Other Comment");
    r14->setContext("NestedComment");
    r14->setDefinition(m_definition);
    QSharedPointer<Rule> sr14(r14);
    multiComment->addRule(sr14);
    dummy->addRule(sr14);
    afterEqual->addRule(sr14);
    afterMinus->addRule(sr14);
    afterPlus->addRule(sr14);

    Detect2CharsRule *r15 = new Detect2CharsRule;
    r15->setChar("#");
    r15->setChar1("/");
    r15->setItemData("Other Comment");
    r15->setContext("#pop");
    r15->setDefinition(m_definition);
    nestedComment->addRule(QSharedPointer<Rule>(r15));

    DetectCharRule *r16 = new DetectCharRule;
    r16->setChar("@");
    r16->setItemData("Marker");
    r16->setContext("Dummy");
    r16->setDefinition(m_definition);
    multiComment->addRule(QSharedPointer<Rule>(r16));

    StringDetectRule *r17 = new StringDetectRule;
    r17->setString("dummy");
    r17->setItemData("Dummy");
    r17->setContext("#stay");
    r17->setDefinition(m_definition);
    dummy->addRule(QSharedPointer<Rule>(r17));

    DetectCharRule *r18 = new DetectCharRule;
    r18->setChar("+");
    r18->setContext("AfterPlus");
    r18->setDefinition(m_definition);
    QSharedPointer<Rule> sr18(r18);
    multiComment->addRule(sr18);
    nestedComment->addRule(sr18);
    afterMinus->addRule(sr18);
    afterEqual->addRule(sr18);

    DetectCharRule *r19 = new DetectCharRule;
    r19->setChar("-");
    r19->setContext("AfterMinus");
    r19->setDefinition(m_definition);
    QSharedPointer<Rule> sr19(r19);
    multiComment->addRule(sr19);
    nestedComment->addRule(sr19);
    afterPlus->addRule(sr19);
    afterEqual->addRule(sr19);

    DetectCharRule *r20 = new DetectCharRule;
    r20->setChar("=");
    r20->setContext("AfterEqual");
    r20->setDefinition(m_definition);
    QSharedPointer<Rule> sr20(r20);
    multiComment->addRule(sr20);
    nestedComment->addRule(sr20);
    afterPlus->addRule(sr20);
    afterMinus->addRule(sr20);

    DetectCharRule *r22 = new DetectCharRule;
    r22->setChar("$");
    r22->setContext("#stay");
    r22->setDefinition(m_definition);
    normal->addRule(QSharedPointer<Rule>(r22));

    DetectCharRule *r23 = new DetectCharRule;
    r23->setChar("?");
    r23->setContext("#stay");
    r23->setItemData("Comment");
    r23->setDefinition(m_definition);
    normal->addRule(QSharedPointer<Rule>(r23));

    RegExprRule *r24 = new RegExprRule;
    r24->setInsensitive("true");
    r24->setPattern("---([a-c]*)([d-f]*)");
    r24->setDefinition(m_definition);
    r24->setItemData("Dummy");
    r24->setContext("Dynamic");
    normal->addRule(QSharedPointer<Rule>(r24));

    DetectCharRule *r25 = new DetectCharRule;
    r25->setChar("1");
    r25->setItemData("Preprocessor");
    r25->setActive("true");
    r25->setDefinition(m_definition);
    dynamic->addRule(QSharedPointer<Rule>(r25));

    StringDetectRule *r26 = new StringDetectRule;
    r26->setString("begin%2end");
    r26->setItemData("Error");
    r26->setActive("true");
    r26->setDefinition(m_definition);
    dynamic->addRule(QSharedPointer<Rule>(r26));

    DetectCharRule *r27 = new DetectCharRule;
    r27->setChar("|");
    r27->setItemData("Error");
    r27->setFirstNonSpace("true");
    r27->setDefinition(m_definition);
    normal->addRule(QSharedPointer<Rule>(r27));
}

void tst_HighlighterEngine::createItemDatas()
{
    QSharedPointer<ItemData> normalText = m_definition->createItemData("Normal Text");
    normalText->setStyle("dsNormal");
    QSharedPointer<ItemData> preprocessor = m_definition->createItemData("Preprocessor");
    preprocessor->setStyle("dsOthers");
    QSharedPointer<ItemData> error = m_definition->createItemData("Error");
    error->setStyle("dsError");
    QSharedPointer<ItemData> keyword = m_definition->createItemData("Keyword");
    keyword->setStyle("dsKeyword");
    QSharedPointer<ItemData> decimal = m_definition->createItemData("Decimal");
    decimal->setStyle("dsDecVal");
    QSharedPointer<ItemData> octal = m_definition->createItemData("Octal");
    octal->setStyle("dsBaseN");
    QSharedPointer<ItemData> comment = m_definition->createItemData("Comment");
    comment->setStyle("dsComment");
    QSharedPointer<ItemData> otherComment = m_definition->createItemData("Other Comment");
    otherComment->setStyle("dsError");
    QSharedPointer<ItemData> marker = m_definition->createItemData("Marker");
    marker->setStyle("dsRegionMarker");
    QSharedPointer<ItemData> dummy = m_definition->createItemData("Dummy");
    dummy->setStyle("dsDataType");
    QSharedPointer<ItemData> charStyle = m_definition->createItemData("Char");
    charStyle->setStyle("dsChar");
    QSharedPointer<ItemData> stringStyle = m_definition->createItemData("String");
    stringStyle->setStyle("dsString");
    QSharedPointer<ItemData> floatStyle = m_definition->createItemData("Float");
    floatStyle->setStyle("dsFloat");
}

void tst_HighlighterEngine::setExpectedData(int state, const HighlightSequence &seq)
{
    m_highlighterMock->setExpectedBlockState(state);
    m_highlighterMock->setExpectedHighlightSequence(seq);
}

void tst_HighlighterEngine::setExpectedData(const QList<int> &states,
                                            const QList<HighlightSequence> &seqs)
{
    m_highlighterMock->setExpectedBlockStates(states);
    m_highlighterMock->setExpectedHighlightSequences(seqs);
}

void tst_HighlighterEngine::createColumns()
{
    QTest::addColumn<QList<int> >("states");
    QTest::addColumn<QList<HighlightSequence> >("sequences");
    QTest::addColumn<QString>("lines");
}

void tst_HighlighterEngine::test()
{
    QFETCH(QList<int>, states);
    QFETCH(QList<HighlightSequence>, sequences);
    QFETCH(QString, lines);

    test(states, sequences, lines);
}

void tst_HighlighterEngine::test(int state, const HighlightSequence &seq, const QString &line)
{
    setExpectedData(state, seq);
    m_text.setPlainText(line);
}

void tst_HighlighterEngine::test(const QList<int> &states,
                                 const QList<HighlightSequence> &seqs,
                                 const QString &lines)
{
    setExpectedData(states, seqs);
    m_text.setPlainText(lines);
}

void tst_HighlighterEngine::clear(QList<int> *states, QList<HighlightSequence> *sequences) const
{
    states->clear();
    sequences->clear();
}

QList<int> tst_HighlighterEngine::createlDefaultStatesList(int size) const
{
    QList<int> states;
    for (int i = 0; i < size; ++i)
        states.append(0);
    return states;
}

void tst_HighlighterEngine::addCharactersToBegin(QTextBlock block, const QString &s)
{
    QTextCursor cursor = m_text.textCursor();
    cursor.beginEditBlock();
    cursor.setPosition(block.position());
    cursor.insertText(s);
    cursor.endEditBlock();
}

void tst_HighlighterEngine::addCharactersToEnd(QTextBlock block, const QString &s)
{
    QTextCursor cursor = m_text.textCursor();
    cursor.beginEditBlock();
    cursor.setPosition(block.position() + block.length() - 1);
    cursor.insertText(s);
    cursor.endEditBlock();
}

void tst_HighlighterEngine::removeFirstCharacters(const QTextBlock &block, int n)
{
    QTextCursor cursor = m_text.textCursor();
    cursor.beginEditBlock();
    cursor.setPosition(block.position());
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, n);
    cursor.removeSelectedText();
    cursor.endEditBlock();
}

void tst_HighlighterEngine::removeLastCharacters(const QTextBlock &block, int n)
{
    QTextCursor cursor = m_text.textCursor();
    cursor.beginEditBlock();
    cursor.setPosition(block.position() + block.length() - 1);
    cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, n);
    cursor.removeSelectedText();
    cursor.endEditBlock();
}

void tst_HighlighterEngine::testSimpleLine()
{
    test();
}

void tst_HighlighterEngine::testSimpleLine_data()
{
    createColumns();

    QList<int> states;
    QList<HighlightSequence> sequences;
    QString text;

    HighlightSequence seqa(0, 3);
    HighlightSequence seqb(0, 15, Formats::instance().othersFormat());
    HighlightSequence seqc(0, 1, Formats::instance().errorFormat());
    HighlightSequence seqd(0, 3, Formats::instance().keywordFormat());
    seqd.add(3, 8);
    seqd.add(8, 9, Formats::instance().baseNFormat());
    HighlightSequence seqe(0, 4, Formats::instance().keywordFormat());
    seqe.add(4, 9);
    seqe.add(9, 12, Formats::instance().decimalFormat());
    HighlightSequence seqf(seqe);
    seqf.add(12, 13);
    HighlightSequence seqg(seqf);
    seqg.add(13, 14);
    HighlightSequence seqh(0, 8, Formats::instance().commentFormat());
    HighlightSequence seqi(seqd);
    seqi.add(9, 17, Formats::instance().commentFormat());
    HighlightSequence seqj(seqd);
    seqj.add(9, 11, Formats::instance().commentFormat());    
    HighlightSequence seqk(0, 3);
    HighlightSequence seql(0, 3, Formats::instance().keywordFormat());
    HighlightSequence seqm(0, 2);
    HighlightSequence seqn(0, 8, Formats::instance().commentFormat());
    HighlightSequence seqo(0, 1);
    seqo.add(1, 2, Formats::instance().decimalFormat());

    states << 0;
    sequences << seqa;
    text = "abc";
    QTest::newRow("case 0") << states << sequences << text;

    sequences.clear();
    sequences << seqb;
    text = "#define max 100";
    QTest::newRow("case 1") << states << sequences << text;

    sequences.clear();
    sequences << seqc;
    text = "#";
    QTest::newRow("case 2") << states << sequences << text;

    sequences.clear();
    sequences << seqd;
    text = "int i = 0";
    QTest::newRow("case 3") << states << sequences << text;

    sequences.clear();
    sequences << seqe;
    text = "long i = 1LL";
    QTest::newRow("case 4") << states << sequences << text;

    text = "long i = 1ul";
    QTest::newRow("case 5") << states << sequences << text;

    sequences.clear();
    sequences << seqf;
    text = "long i = 1ULL";
    QTest::newRow("case 6") << states << sequences << text;

    sequences.clear();
    sequences << seqg;
    text = "long i = 1LLUL";
    QTest::newRow("case 7") << states << sequences << text;

    text = "long i = 1ULLL";
    QTest::newRow("case 8") << states << sequences << text;

    sequences.clear();
    sequences << seqh;
    text = "//int i;";
    QTest::newRow("case 9") << states << sequences << text;

    sequences.clear();
    sequences << seqi;
    text = "int i = 0//int i;";
    QTest::newRow("case 10") << states << sequences << text;

    sequences.clear();
    sequences << seqj;
    text = "int i = 0//";
    QTest::newRow("case 11") << states << sequences << text;

    sequences.clear();
    sequences << seqk << seqk;
    text = "bla\nbla";
    QTest::newRow("case 12") << createlDefaultStatesList(2) << sequences << text;

    sequences.clear();
    sequences << seql << seqm;
    text = "int\ni;";
    QTest::newRow("case 13") << createlDefaultStatesList(2) << sequences << text;

    sequences.clear();
    sequences << seqn << seqm;
    text = "//int i;\ni;";
    QTest::newRow("case 14") << createlDefaultStatesList(2) << sequences << text;

    // Even when a matching rule does not take to another context, iteration over the rules
    // should start over from the first rule in the current context.
    sequences.clear();
    sequences << seqo;
    text = "$?";
    QTest::newRow("case 15") << createlDefaultStatesList(2) << sequences << text;
}

void tst_HighlighterEngine::testLineContinue()
{
    test();
}

void tst_HighlighterEngine::testLineContinue_data()
{
    createColumns();

    QList<int> states;
    QList<HighlightSequence> sequences;
    QString lines;

    HighlightSequence seqa(0, 12, Formats::instance().othersFormat());
    HighlightSequence seqb(0, 7, Formats::instance().othersFormat());
    HighlightSequence seqc(0, 8, Formats::instance().othersFormat());
    HighlightSequence seqd(0, 3, Formats::instance().othersFormat());
    HighlightSequence seqe(0, 3, Formats::instance().keywordFormat());
    seqe.add(3, 8);
    seqe.add(8, 9, Formats::instance().baseNFormat());
    seqe.add(9, 10);

    states << 1 << 2;
    sequences << seqa << seqb;
    lines = "#define max\\\n    100";
    QTest::newRow("case 0") << states << sequences << lines;

    clear(&states, &sequences);
    states << 1 << 1;
    sequences << seqa << seqc;
    lines = "#define max\\\n    100\\";
    QTest::newRow("case 1") << states << sequences << lines;

    clear(&states, &sequences);
    states << 1 << 1 << 2;
    sequences << seqa << seqc << seqd;
    lines = "#define max\\\n    100\\\n000";
    QTest::newRow("case 2") << states << sequences << lines;

    clear(&states, &sequences);
    states << 1 << 1 << 2 << 0;
    sequences << seqa << seqc << seqd << seqe;
    lines = "#define max\\\n    100\\\n000\nint i = 0;";
    QTest::newRow("case 3") << states << sequences << lines;
}

void tst_HighlighterEngine::setupForEditingLineContinue()
{
    m_highlighterMock->startNoTestCalls();
    m_text.setPlainText("#define max\\\n    xxx\\\nzzz");
    m_highlighterMock->endNoTestCalls();
}

void tst_HighlighterEngine::testEditingLineContinue0()
{
    setupForEditingLineContinue();

    QList<HighlightSequence> sequences;
    HighlightSequence seqa(0, 11, Formats::instance().othersFormat());
    HighlightSequence seqb(0, 8);
    HighlightSequence seqc(0, 3);
    sequences << seqa << seqb << seqc;
    setExpectedData(createlDefaultStatesList(3), sequences);

    removeLastCharacters(m_text.document()->firstBlock(), 1);
}

void tst_HighlighterEngine::testEditingLineContinue1()
{
    setupForEditingLineContinue();

    setExpectedData(1, HighlightSequence(0, 6, Formats::instance().othersFormat()));

    // In this case highlighting should be triggered only for the modified line.
    removeFirstCharacters(m_text.document()->firstBlock().next(), 2);
}

void tst_HighlighterEngine::testEditingLineContinue2()
{
    setupForEditingLineContinue();

    QList<HighlightSequence> sequences;
    HighlightSequence seqa(0, 17, Formats::instance().othersFormat());
    HighlightSequence seqb(0, 8);
    HighlightSequence seqc(0, 3);
    sequences << seqa << seqb << seqc;
    setExpectedData(createlDefaultStatesList(3), sequences);

    addCharactersToEnd(m_text.document()->firstBlock(), "ixum");
}

void tst_HighlighterEngine::testEditingLineContinue3()
{
    setupForEditingLineContinue();

    setExpectedData(1, HighlightSequence(0, 12, Formats::instance().othersFormat()));

    // In this case highlighting should be triggered only for the modified line.
    addCharactersToBegin(m_text.document()->firstBlock().next(), "ixum");
}

void tst_HighlighterEngine::testEditingLineContinue4()
{
    setupForEditingLineContinue();

    QList<int> states;
    states << 2 << 0 << 0;
    QList<HighlightSequence> sequences;
    HighlightSequence seqa(0, 0);
    HighlightSequence seqb(0, 8);
    HighlightSequence seqc(0, 3);
    sequences << seqa << seqb << seqc;
    setExpectedData(states, sequences);

    m_highlighterMock->considerEmptyLines();
    addCharactersToBegin(m_text.document()->firstBlock().next(), "\n");
}

void tst_HighlighterEngine::testEditingLineContinue5()
{
    setupForEditingLineContinue();

    QList<int> states;
    states << 2 << 0;
    QList<HighlightSequence> sequences;
    HighlightSequence seqa(0, 9, Formats::instance().othersFormat());
    HighlightSequence seqb(0, 3);
    sequences << seqa << seqb;
    setExpectedData(states, sequences);

    addCharactersToEnd(m_text.document()->firstBlock().next(), "x");
}

void tst_HighlighterEngine::testPersistentStates()
{
    test();
}

void tst_HighlighterEngine::testPersistentStates_data()
{
    createColumns();

    QList<int> states;
    QList<HighlightSequence> sequences;
    QString text;

    HighlightSequence seqa(0, 3, Formats::instance().keywordFormat());
    seqa.add(3, 6);
    seqa.add(6, 15, Formats::instance().commentFormat());
    seqa.add(15, 16);
    HighlightSequence seqb(0, 8, Formats::instance().commentFormat());
    HighlightSequence seqc(0, 2, Formats::instance().commentFormat());
    HighlightSequence seqd(0, 9, Formats::instance().commentFormat());
    seqd.add(9, 18, Formats::instance().errorFormat());
    HighlightSequence seqe(0, 5, Formats::instance().errorFormat());
    seqe.add(5, 8, Formats::instance().commentFormat());
    HighlightSequence seqf(0, 2, Formats::instance().commentFormat());
    seqf.add(2, 6);
    HighlightSequence seqg(0, 1);
    seqg.add(1, 7, Formats::instance().commentFormat());
    seqg.add(7, 8, Formats::instance().regionMarketFormat());
    seqg.add(8, 15, Formats::instance().errorFormat());
    seqg.add(15, 16, Formats::instance().regionMarketFormat());
    seqg.add(16, 21, Formats::instance().dataTypeFormat());
    seqg.add(21, 22, Formats::instance().regionMarketFormat());
    HighlightSequence seqh(seqc);
    seqh.add(2, 3);
    HighlightSequence seqi(seqc);
    seqi.add(2, 3, Formats::instance().charFormat());
    seqi.add(3, 4, Formats::instance().stringFormat());
    HighlightSequence seqj(seqc);
    seqj.add(2, 3, Formats::instance().charFormat());
    seqj.add(3, 4, Formats::instance().floatFormat());
    HighlightSequence seqk(seqc);
    seqk.add(2, 3, Formats::instance().charFormat());
    seqk.add(3, 5, Formats::instance().errorFormat());
    seqk.add(5, 6, Formats::instance().floatFormat());
    HighlightSequence seql(seqk);
    seql.add(6, 7, Formats::instance().stringFormat());

    states << 0;
    sequences << seqa;
    text = "int i /* 000 */;";
    QTest::newRow("case 0") << states << sequences << text;

    clear(&states, &sequences);
    states << 3 << 3;
    sequences << seqb << seqc;
    text = "/*int i;\ni;";
    QTest::newRow("case 1") << states << sequences << text;

    clear(&states, &sequences);
    states << 3 << 3 << 3;
    sequences << seqb << seqc << seqb;
    text = "/*int i;\ni;\nint abc;";
    QTest::newRow("case 2") << states << sequences << text;

    clear(&states, &sequences);
    states << 3 << 3 << 0;
    sequences << seqb << seqc << seqc;
    text = "/*int i;\ni;\n*/";
    QTest::newRow("case 3") << states << sequences << text;

    clear(&states, &sequences);
    states << 4 << 3 << 0;
    sequences << seqd << seqe << seqf;
    text = "/*int i; /# int j;\nfoo#/bar\n*/f();";
    QTest::newRow("case 4") << states << sequences << text;

    clear(&states, &sequences);
    states << 3;
    sequences << seqg;
    text = "i/*bla @/#foo#/ dummy ";
    QTest::newRow("case 5") << states << sequences << text;

    clear(&states, &sequences);
    states << 3 << 3;
    sequences << seqg << seqc;
    text = "i/*bla @/#foo#/ dummy \ni;";
    QTest::newRow("case 6") << states << sequences << text;

    clear(&states, &sequences);
    states << 3 << 3 << 0;
    sequences << seqg << seqc << seqc;
    text = "i/*bla @/#foo#/ dummy \ni;\n*/";
    QTest::newRow("case 7") << states << sequences << text;

    clear(&states, &sequences);
    states << 3 << 3 << 0;
    sequences << seqg << seqc << seqh;
    text = "i/*bla @/#foo#/ dummy \ni;\n*/a";
    QTest::newRow("case 8") << states << sequences << text;

    clear(&states, &sequences);
    states << 3;
    sequences << seqi;
    text = "/*+-";
    QTest::newRow("case 9") << states << sequences << text;

    clear(&states, &sequences);
    states << 0;
    sequences << seqj;
    text = "/*+=";
    QTest::newRow("case 10") << states << sequences << text;

    clear(&states, &sequences);
    states << 3;
    sequences << seqk;
    text = "/*+/#=";
    QTest::newRow("case 11") << states << sequences << text;

    clear(&states, &sequences);
    states << 6;
    sequences << seql;
    text = "/*+/#=-";
    QTest::newRow("case 12") << states << sequences << text;
}

void tst_HighlighterEngine::testEditingPersistentStates0()
{
    m_highlighterMock->startNoTestCalls();
    m_text.setPlainText("a b c /\ninside\n*/\na b c");
    m_highlighterMock->endNoTestCalls();

    QList<int> states;
    states << 3 << 3 << 0 << 0;
    QList<HighlightSequence> sequences;
    HighlightSequence seqa(0, 6);
    seqa.add(6, 8, Formats::instance().commentFormat());
    HighlightSequence seqb(0, 6, Formats::instance().commentFormat());
    HighlightSequence seqc(0, 2, Formats::instance().commentFormat());
    HighlightSequence seqd(0, 5);
    sequences << seqa << seqb << seqc << seqd;
    setExpectedData(states, sequences);

    addCharactersToEnd(m_text.document()->firstBlock(), "*");
}

void tst_HighlighterEngine::testEditingPersistentStates1()
{
    m_highlighterMock->startNoTestCalls();
    m_text.setPlainText("/*abc\n/\nnesting\nnesting\n#/\n*/xyz");
    m_highlighterMock->endNoTestCalls();

    QList<int> states;
    states << 4 << 4 << 4 << 3 << 0;
    QList<HighlightSequence> sequences;
    HighlightSequence seqa(0, 2, Formats::instance().errorFormat());
    HighlightSequence seqb(0, 7, Formats::instance().errorFormat());
    HighlightSequence seqc(seqb);
    HighlightSequence seqd(0, 2, Formats::instance().errorFormat());
    HighlightSequence seqe(0, 2, Formats::instance().commentFormat());
    seqe.add(2, 5);
    sequences << seqa << seqb << seqc << seqd << seqe;
    setExpectedData(states, sequences);

    addCharactersToEnd(m_text.document()->firstBlock().next(), "#");
}

void tst_HighlighterEngine::testEditingPersistentStates2()
{
    m_highlighterMock->startNoTestCalls();
    m_text.setPlainText("/*abc\n/\nnesting\nnesting\n*/\n#/xyz");
    m_highlighterMock->endNoTestCalls();

    QList<int> states;
    states << 4 << 4 << 4 << 4 << 3;
    QList<HighlightSequence> sequences;
    HighlightSequence seqa(0, 2, Formats::instance().errorFormat());
    HighlightSequence seqb(0, 7, Formats::instance().errorFormat());
    HighlightSequence seqc(seqb);
    HighlightSequence seqd(0, 2, Formats::instance().errorFormat());
    HighlightSequence seqe(seqd);
    seqe.add(2, 5, Formats::instance().commentFormat());
    sequences << seqa << seqb << seqc << seqd << seqe;
    setExpectedData(states, sequences);

    addCharactersToEnd(m_text.document()->firstBlock().next(), "#");
}

void tst_HighlighterEngine::testDynamicContexts()
{
    test();
}

void tst_HighlighterEngine::testDynamicContexts_data()
{
    createColumns();

    QList<int> states;
    QList<HighlightSequence> sequences;
    QString text;

    HighlightSequence seqa(0, 2);
    seqa.add(2, 15, Formats::instance().dataTypeFormat());
    seqa.add(15, 16, Formats::instance().othersFormat());
    seqa.add(16, 17, Formats::instance().regionMarketFormat());
    HighlightSequence seqb(seqa);
    seqb.add(17, 31, Formats::instance().errorFormat());

    states << 0;
    sequences << seqa;
    text = "a ---abcddeeff a ";
    QTest::newRow("case 0") << states << sequences << text;

    sequences.clear();
    sequences << seqb;
    text = "a ---abcddeeff a beginddeeffend";
    QTest::newRow("case 1") << states << sequences << text;
}

void tst_HighlighterEngine::testFirstNonSpace()
{
    test();
}

void tst_HighlighterEngine::testFirstNonSpace_data()
{
    createColumns();

    QList<int> states;
    QList<HighlightSequence> sequences;
    QString text;

    HighlightSequence seqa(0, 1, Formats::instance().errorFormat());
    HighlightSequence seqb(0, 3);
    seqb.add(3, 4, Formats::instance().errorFormat());
    HighlightSequence seqc(0, 1);
    seqc.add(1, 2, Formats::instance().errorFormat());
    HighlightSequence seqd(0, 2);

    states << 0;
    sequences << seqa;
    text = "|";
    QTest::newRow("case 0") << states << sequences << text;

    sequences.clear();
    sequences << seqb;
    text = "   |";
    QTest::newRow("case 1") << states << sequences << text;

    sequences.clear();
    sequences << seqc;
    text = "\t|";
    QTest::newRow("case 2") << states << sequences << text;

    sequences.clear();
    sequences << seqd;
    text = "a|";
    QTest::newRow("case 3") << states << sequences << text;
}

QTEST_MAIN(tst_HighlighterEngine)
#include "tst_highlighterengine.moc"

