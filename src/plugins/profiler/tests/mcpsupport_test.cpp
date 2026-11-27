// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpsupport_test.h"
#include "fakedebugserver.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>

#include <profiler/qmlprofilerclientmanager.h>
#include <profiler/qmlprofilerstatemanager.h>
#include <profiler/qmlprofilertool.h>
#include <profiler/qmlprofilertracebackend.h>

#include <mcp/server/toolregistry.h>

#include <utils/temporarydirectory.h>
#include <utils/url.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QScopeGuard>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

#include <optional>

using namespace Mcp::Schema;
using namespace ProjectExplorer;
using namespace QmlDebug;
using namespace Utils;
using namespace std::chrono_literals;

namespace Profiler::Internal {

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

// For tools that may answer after their callback has returned.
static QJsonObject callToolAsync(const QString &name, const QJsonObject &args = {})
{
    std::optional<Utils::Result<CallToolResult>> answer;
    Mcp::ToolRegistry::callToolForTests(name, CallToolRequestParams{}.arguments(args),
                                        [&answer](const Utils::Result<CallToolResult> &result) {
                                            answer = result;
                                        });
    if (!QTest::qWaitFor([&answer] { return answer.has_value(); })) {
        QTest::qFail("The tool did not answer.", __FILE__, __LINE__);
        return {};
    }
    if (!*answer) {
        QTest::qFail(qPrintable(answer->error()), __FILE__, __LINE__);
        return {};
    }
    return (*answer)->structuredContentAsObject();
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

void ProfilerMcpSupportTest::testReadOnlyHints_data()
{
    QTest::addColumn<QString>("toolName");
    QTest::addColumn<bool>("readOnly");

    QTest::newRow("profiler_qml_attach") << "profiler_qml_attach" << false;
    QTest::newRow("profiler_qml_save") << "profiler_qml_save" << false;
    QTest::newRow("profiler_qml_get_statistics") << "profiler_qml_get_statistics" << true;
}

void ProfilerMcpSupportTest::testReadOnlyHints()
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

// Bad options are refused before anything is started, so these leave no run
// behind.
void ProfilerMcpSupportTest::testRejectsInvalidRecordingOptions_data()
{
    QTest::addColumn<QString>("toolName");
    QTest::addColumn<QJsonObject>("args");
    QTest::addColumn<QString>("error");

    QTest::newRow("start, unknown feature")
        << "profiler_qml_start" << QJsonObject{{"features", QJsonArray{"binding", "bogus"}}}
        << "Unknown feature \"bogus\".";
    QTest::newRow("start, no features")
        << "profiler_qml_start" << QJsonObject{{"features", QJsonArray{}}}
        << "No features to record given.";
    QTest::newRow("attach, negative flush interval")
        << "profiler_qml_attach" << QJsonObject{{"port", 1234}, {"flush_interval_ms", -1}}
        << "flush_interval_ms must be a non-negative integer.";
    QTest::newRow("attach, unknown kit")
        << "profiler_qml_attach" << QJsonObject{{"port", 1234}, {"kit", "no such kit"}}
        << "There is no kit \"no such kit\".";
}

void ProfilerMcpSupportTest::testRejectsInvalidRecordingOptions()
{
    QFETCH(QString, toolName);
    QFETCH(QJsonObject, args);
    QFETCH(QString, error);

    const QJsonObject result = callTool(toolName, args);
    QCOMPARE(result.value("success").toBool(true), false);
    QCOMPARE(result.value("error").toString(), error);
}

void ProfilerMcpSupportTest::testAttachRecordSaveAndSummarize()
{
    Kit *const kit = KitManager::registerKit({}, "mcpkit");
    QVERIFY(kit);
    const auto deregisterKit = qScopeGuard([kit] { KitManager::deregisterKit(kit); });

    QmlProfilerTool *tool = QmlProfilerTool::instance();
    QVERIFY(tool);

    QPointer<QmlProfilerTraceBackend> backend;
    const QMetaObject::Connection backendConnection = connect(
        tool, &QmlProfilerTool::liveBackendChanged, this,
        [&backend](QmlProfilerTraceBackend *live) {
            backend = live;
            live->clientManager()->setRetryInterval(10);
            live->clientManager()->setMaximumRetries(10);
        });
    const auto disconnectFromTool = qScopeGuard([&] { disconnect(backendConnection); });

    const QUrl serverUrl = Utils::urlFromLocalHostAndFreePort();
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, quint16(serverUrl.port())));
    std::unique_ptr<QTcpSocket> connection;
    connect(&server, &QTcpServer::newConnection, this, [&] {
        connection.reset(server.nextPendingConnection());
        fakeDebugServer(connection.get());
    });

    const QJsonObject attached = callTool(
        "profiler_qml_attach",
        {{"port", serverUrl.port()},
         {"host", serverUrl.host()},
         {"kit", kit->displayName()},
         {"features", QJsonArray{"javascript", "binding"}},
         {"flush_interval_ms", 250}});
    QVERIFY2(attached.value("success").toBool(), qPrintable(attached.value("error").toString()));

    QTRY_VERIFY(backend);
    QTRY_VERIFY(connection);
    QTRY_VERIFY(backend->clientManager()->isConnected());
    QCOMPARE(backend->stateManager()->requestedFeatures(),
             (1ULL << ProfileJavaScript) | (1ULL << ProfileBinding));
    QCOMPARE(backend->clientManager()->flushInterval(), 250ms);

    const QJsonObject status = callTool("profiler_qml_get_status");
    QCOMPARE(status.value("requested_features").toArray(), (QJsonArray{"javascript", "binding"}));
    QCOMPARE(status.value("running").toBool(), true);

    QCOMPARE(callTool("profiler_qml_attach", {{"port", serverUrl.port()}}).value("error"),
             "A QML profiler session is already running.");
    QCOMPARE(callTool("profiler_qml_get_statistics").value("error"),
             "The QML profiler is still recording. Stop it first.");

    // The application going away ends the run.
    connection.reset();
    QTRY_COMPARE(backend->stateManager()->currentState(), QmlProfilerStateManager::Idle);

    // The summary is made in another thread, so the GUI thread is free meanwhile.
    std::optional<Utils::Result<CallToolResult>> early;
    Mcp::ToolRegistry::callToolForTests("profiler_qml_get_statistics", CallToolRequestParams{},
                                        [&early](const Utils::Result<CallToolResult> &result) {
                                            early = result;
                                        });
    QVERIFY(!early.has_value());
    QTRY_VERIFY(early.has_value());

    const QJsonObject statistics = callToolAsync("profiler_qml_get_statistics");
    QVERIFY2(!statistics.contains("error"), qPrintable(statistics.value("error").toString()));
    QVERIFY(statistics.value("events").isArray());
    QVERIFY(statistics.value("frames").isObject());

    const TemporaryDirectory directory("profiler-mcp-save");
    QCOMPARE(callTool("profiler_qml_save", {{"path", directory.filePath("trace.txt").path()}})
                 .value("error"),
             "path must end in .qtd or .qzt.");

    const FilePath target = directory.filePath("trace.qtd");
    const QJsonObject savedResult = callToolAsync("profiler_qml_save", {{"path", target.path()}});
    QVERIFY2(savedResult.value("success").toBool(),
             qPrintable(savedResult.value("error").toString()));
    QVERIFY(target.exists());
    QVERIFY(target.fileSize() > 0);
}

} // namespace Profiler::Internal
