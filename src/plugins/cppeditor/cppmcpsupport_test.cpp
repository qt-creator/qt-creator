// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppmcpsupport_test.h"

#include "cpptoolstestcase.h"

#include <mcp/server/toolregistry.h>

#include <utils/filepath.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

namespace CppEditor::Internal {

// Invoke a registered MCP tool by name and return its structured content. On
// failure *error carries the message, so a test can tell a registry-level
// refusal apart from an error the tool itself reported.
static QJsonObject callTool(const QString &name, const QJsonObject &args, QString *error = nullptr)
{
    if (error)
        error->clear();
    const Utils::Result<Mcp::Schema::CallToolResult> result = Mcp::ToolRegistry::callToolForTests(
        name, Mcp::Schema::CallToolRequestParams{}.arguments(args));
    if (!result) {
        if (error)
            *error = result.error();
        return {};
    }
    if (error && result->isError().value_or(false)) {
        for (const Mcp::Schema::ContentBlock &block : result->content()) {
            if (const auto *text = std::get_if<Mcp::Schema::TextContent>(&block))
                *error += text->text();
        }
        if (error->isEmpty())
            *error = QString("Tool \"%1\" reported an error.").arg(name);
    }
    return result->structuredContentAsObject();
}

// An asynchronous tool of our own, so that testing the registry's refusal to
// call one does not depend on which other plugins happen to be loaded.
static QString asyncToolName()
{
    static const QString name = [] {
        const QString toolName = "cppeditor_test_async_tool";
        Mcp::ToolRegistry::registerTool(
            Mcp::Schema::Tool{}.name(toolName).title("Asynchronous tool, for tests"),
            [](const Mcp::Schema::CallToolRequestParams &,
               const Mcp::ToolInterface &) -> Utils::Result<> { return Utils::ResultOk; });
        return toolName;
    }();
    return name;
}

// Find the first object in a JSON array whose "name" equals the given name.
static QJsonObject objectNamed(const QJsonArray &array, const QString &name)
{
    for (const QJsonValue &value : array) {
        if (value.toObject().value("name").toString() == name)
            return value.toObject();
    }
    return {};
}

static bool writeAndParse(CppEditor::Tests::TemporaryDir &dir, const QByteArray &source,
                          Utils::FilePath *file)
{
    if (!dir.isValid())
        return false;
    CppEditor::Internal::Tests::CppTestDocument document("f.cpp", source);
    document.setBaseDirectory(dir.path());
    if (!document.writeToDisk())
        return false;
    if (!CppEditor::Tests::TestCase::parseFiles({document.filePath()}))
        return false;
    *file = document.filePath();
    return true;
}

void CppMcpSupportTest::testGetFileSymbols()
{
    CppEditor::Tests::TestCase testCase;
    QVERIFY(testCase.succeededSoFar());
    CppEditor::Tests::TemporaryDir dir;
    Utils::FilePath file;
    QVERIFY(writeAndParse(dir,
                          "enum Color { Red, Green };\n"
                          "int add(int a, int b) { return a + b; }\n",
                          &file));

    const QJsonArray symbols = callTool("get_file_symbols",
                                        {{"file", file.toFSPathString()}}).value("symbols").toArray();

    const QJsonObject add = objectNamed(symbols, "add");
    QCOMPARE(add.value("kind").toString(), QString("function"));
    QCOMPARE(add.value("line").toInt(), 2);
    QCOMPARE(add.value("column").toInt(), 5); // 1-based
    QCOMPARE(objectNamed(symbols, "Color").value("kind").toString(), QString("enum"));
}

void CppMcpSupportTest::testGetSymbolInfo()
{
    CppEditor::Tests::TestCase testCase;
    QVERIFY(testCase.succeededSoFar());
    CppEditor::Tests::TemporaryDir dir;
    Utils::FilePath file;
    QVERIFY(writeAndParse(dir, "int add(int a, int b) { return a + b; }\n", &file));

    const QJsonObject info = callTool("get_symbol_info",
                                      {{"file", file.toFSPathString()}, {"line", 1}, {"column", 5}});
    QCOMPARE(info.value("name").toString(), QString("add"));
    QCOMPARE(info.value("kind").toString(), QString("function"));
    QCOMPARE(info.value("definition").toObject().value("line").toInt(), 1);
}

void CppMcpSupportTest::testFindReferences()
{
    CppEditor::Tests::TestCase testCase;
    QVERIFY(testCase.succeededSoFar());
    CppEditor::Tests::TemporaryDir dir;
    Utils::FilePath file;
    QVERIFY(writeAndParse(dir,
                          "static int g(int x) { return x; }\n"
                          "int h() { return g(1) + g(2); }\n",
                          &file));

    // g at line 1, column 12; one declaration + two calls.
    const QJsonArray refs = callTool("find_references",
                                     {{"file", file.toFSPathString()}, {"line", 1}, {"column", 12}})
                                .value("references").toArray();
    QCOMPARE(refs.size(), 3);
}

void CppMcpSupportTest::testGetTypeHierarchy()
{
    CppEditor::Tests::TestCase testCase;
    QVERIFY(testCase.succeededSoFar());
    CppEditor::Tests::TemporaryDir dir;
    Utils::FilePath file;
    QVERIFY(writeAndParse(dir,
                          "struct Base {};\n"
                          "struct Derived : Base {};\n",
                          &file));

    // Base at line 1, column 8.
    const QJsonObject hierarchy = callTool("get_type_hierarchy",
                                           {{"file", file.toFSPathString()}, {"line", 1}, {"column", 8}});
    QCOMPARE(hierarchy.value("name").toString(), QString("Base"));
    QCOMPARE(objectNamed(hierarchy.value("derived").toArray(), "Derived").value("name").toString(),
             QString("Derived"));
}

void CppMcpSupportTest::testFindOverrides()
{
    CppEditor::Tests::TestCase testCase;
    QVERIFY(testCase.succeededSoFar());
    CppEditor::Tests::TemporaryDir dir;
    Utils::FilePath file;
    QVERIFY(writeAndParse(dir,
                          "struct Base { virtual void f(); };\n"
                          "struct Derived : Base { void f() override; };\n",
                          &file));

    // Base::f at line 1, column 28.
    const QJsonObject result = callTool("find_overrides",
                                        {{"file", file.toFSPathString()}, {"line", 1}, {"column", 28}});
    QVERIFY(result.value("is_virtual").toBool());
    QStringList names;
    for (const QJsonValue &value : result.value("overrides").toArray())
        names << value.toObject().value("name").toString();
    QVERIFY(names.contains("Base::f"));
    QVERIFY(names.contains("Derived::f"));
}

void CppMcpSupportTest::testRenameSymbolDryRun()
{
    CppEditor::Tests::TestCase testCase;
    QVERIFY(testCase.succeededSoFar());
    CppEditor::Tests::TemporaryDir dir;
    Utils::FilePath file;
    const QByteArray source = "static int g(int x) { return x; }\n"
                              "int h() { return g(1) + g(2); }\n";
    QVERIFY(writeAndParse(dir, source, &file));

    // g at line 1, column 12; the declaration and both calls get rewritten.
    QString error;
    const QJsonObject result = callTool("rename_symbol",
                                        {{"file", file.toFSPathString()},
                                         {"line", 1},
                                         {"column", 12},
                                         {"new_name", "gg"}},
                                        &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(result.value("applied").toBool(), false);
    QCOMPARE(result.value("symbol").toString(), QString("g"));
    QCOMPARE(result.value("total_edits").toInt(), 3);

    const QJsonArray edits = result.value("edits").toArray();
    QCOMPARE(edits.size(), 3);
    QCOMPARE(edits.at(0).toObject().value("line").toInt(), 1);
    QCOMPARE(edits.at(0).toObject().value("column").toInt(), 12);
    QCOMPARE(edits.at(0).toObject().value("old_text").toString(), QString("g"));
    QCOMPARE(edits.at(0).toObject().value("new_text").toString(), QString("gg"));

    // A dry run must not have touched the file.
    QCOMPARE(file.fileContents().value_or(QByteArray()), source);
}

void CppMcpSupportTest::testErrorHandling()
{
    QString error;
    callTool("get_file_symbols", {}, &error); // missing required "file"
    QVERIFY(!error.isEmpty());

    // An asynchronous tool must be refused as such, not merely be absent.
    callTool(asyncToolName(), {}, &error);
    QVERIFY2(error.contains("asynchronous"), qPrintable(error));

    callTool("no_such_tool", {}, &error);
    QVERIFY2(error.contains("No registered tool"), qPrintable(error));

    // A valid identifier is required for a rename.
    callTool("rename_symbol",
             {{"file", "/x.cpp"}, {"line", 1}, {"column", 1}, {"new_name", "1bad"}},
             &error);
    QVERIFY2(error.contains("not a valid C++ identifier"), qPrintable(error));
}

} // namespace CppEditor::Internal
