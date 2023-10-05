// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldesigner/components/connectioneditor/connectioneditorevaluator.h"
#include "qmljs/parser/qmljsast_p.h"
#include "qmljs/qmljsdocument.h"

#include <QApplication>
#include <QGraphicsObject>
#include <QScopedPointer>
#include <QtTest>

#include <algorithm>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlDesigner;

class tst_ConnectionEditor : public QObject
{
    Q_OBJECT
public:
    tst_ConnectionEditor();

private slots:
    void test01();
    void test01_data();
    void invalidSyntax();
    void displayStrings();
    void displayStrings_data();
    void parseAssignment();
    void parseSetProperty();
    void parseSetState();
    void parseConsoleLog();
    void parseCallFunction();
    void parseEmpty();
    void parseComplexCondition();
    QByteArray countOne();

private:
    static int m_cnt;
    ConnectionEditorEvaluator evaluator;
};

int tst_ConnectionEditor::m_cnt = 0;

tst_ConnectionEditor::tst_ConnectionEditor() {}

#define QCOMPARE_NOEXIT(actual, expected) \
    QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__)

void tst_ConnectionEditor::test01_data()
{
    QTest::addColumn<QString>("statement");
    QTest::addColumn<bool>("expectedValue");

    QTest::newRow("if then else property set")
        << "if (object.atr === 3 && kmt === 43) {kMtr.lptr = 3;} else {kMtr.ptr = 4;}" << true;

    QTest::newRow("empty") << "{}" << true;
    QTest::newRow("console.log") << "{console.log(\"test\")}" << true;
    QTest::newRow("function call") << "{someItem.functionCall()}" << true;
    QTest::newRow("property set number - extra .") << "{someItem.width.kkk = 10}" << true;
    QTest::newRow("property set color") << "{someItem.color = \"red\"}" << true;
    QTest::newRow("set state") << "{someItem.state = \"state\"}" << true;
    QTest::newRow("property set string") << "{someItem.text = \"some string\"}" << true;
    QTest::newRow("property set") << "{someItem.speed = 1.0}" << true;
    QTest::newRow("assignment") << "{someItem.speed = someOtherItem.speed}" << true;
    QTest::newRow("if assignemnt")
        << "{if (someItem.bool) { someItem.speed = someOtherItem.speed }}" << true;
    QTest::newRow("if function call") << "{if (someItem.bool) { someItem.functionCall() }}" << true;
    QTest::newRow("if console log") << "{if (someItem.bool) { console.log(\"ok\")  }}" << true;
    QTest::newRow("if else console log")
        << "{if (someItem.bool) { console.log(\"ok\")  } else {console.log(\"ko\")}}" << true;
    QTest::newRow("if else empty")
        << "{if (someItem.bool && someItem.someBool2) {  } else { } }" << true;
    QTest::newRow("if > number") << "{if (someItem.width > 10) { }}" << true;
    QTest::newRow("if > && bool ") << "{if (someItem.width > 10 && someItem.someBool) { }}" << true;
    QTest::newRow("if ||") << "{if (someItem.width > 10 || someItem.someBool) { }}" << true;
    QTest::newRow("if ===") << "{if (someItem.width === someItem.height) { }}" << true;
    QTest::newRow("if === 2") << "{if (someItem.width === someItem.height) {} }" << true;

    // False Conditions
    QTest::newRow("call with argument") << "{someItem.complexCall(blah)}" << false;
    QTest::newRow("+ in condition") << "{someItem.width = someItem.Height + 10}" << false;
    QTest::newRow("type not matching")
        << "if (someItem.bool) {console.log(\"ok\") } else { someItem.functionCall() }" << false;
    QTest::newRow("type not matching 2") << "{if (someItem.width == someItem.height) {} }" << false;
    QTest::newRow("if = in condition") << "{if (someItem.width = someItem.height) {} }" << false;
    QTest::newRow("if + in condition with >")
        << "{if (someItem.width > someItem.height + 10) {} }" << false;
    QTest::newRow("if {} missing") << "{if (someItem.width > someItem.height) ak = 2; }" << false;
    QTest::newRow("two statements")
        << "{if (someItem.bool) { console.log(\"ok\");console.log(\"ok\")  }}" << false;
    QTest::newRow("if with string")
        << "{if (someItem.string === \"test\") { console.log(\"ok\")  }}" << true;
    QTest::newRow("if just with true") << "{if (true) { console.log(\"ok\")  }}" << true;
}

void tst_ConnectionEditor::invalidSyntax()
{
    const QString statement1 = "!! This is no valid QML !!";
    const QString statement2
        = "{someItem.complexCall(blah)}"; //valid QML bit not valid for ConnectionEditor

    QString result = ConnectionEditorEvaluator::getDisplayStringForType(statement1);
    QCOMPARE(result, ConnectionEditorStatements::CUSTOM_DISPLAY_NAME);

    result = ConnectionEditorEvaluator::getDisplayStringForType(statement2);
    QCOMPARE(result, ConnectionEditorStatements::CUSTOM_DISPLAY_NAME);

    auto resultHandler = ConnectionEditorEvaluator::parseStatement(statement1);
    auto parsedStatement = ConnectionEditorStatements::okStatement(resultHandler);
    QVERIFY(std::holds_alternative<ConnectionEditorStatements::EmptyBlock>(parsedStatement));

    resultHandler = ConnectionEditorEvaluator::parseStatement(statement2);
    parsedStatement = ConnectionEditorStatements::okStatement(resultHandler);
    QVERIFY(std::holds_alternative<ConnectionEditorStatements::EmptyBlock>(parsedStatement));
}

void tst_ConnectionEditor::displayStrings()
{
    QFETCH(QString, statement);
    QFETCH(QString, expectedValue);

    const QString result = ConnectionEditorEvaluator::getDisplayStringForType(statement);

    QCOMPARE(result, expectedValue);
}

void tst_ConnectionEditor::displayStrings_data()
{
    QTest::addColumn<QString>("statement");
    QTest::addColumn<QString>("expectedValue");

    QTest::newRow("Empty") << "{}" << ConnectionEditorStatements::EMPTY_DISPLAY_NAME;

    QTest::newRow("Pure Console log")
        << "{console.log(\"test\")}" << ConnectionEditorStatements::LOG_DISPLAY_NAME;
    QTest::newRow("Condition Console log") << "{if (someItem.bool) { console.log(\"ok\")  }}"
                                           << ConnectionEditorStatements::LOG_DISPLAY_NAME;
    QTest::newRow("Pure Set Property")
        << "{someItem.speed = 1.0}" << ConnectionEditorStatements::SETPROPERTY_DISPLAY_NAME;
    QTest::newRow("Condition Set Property") << "{if (someItem.bool) {someItem.speed = 1.0}}"
                                            << ConnectionEditorStatements::SETPROPERTY_DISPLAY_NAME;

    QTest::newRow("Pure Function Call")
        << "{someItem.functionCall()}" << ConnectionEditorStatements::FUNCTION_DISPLAY_NAME;

    QTest::newRow("Condition Function Call") << "{if (someItem.bool) {someItem.functionCall()}}"
                                             << ConnectionEditorStatements::FUNCTION_DISPLAY_NAME;

    QTest::newRow("Pure Assignment") << "{someItem.speed = someOtherItem.speed}"
                                     << ConnectionEditorStatements::ASSIGNMENT_DISPLAY_NAME;

    QTest::newRow("Condition Assignment")
        << "{if (someItem.bool) {someItem.speed = someOtherItem.speed}}"
        << ConnectionEditorStatements::ASSIGNMENT_DISPLAY_NAME;

    QTest::newRow("Pure State") << "{someItem.state = \"state\"}"
                                << ConnectionEditorStatements::SETSTATE_DISPLAY_NAME;

    QTest::newRow("Condition State") << "{if (someItem.bool) {someItem.state = \"state\"}}"
                                     << ConnectionEditorStatements::SETSTATE_DISPLAY_NAME;

    QTest::newRow("Pure Set Property Color")
        << "{someItem.color = \"red\"}" << ConnectionEditorStatements::SETPROPERTY_DISPLAY_NAME;

    QTest::newRow("Custom function call assignment")
        << "{someItem.color = item.functionCall()}"
        << ConnectionEditorStatements::CUSTOM_DISPLAY_NAME;

    QTest::newRow("Custom function call with argument")
        << "{someItem.color = item.functionCall(\"test\")}"
        << ConnectionEditorStatements::CUSTOM_DISPLAY_NAME;

    QTest::newRow("Custom function call with argument 2")
        << "{item.functionCall(\"test\")}" << ConnectionEditorStatements::CUSTOM_DISPLAY_NAME;
}

void tst_ConnectionEditor::parseAssignment()
{
    auto result = ConnectionEditorEvaluator::parseStatement(
        "{someItem.speed = someOtherItem.speed2}");
    auto parsedStatement = ConnectionEditorStatements::okStatement(result);

    QVERIFY(std::holds_alternative<ConnectionEditorStatements::Assignment>(parsedStatement));
    auto assignment = std::get<ConnectionEditorStatements::Assignment>(parsedStatement);

    QCOMPARE(assignment.lhs.nodeId, "someItem");
    QCOMPARE(assignment.lhs.propertyName, "speed");

    QCOMPARE(assignment.rhs.nodeId, "someOtherItem");
    QCOMPARE(assignment.rhs.propertyName, "speed2");

    result = ConnectionEditorEvaluator::parseStatement(
        "{if (someItem.bool) {someItem.speed = someOtherItem.speed2}}");
    parsedStatement = ConnectionEditorStatements::okStatement(result);

    QVERIFY(std::holds_alternative<ConnectionEditorStatements::Assignment>(parsedStatement));
    assignment = std::get<ConnectionEditorStatements::Assignment>(parsedStatement);

    QCOMPARE(assignment.lhs.nodeId, "someItem");
    QCOMPARE(assignment.lhs.propertyName, "speed");

    QCOMPARE(assignment.rhs.nodeId, "someOtherItem");
    QCOMPARE(assignment.rhs.propertyName, "speed2");

    result = ConnectionEditorEvaluator::parseStatement(
        "{if (someItem.bool) {someItem1.speed = someOtherItem.speed2} else {someItem.speed = "
        "someOtherItem2.speed3}}");
    parsedStatement = ConnectionEditorStatements::koStatement(result);

    QVERIFY(std::holds_alternative<ConnectionEditorStatements::Assignment>(parsedStatement));
    assignment = std::get<ConnectionEditorStatements::Assignment>(parsedStatement);

    QCOMPARE(assignment.lhs.nodeId, "someItem");
    QCOMPARE(assignment.lhs.propertyName, "speed");

    QCOMPARE(assignment.rhs.nodeId, "someOtherItem2");
    QCOMPARE(assignment.rhs.propertyName, "speed3");
}

void tst_ConnectionEditor::parseSetProperty()
{
    auto result = ConnectionEditorEvaluator::parseStatement("{someItem.color = \"red\"}");
    auto parsedStatement = ConnectionEditorStatements::okStatement(result);

    QVERIFY(std::holds_alternative<ConnectionEditorStatements::PropertySet>(parsedStatement));
    auto propertySet = std::get<ConnectionEditorStatements::PropertySet>(parsedStatement);

    QCOMPARE(propertySet.lhs.nodeId, "someItem");
    QCOMPARE(propertySet.lhs.propertyName, "color");

    QVERIFY(std::holds_alternative<QString>(propertySet.rhs));

    auto string = std::get<QString>(propertySet.rhs);

    QCOMPARE(string, "red");

    result = ConnectionEditorEvaluator::parseStatement(
        "{if (someItem.bool){someItem.color = \"red\"}}");
    parsedStatement = ConnectionEditorStatements::okStatement(result);

    QVERIFY(std::holds_alternative<ConnectionEditorStatements::PropertySet>(parsedStatement));
    propertySet = std::get<ConnectionEditorStatements::PropertySet>(parsedStatement);

    QCOMPARE(propertySet.lhs.nodeId, "someItem");
    QCOMPARE(propertySet.lhs.propertyName, "color");

    QVERIFY(std::holds_alternative<QString>(propertySet.rhs));

    string = std::get<QString>(propertySet.rhs);

    QCOMPARE(string, "red");

    result = ConnectionEditorEvaluator::parseStatement(
        "{if (someItem.bool){someItem.color = \"red\"} else {someItem2.color = \"green\"}}");
    parsedStatement = ConnectionEditorStatements::koStatement(result);

    QVERIFY(std::holds_alternative<ConnectionEditorStatements::PropertySet>(parsedStatement));
    propertySet = std::get<ConnectionEditorStatements::PropertySet>(parsedStatement);

    QCOMPARE(propertySet.lhs.nodeId, "someItem2");
    QCOMPARE(propertySet.lhs.propertyName, "color");

    QVERIFY(std::holds_alternative<QString>(propertySet.rhs));

    string = std::get<QString>(propertySet.rhs);

    QCOMPARE(string, "green");
}

void tst_ConnectionEditor::parseSetState()
{
    auto result = ConnectionEditorEvaluator::parseStatement("{someItem.state = \"myState\"}");
    auto parsedStatement = ConnectionEditorStatements::okStatement(result);

    QVERIFY(std::holds_alternative<ConnectionEditorStatements::StateSet>(parsedStatement));
    auto stateSet = std::get<ConnectionEditorStatements::StateSet>(parsedStatement);

    QCOMPARE(stateSet.nodeId, "someItem");       // TODO should be just someItem
    QCOMPARE(stateSet.stateName, "\"myState\""); // TODO quotes should be removed.

    //skipping if/else case, since this test requires adjustments
}

void tst_ConnectionEditor::parseConsoleLog()
{
    auto result = ConnectionEditorEvaluator::parseStatement("{console.log(\"teststring\")}");
    auto parsedStatement = ConnectionEditorStatements::okStatement(result);

    QVERIFY(std::holds_alternative<ConnectionEditorStatements::ConsoleLog>(parsedStatement));
    auto consoleLog = std::get<ConnectionEditorStatements::ConsoleLog>(parsedStatement);

    QVERIFY(std::holds_alternative<QString>(consoleLog.argument));
    auto argument = std::get<QString>(consoleLog.argument); //TODO could be just a string
    QCOMPARE(argument, "teststring");
}

void tst_ConnectionEditor::parseCallFunction()
{
    auto result = ConnectionEditorEvaluator::parseStatement("{someItem.trigger()}");
    auto parsedStatement = ConnectionEditorStatements::okStatement(result);

    QVERIFY(std::holds_alternative<ConnectionEditorStatements::MatchedFunction>(parsedStatement));
    auto functionCall = std::get<ConnectionEditorStatements::MatchedFunction>(parsedStatement);

    QCOMPARE(functionCall.nodeId, "someItem");
    QCOMPARE(functionCall.functionName, "trigger");

    result = ConnectionEditorEvaluator::parseStatement("{if (someItem.bool){someItem.trigger()}}");
    parsedStatement = ConnectionEditorStatements::okStatement(result);

    QVERIFY(std::holds_alternative<ConnectionEditorStatements::MatchedFunction>(parsedStatement));
    functionCall = std::get<ConnectionEditorStatements::MatchedFunction>(parsedStatement);

    QCOMPARE(functionCall.nodeId, "someItem");
    QCOMPARE(functionCall.functionName, "trigger");

    result = ConnectionEditorEvaluator::parseStatement(
        "{if (someItem.bool){someItem.trigger()} else {someItem2.trigger2()}}");
    parsedStatement = ConnectionEditorStatements::koStatement(result);

    QVERIFY(std::holds_alternative<ConnectionEditorStatements::MatchedFunction>(parsedStatement));
    functionCall = std::get<ConnectionEditorStatements::MatchedFunction>(parsedStatement);

    QCOMPARE(functionCall.nodeId, "someItem2");
    QCOMPARE(functionCall.functionName, "trigger2");
}

void tst_ConnectionEditor::parseEmpty()
{
    auto result = ConnectionEditorEvaluator::parseStatement("{}");
    auto parsedStatement = ConnectionEditorStatements::okStatement(result);

    QVERIFY(std::holds_alternative<ConnectionEditorStatements::EmptyBlock>(parsedStatement));
}

void tst_ConnectionEditor::parseComplexCondition()
{
    auto result = ConnectionEditorEvaluator::parseStatement(
        "{if (someItem.deeper.gothis && thistest.is.getit){someItem.trigger()}}");
    auto &matchedCondition = ConnectionEditorStatements::matchedCondition(result);
    QCOMPARE(matchedCondition.statements.count(), 2);

    auto firstStatement = matchedCondition.statements.first();
    QVERIFY(std::holds_alternative<ConnectionEditorStatements::Variable>(firstStatement));

    auto variable = std::get<ConnectionEditorStatements::Variable>(firstStatement);

    QCOMPARE(variable.nodeId, "someItem");
    QCOMPARE(variable.propertyName, "deeper.gothis");

    auto lastStatement = matchedCondition.statements.last();
    QVERIFY(std::holds_alternative<ConnectionEditorStatements::Variable>(lastStatement));

    variable = std::get<ConnectionEditorStatements::Variable>(lastStatement);

    QCOMPARE(variable.nodeId, "thistest");
    QCOMPARE(variable.propertyName, "is.getit");

    matchedCondition.statements.clear();
    matchedCondition.tokens.clear();

    variable.nodeId = "justId";
    variable.propertyName = {};

    matchedCondition.statements.append(variable);

    const QString source = ConnectionEditorStatements::toJavascript(result);

    QVERIFY(source.startsWith("if (justId)"));
}

void tst_ConnectionEditor::test01()
{
    QFETCH(QString, statement);
    QFETCH(bool, expectedValue);

    QmlJS::Document::MutablePtr newDoc = QmlJS::Document::create(Utils::FilePath::fromString(
                                                                     "<expression>"),
                                                                 QmlJS::Dialect::JavaScript);

    newDoc->setSource(statement);
    newDoc->parseJavaScript();
    newDoc->ast()->accept(&evaluator);

    bool parseCheck = newDoc->isParsedCorrectly();
    bool valid = evaluator.status() == ConnectionEditorEvaluator::Succeeded;
    QVERIFY(parseCheck);
    //QVERIFY(valid);

    bool result = parseCheck && valid;

    QCOMPARE(result, expectedValue);
}

QByteArray tst_ConnectionEditor::countOne()
{
    return QByteArray::number(m_cnt++);
}

QTEST_GUILESS_MAIN(tst_ConnectionEditor);

#include "tst_connectioneditor.moc"
