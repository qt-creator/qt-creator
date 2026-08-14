// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpsupport.h"

#include "qmlprofilermodelmanager.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilertool.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>

#include <mcp/server/toolregistry.h>

#include <utils/result.h>

#include <QJsonObject>

using namespace ProjectExplorer;
using namespace Utils;

namespace Profiler::Internal {

static RunControl *runningProfilerRunControl()
{
    const QList<RunControl *> runControls = ProjectExplorerPlugin::allRunControls();
    for (RunControl *rc : runControls) {
        if (rc && rc->runMode() == Constants::QML_PROFILER_RUN_MODE && rc->isRunning())
            return rc;
    }
    return nullptr;
}

void registerMcpTools()
{
    using namespace Mcp::Schema;
    using Mcp::ToolRegistry;

    using SimplifiedCallback = std::function<QJsonObject(const QJsonObject &)>;
    static const auto wrap = [](const SimplifiedCallback &cb) {
        return [cb](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            return CallToolResult{}.structuredContent(cb(params.argumentsAsObject())).isError(false);
        };
    };

    ToolRegistry::registerTool(
        Tool{}
            .name("start_profiler")
            .title("Start the QML profiler")
            .description(
                "Starts the QML profiler on the current startup project (QML profiler run mode), "
                "using its active run configuration and kit. Does not build first - use the build "
                "tool beforehand if it may be out of date. Recording starts automatically; poll "
                "get_profiler_status for progress, and use stop_profiler (or let the application "
                "exit) to finalize the trace.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &) -> QJsonObject {
            const Utils::Result<> canRun = ProjectExplorerPlugin::canRunStartupProject(
                Constants::QML_PROFILER_RUN_MODE);
            if (!canRun)
                return {{"success", false}, {"error", canRun.error()}};
            QmlProfilerTool::instance()->profileStartupProject();
            return {{"success", true},
                    {"message", "QML profiler start requested for the startup project."}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("get_profiler_status")
            .title("Get QML profiler status")
            .description(
                "Returns the QML profiler state (Idle/AppRunning/AppStopRequested/AppDying), "
                "whether the server is recording, whether a profiler run is active, and a summary "
                "of the data collected so far: the event count and the trace duration in "
                "nanoseconds.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("state", QJsonObject{{"type", "string"}})
                    .addProperty("recording", QJsonObject{{"type", "boolean"}})
                    .addProperty("running", QJsonObject{{"type", "boolean"}})
                    .addProperty("num_events", QJsonObject{{"type", "integer"}})
                    .addProperty("trace_duration_ns", QJsonObject{{"type", "integer"}})
                    .addRequired("state")),
        wrap([](const QJsonObject &) -> QJsonObject {
            QmlProfilerTool *tool = QmlProfilerTool::instance();
            QmlProfilerModelManager *modelManager = tool->modelManager();
            QmlProfilerStateManager *stateManager = tool->stateManager();
            return {
                {"state", stateManager->currentStateAsString()},
                {"recording", stateManager->serverRecording()},
                {"running", runningProfilerRunControl() != nullptr},
                {"num_events", modelManager->numEvents()},
                {"trace_duration_ns", qint64(modelManager->traceDuration())},
            };
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("stop_profiler")
            .title("Stop the QML profiler")
            .description(
                "Stops the running QML profiler session by requesting its run control to stop, "
                "which finalizes the trace. Returns an error if no profiler session is running. "
                "Poll get_profiler_status afterwards for the finalized event count.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &) -> QJsonObject {
            RunControl *runControl = runningProfilerRunControl();
            if (!runControl)
                return {{"success", false}, {"error", "No QML profiler session is running."}};
            runControl->initiateStop();
            return {{"success", true}, {"message", "QML profiler stop requested."}};
        }));
}

} // namespace Profiler::Internal
