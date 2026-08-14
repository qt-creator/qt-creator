// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "mcpsupport_test.h"

#include "localhelpmanager.h"

#include <mcp/server/toolregistry.h>

#include <QHelpFilterEngine>
#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

namespace Help::Internal {

using namespace Mcp::Schema;

static QJsonObject callTool(const QString &name, const QJsonObject &args = {})
{
    const Utils::Result<CallToolResult> result
        = Mcp::ToolRegistry::callToolForTests(name, CallToolRequestParams{}.arguments(args));
    if (!result) {
        QTest::qFail(qPrintable(result.error()), __FILE__, __LINE__);
        return {};
    }
    return result->structuredContentAsObject();
}

static Tool toolNamed(const QString &name)
{
    const QList<Tool> tools = Mcp::ToolRegistry::registeredTools();
    for (const Tool &tool : tools) {
        if (tool.name() == name)
            return tool;
    }
    return {};
}

class McpSupportTest final : public QObject
{
    Q_OBJECT

private slots:
    void testReadOnlyHints_data();
    void testReadOnlyHints();
    void testVersionFilterLifecycle();
};

// An absent readOnlyHint is not the statement readOnlyHint(false) is, so a
// client running in a confirm-before-writing mode has nothing to key off. Every
// tool on this surface has to say which side it is on.
void McpSupportTest::testReadOnlyHints_data()
{
    QTest::addColumn<QString>("toolName");
    QTest::addColumn<bool>("readOnly");

    QTest::newRow("get_registered_documentation") << "get_registered_documentation" << true;
    QTest::newRow("list_help_filters") << "list_help_filters" << true;
    QTest::newRow("get_help_contents") << "get_help_contents" << true;
    QTest::newRow("register_documentation") << "register_documentation" << false;
    QTest::newRow("set_help_filter") << "set_help_filter" << false;
    QTest::newRow("set_help_version_filter") << "set_help_version_filter" << false;
}

void McpSupportTest::testReadOnlyHints()
{
    QFETCH(QString, toolName);
    QFETCH(bool, readOnly);

    const Tool tool = toolNamed(toolName);
    QCOMPARE(tool.name(), toolName);
    const std::optional<ToolAnnotations> &annotations = tool.annotations();
    QVERIFY(annotations.has_value());
    QVERIFY(annotations->readOnlyHint().has_value());
    QCOMPARE(*annotations->readOnlyHint(), readOnly);
}

// set_help_version_filter names the filter it creates by a rule and writes it
// into the user's help collection. Walking versions must leave at most one such
// entry behind, and clearing must leave none.
void McpSupportTest::testVersionFilterLifecycle()
{
    QHelpFilterEngine *engine = LocalHelpManager::filterEngine();
    QVERIFY(engine);
    const QStringList before = engine->filters();
    QVERIFY(!before.contains("Version 6.8.0"));
    QVERIFY(!before.contains("Version 6.12.0"));

    QJsonObject result = callTool("set_help_version_filter", {{"version", "6.8.0"}});
    if (!result.value("ok").toBool()) {
        QSKIP("No writable help collection in this run, so no filter can be created.");
    }
    QCOMPARE(result.value("active").toString(), QString("Version 6.8.0"));
    QVERIFY(engine->filters().contains("Version 6.8.0"));

    // A second version replaces the first rather than adding to it.
    result = callTool("set_help_version_filter", {{"version", "6.12.0"}});
    QVERIFY(result.value("ok").toBool());
    QCOMPARE(result.value("active").toString(), QString("Version 6.12.0"));
    QVERIFY(engine->filters().contains("Version 6.12.0"));
    QVERIFY(!engine->filters().contains("Version 6.8.0"));

    // list_help_filters sees the same thing the engine does.
    const QJsonArray listed = callTool("list_help_filters").value("filters").toArray();
    QVERIFY(listed.contains(QJsonValue("Version 6.12.0")));

    // Clearing removes the entry, leaving the configuration as it was found.
    result = callTool("set_help_version_filter", {{"version", QString()}});
    QVERIFY(result.value("active").toString().isEmpty());
    QVERIFY(!engine->filters().contains("Version 6.12.0"));
    QStringList after = engine->filters();
    QStringList expected = before;
    after.sort();
    expected.sort();
    QCOMPARE(after, expected);
}

QObject *createMcpSupportTest()
{
    return new McpSupportTest;
}

} // namespace Help::Internal

#include "mcpsupport_test.moc"

#endif // WITH_TESTS
