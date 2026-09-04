// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <mcp/server/mcpserver.h>

#include <utils/result.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

using namespace Mcp;

using Tool = Schema::Tool;
using InputSchema = Schema::Tool::InputSchema;
using Params = Schema::CallToolRequestParams;

static Utils::Result<> check(const InputSchema &schema, const QJsonObject &arguments)
{
    const Tool tool = Tool{}.name("probe").inputSchema(schema);
    return validateToolArguments(tool, Params{}.name(tool.name()).arguments(arguments));
}

class tst_ToolValidation : public QObject
{
    Q_OBJECT

private slots:
    void unknownArgument();
    void unknownBeforeRequired();
    void missingRequired();
    void wrongKind_data();
    void wrongKind();
    void article();
    void enumValues();
    void enumIsTypeExact();
    void enumNamesWhatArrived();
    void toolWithoutProperties();
    void undeclaredType();
};

static InputSchema typical()
{
    return InputSchema{}
        .addProperty("s", QJsonObject{{"type", "string"}})
        .addProperty("i", QJsonObject{{"type", "integer"}})
        .addProperty("n", QJsonObject{{"type", "number"}})
        .addProperty("b", QJsonObject{{"type", "boolean"}})
        .addProperty("arr", QJsonObject{{"type", "array"}})
        .addProperty("obj", QJsonObject{{"type", "object"}});
}

void tst_ToolValidation::unknownArgument()
{
    const Utils::Result<> r = check(typical(), {{"s", "hi"}, {"nope", 1}});
    QVERIFY(!r);
    QCOMPARE(r.error(),
             QString("Unknown argument \"nope\" for tool \"probe\". "
                     "Known: arr, b, i, n, obj, s"));

    QVERIFY(check(typical(), {{"s", "hi"}}));
}

void tst_ToolValidation::unknownBeforeRequired()
{
    // A misspelled name is the mistake. Reported as the required one it
    // failed to supply, the message would be true and silent about it.
    const Utils::Result<> r = check(typical().addRequired("s"), {{"S", "hi"}});
    QVERIFY(!r);
    QCOMPARE(r.error(),
             QString("Unknown argument \"S\" for tool \"probe\". "
                     "Known: arr, b, i, n, obj, s"));
}

void tst_ToolValidation::missingRequired()
{
    const InputSchema schema = typical().addRequired("s");
    const Utils::Result<> r = check(schema, {{"i", 1}});
    QVERIFY(!r);
    QCOMPARE(r.error(), QString("Missing required argument \"s\" for tool \"probe\""));

    QVERIFY(check(schema, {{"s", "hi"}}));
}

void tst_ToolValidation::wrongKind_data()
{
    QTest::addColumn<QString>("declared");
    QTest::addColumn<QString>("argument");
    QTest::addColumn<QJsonValue>("value");
    QTest::addColumn<QString>("error");

    // The message has to name what arrived, not just that it was wrong: these
    // four are four different mistakes.
    QTest::newRow("number for string")
        << "string" << "s" << QJsonValue(42)
        << "Argument \"s\" of tool \"probe\" wants a string, got a number";
    QTest::newRow("boolean for string")
        << "string" << "s" << QJsonValue(true)
        << "Argument \"s\" of tool \"probe\" wants a string, got a boolean";
    QTest::newRow("null for string")
        << "string" << "s" << QJsonValue()
        << "Argument \"s\" of tool \"probe\" wants a string, got null";
    QTest::newRow("array for string")
        << "string" << "s" << QJsonValue(QJsonArray{"a"})
        << "Argument \"s\" of tool \"probe\" wants a string, got an array";
    QTest::newRow("object for string")
        << "string" << "s" << QJsonValue(QJsonObject{})
        << "Argument \"s\" of tool \"probe\" wants a string, got an object";
    QTest::newRow("string for number")
        << "number" << "n" << QJsonValue("3.5")
        << "Argument \"n\" of tool \"probe\" wants a number, got a string";
    QTest::newRow("boolean for integer")
        << "integer" << "i" << QJsonValue(true)
        << "Argument \"i\" of tool \"probe\" wants an integer, got a boolean";
    QTest::newRow("fraction for integer")
        << "integer" << "i" << QJsonValue(3.5)
        << "Argument \"i\" of tool \"probe\" wants an integer, got a fractional number";
    QTest::newRow("string for array")
        << "array" << "arr" << QJsonValue("a")
        << "Argument \"arr\" of tool \"probe\" wants an array, got a string";
    QTest::newRow("string for object")
        << "object" << "obj" << QJsonValue("a")
        << "Argument \"obj\" of tool \"probe\" wants an object, got a string";
}

void tst_ToolValidation::wrongKind()
{
    QFETCH(QString, declared);
    QFETCH(QString, argument);
    QFETCH(QJsonValue, value);
    QFETCH(QString, error);

    const Utils::Result<> r = check(typical(), {{argument, value}});
    QVERIFY2(!r, "expected a rejection");
    QCOMPARE(r.error(), error);
}

void tst_ToolValidation::article()
{
    // "an" before integer, array and object; "a" before the rest.
    for (const QString &vowelled : {QString("integer"), QString("array"), QString("object")}) {
        const Utils::Result<> r = check(
            InputSchema{}.addProperty("v", QJsonObject{{"type", vowelled}}), {{"v", "x"}});
        QVERIFY(!r);
        QVERIFY2(r.error().contains("wants an " + vowelled), qPrintable(r.error()));
    }
    const Utils::Result<> r = check(
        InputSchema{}.addProperty("v", QJsonObject{{"type", "boolean"}}), {{"v", "x"}});
    QVERIFY(!r);
    QVERIFY2(r.error().contains("wants a boolean"), qPrintable(r.error()));
}

void tst_ToolValidation::enumValues()
{
    const InputSchema schema = InputSchema{}.addProperty(
        "mode", QJsonObject{{"type", "string"}, {"enum", QJsonArray{"append", "set"}}});

    QVERIFY(check(schema, {{"mode", "append"}}));
    QVERIFY(check(schema, {{"mode", "set"}}));

    const Utils::Result<> r = check(schema, {{"mode", "keys"}});
    QVERIFY(!r);
    QCOMPARE(r.error(),
             QString("Invalid value \"keys\" for argument \"mode\" of tool \"probe\". "
                     "Expected one of: \"append\", \"set\""));

    // Nothing declared for it means nothing to compare against.
    QVERIFY(check(schema, {}));
}

void tst_ToolValidation::enumIsTypeExact()
{
    // A stringifying comparison would let a number pass a string enum and a
    // string pass a boolean one.
    const InputSchema strings = InputSchema{}.addProperty(
        "v", QJsonObject{{"type", "string"}, {"enum", QJsonArray{"1", "2"}}});
    QVERIFY(check(strings, {{"v", "1"}}));
    QVERIFY(!check(strings, {{"v", 1}}));

    const InputSchema booleans = InputSchema{}.addProperty(
        "v", QJsonObject{{"type", "boolean"}, {"enum", QJsonArray{true}}});
    QVERIFY(check(booleans, {{"v", true}}));
    QVERIFY(!check(booleans, {{"v", "true"}}));

    const InputSchema numbers = InputSchema{}.addProperty(
        "v", QJsonObject{{"type", "integer"}, {"enum", QJsonArray{1, 2}}});
    QVERIFY(check(numbers, {{"v", 1}}));
    QVERIFY(!check(numbers, {{"v", "1"}}));
}

void tst_ToolValidation::enumNamesWhatArrived()
{
    // A top-level enum constrains the whole value, so an array does not match
    // one of its items - but the message still has to say what came in.
    const InputSchema schema = InputSchema{}.addProperty(
        "include", QJsonObject{{"type", "array"}, {"enum", QJsonArray{"log", "messages"}}});

    const Utils::Result<> r = check(schema, {{"include", QJsonArray{"log"}}});
    QVERIFY(!r);
    QVERIFY2(r.error().contains("Invalid value [\"log\"]"), qPrintable(r.error()));

    const Utils::Result<> nul = check(
        InputSchema{}.addProperty(
            "mode", QJsonObject{{"type", "string"}, {"enum", QJsonArray{"append"}}}),
        {{"mode", QJsonValue()}});
    QVERIFY(!nul);
    QVERIFY2(nul.error().contains("Invalid value null"), qPrintable(nul.error()));

    // A number is printed as it arrived, not rounded to six digits.
    const Utils::Result<> big = check(
        InputSchema{}.addProperty(
            "v", QJsonObject{{"type", "integer"}, {"enum", QJsonArray{1}}}),
        {{"v", 1234567}});
    QVERIFY(!big);
    QVERIFY2(big.error().contains("Invalid value 1234567 "), qPrintable(big.error()));
}

void tst_ToolValidation::toolWithoutProperties()
{
    // A tool that declares no properties takes no arguments, so anything sent
    // is undeclared. An empty schema and no schema at all mean the same thing.
    for (const InputSchema &schema : {InputSchema{}, Tool{}.inputSchema()}) {
        QVERIFY(check(schema, {}));
        const Utils::Result<> r = check(schema, {{"mode", "keys"}});
        QVERIFY2(!r, "an argument to a tool that declares none must be refused");
        QCOMPARE(r.error(),
                 QString("Tool \"probe\" takes no arguments, but \"mode\" was passed"));
    }

    // required is still honoured when no properties are declared.
    const Utils::Result<> missing = check(InputSchema{}.addRequired("must"), {});
    QVERIFY(!missing);
    QCOMPARE(missing.error(), QString("Missing required argument \"must\" for tool \"probe\""));
}

void tst_ToolValidation::undeclaredType()
{
    // A property with no "type" constrains nothing.
    const InputSchema schema = InputSchema{}.addProperty(
        "v", QJsonObject{{"description", "anything"}});
    QVERIFY(check(schema, {{"v", "x"}}));
    QVERIFY(check(schema, {{"v", 42}}));
    QVERIFY(check(schema, {{"v", QJsonArray{1}}}));
}

QTEST_GUILESS_MAIN(tst_ToolValidation)

#include "tst_toolvalidation.moc"
