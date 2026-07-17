// Copyright (C) 2026 Jeff Heller
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcptools.h"

#include "cmakebuildsystem.h"

#include <mcp/server/mcpserver.h>
#include <mcp/server/toolregistry.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/issuesmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/result.h>

#include <QElapsedTimer>
#include <QJsonObject>

using namespace Mcp;
using namespace ProjectExplorer;
using namespace Utils;

namespace Schema = Mcp::Schema;

namespace CMakeProjectManager::Internal {

void registerMcpTools()
{
    using Tool = Schema::Tool;
    using ToolAnnotations = Schema::ToolAnnotations;
    using CallToolResult = Schema::CallToolResult;
    using ToolExecution = Schema::ToolExecution;

    static IssuesManager issuesManager;

    const auto reconfigureOutputSchema =
        Tool::OutputSchema{}
            .addProperty(
                "succeeded",
                QJsonObject{
                    {"type", "boolean"},
                    {"description",
                     "Whether CMake finished without errors. Use this to decide "
                     "success/failure rather than inspecting the issues array."}})
            .addProperty(
                "error_count",
                QJsonObject{{"type", "integer"}, {"minimum", 0}})
            .addProperty(
                "warning_count",
                QJsonObject{{"type", "integer"}, {"minimum", 0}})
            .addProperty(
                "duration_ms",
                QJsonObject{
                    {"type", "integer"},
                    {"minimum", 0},
                    {"description", "Wall-clock duration in milliseconds."}})
            .addProperty(
                "issues",
                QJsonObject{
                    {"type", "array"},
                    {"description",
                     "CMake errors and warnings, same shape as list_issues' issues array."}})
            .addProperty("summary_text", QJsonObject{{"type", "string"}})
            .addRequired("succeeded")
            .addRequired("error_count")
            .addRequired("warning_count")
            .addRequired("duration_ms")
            .addRequired("issues")
            .addRequired("summary_text");

    ToolRegistry::registerTool(
        Tool{}
            .name("reconfigure_cmake")
            .title("Re-run CMake on the active project")
            .description(
                "Re-runs CMake on the active project (equivalent to Build → Run CMake) "
                "and blocks until CMake finishes. Returns a structured result with "
                "success verdict, error/warning counts, and the full issue list. "
                "Use after editing CMakeLists.txt to add a build target or test so "
                "the next build/run_tests sees the refreshed target list. "
                "The natural pattern is: reconfigure → build → run_tests.")
            .execution(ToolExecution().taskSupport(ToolExecution::TaskSupport::optional))
            .outputSchema(reconfigureOutputSchema)
            .annotations(ToolAnnotations{}.readOnlyHint(false)),
        [](const Schema::CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            Project *project = ProjectManager::startupProject();
            if (!project) {
                toolInterface.finish(
                    CallToolResult{}.isError(true).addContent(
                        Schema::TextContent{}.text(QStringLiteral("No active project."))));
                return ResultOk;
            }

            auto *bs = qobject_cast<CMakeBuildSystem *>(project->activeBuildSystem());
            if (!bs) {
                toolInterface.finish(
                    CallToolResult{}.isError(true).addContent(
                        Schema::TextContent{}.text(
                            QStringLiteral("Active project does not use CMake."))));
                return ResultOk;
            }

            struct State {
                bool finished = false;
                bool succeeded = false;
                QElapsedTimer timer;
            };
            auto state = std::make_shared<State>();
            state->timer.start();

            // Connect BEFORE reparse() to avoid losing a synchronously emitted
            // parsingFinished — e.g. when CMake has nothing to do and the parse
            // completes in one tick. SingleShotConnection captures exactly one
            // verdict per tool invocation.
            QObject::connect(
                bs,
                &BuildSystem::parsingFinished,
                bs,
                [state](bool success) {
                    state->succeeded = success;
                    state->finished = true;
                },
                Qt::SingleShotConnection);

            bs->reparse(CMakeBuildSystem::REPARSE_FORCE_CMAKE_RUN
                        | CMakeBuildSystem::REPARSE_URGENT);

            using namespace std::chrono_literals;

            toolInterface.startTask(
                1s,
                [state](Schema::Task task) -> Schema::Task {
                    if (state->finished) {
                        task.status(state->succeeded ? Schema::TaskStatus::completed
                                                     : Schema::TaskStatus::failed);
                        task.statusMessage(
                            state->succeeded
                                ? QStringLiteral("CMake reconfigure succeeded")
                                : QStringLiteral("CMake reconfigure failed"));
                        Mcp::letTaskDieIn(task, 1min);
                        return task;
                    }
                    return task.status(Schema::TaskStatus::working)
                        .statusMessage(QStringLiteral("CMake reconfiguring..."));
                },
                [state]() -> Utils::Result<CallToolResult> {
                    const QJsonObject issuesData = issuesManager.getCurrentIssues();
                    const QJsonObject summary = issuesData.value("summary").toObject();
                    const int errorCount = summary.value("errorCount").toInt();
                    const int warningCount = summary.value("warningCount").toInt();
                    const qint64 durationMs = state->timer.elapsed();

                    const QString summaryText =
                        state->succeeded
                            ? (warningCount == 0
                                   ? QString("CMake reconfigure succeeded in %1 ms")
                                         .arg(durationMs)
                                   : QString("CMake reconfigure succeeded with %1 warning(s) "
                                             "in %2 ms")
                                         .arg(warningCount)
                                         .arg(durationMs))
                            : QString("CMake reconfigure failed with %1 error(s) in %2 ms")
                                  .arg(errorCount)
                                  .arg(durationMs);

                    return CallToolResult{}
                        .structuredContent(QJsonObject{
                            {"succeeded", state->succeeded},
                            {"error_count", errorCount},
                            {"warning_count", warningCount},
                            {"duration_ms", durationMs},
                            {"issues", issuesData.value("issues")},
                            {"summary_text", summaryText},
                        })
                        .isError(!state->succeeded);
                },
                []() {}, // CMake reparse has no cancellation API
                Mcp::progressToken(params));

            return ResultOk;
        });
}

} // namespace CMakeProjectManager::Internal
