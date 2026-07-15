// Copyright (C) 2026 Jeff Heller
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcptools.h"

#include "cmakebuildsystem.h"
#include "cmakeprojectmanager.h"

#include <mcp/server/mcpserver.h>
#include <mcp/server/toolregistry.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/issuesmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>

#include <utils/result.h>

#include <QElapsedTimer>
#include <QJsonArray>
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
            .addProperty("error_count", QJsonObject{{"type", "integer"}, {"minimum", 0}})
            .addProperty("warning_count", QJsonObject{{"type", "integer"}, {"minimum", 0}})
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
            .name("reconfigure")
            .title("Re-run CMake on a project")
            .description(
                "Re-runs CMake on a project (equivalent to Build → Run CMake) and "
                "blocks until CMake finishes. Returns a verdict: {succeeded, error_count, "
                "warning_count, duration_ms, issues, summary_text}. Use after editing "
                "CMakeLists.txt to add a target or test so the next build/run_tests sees "
                "the refreshed target list; the natural pattern is reconfigure → build "
                "→ run_tests. Uses the startup project if 'project' is omitted.")
            .execution(ToolExecution().taskSupport(ToolExecution::TaskSupport::optional))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "project",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Name of the project to reconfigure. Uses the startup project if "
                             "omitted. Run 'list_projects' to see available names."}})
                    .addProperty(
                        "mode",
                        QJsonObject{
                            {"type", "string"},
                            {"enum", QJsonArray{"normal", "profiling"}},
                            {"description",
                             "'normal' (default): standard reconfigure. 'profiling': "
                             "reconfigure with profiling enabled and open the CTF Visualizer "
                             "with the resulting profile."}}))
            .outputSchema(reconfigureOutputSchema)
            .annotations(ToolAnnotations{}.readOnlyHint(false)),
        [](const Schema::CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            const QJsonObject args = params.argumentsAsObject();
            const QString projectName = args.value("project").toString();

            // Structured error that still satisfies the verdict output schema.
            const auto fail = [&](const QString &reason, const QString &message) {
                toolInterface.finish(CallToolResult{}.isError(true).structuredContent(QJsonObject{
                    {"succeeded", false},
                    {"reason", reason},
                    {"error_count", 0},
                    {"warning_count", 0},
                    {"duration_ms", 0},
                    {"issues", QJsonArray{}},
                    {"summary_text", message}}));
            };

            Project *project = nullptr;
            if (projectName.isEmpty()) {
                project = ProjectManager::startupProject();
                if (!project) {
                    fail("no_startup_project",
                         "No startup project. Open a CMake project or pass 'project'.");
                    return ResultOk;
                }
            } else {
                for (Project *p : ProjectManager::projects()) {
                    if (p->displayName() == projectName) {
                        project = p;
                        break;
                    }
                }
                if (!project) {
                    fail("project_not_found",
                         QString("No open project named '%1'. Run 'list_projects' to see "
                                 "available names.")
                             .arg(projectName));
                    return ResultOk;
                }
            }

            BuildSystem *buildSystem = project->activeBuildSystem();
            if (!buildSystem) {
                fail("no_build_config",
                     QString("Project '%1' has no active build configuration.")
                         .arg(project->displayName()));
                return ResultOk;
            }
            auto *bs = qobject_cast<CMakeBuildSystem *>(buildSystem);
            if (!bs) {
                fail("no_cmake_project",
                     QString("Project '%1' does not use the CMake build system.")
                         .arg(project->displayName()));
                return ResultOk;
            }

            // The runCMake* helpers reparse only if saveModifiedFiles() succeeds;
            // save up front so a cancelled save fails fast instead of leaving the
            // task waiting for a parsingFinished that never comes.
            if (!ProjectExplorerPlugin::saveModifiedFiles()) {
                fail("save_cancelled",
                     "Reconfigure aborted: saving modified files was cancelled.");
                return ResultOk;
            }

            const bool profiling = args.value("mode").toString() == QLatin1String("profiling");

            struct State
            {
                bool finished = false;
                bool succeeded = false;
                QElapsedTimer timer;
            };
            auto state = std::make_shared<State>();
            state->timer.start();

            // Connect BEFORE the reparse to avoid losing a synchronously emitted
            // parsingFinished. SingleShotConnection captures exactly one verdict.
            QObject::connect(
                bs,
                &BuildSystem::parsingFinished,
                bs,
                [state](bool success) {
                    state->succeeded = success;
                    state->finished = true;
                },
                Qt::SingleShotConnection);

            if (profiling)
                runCMakeWithProfiling(bs);
            else
                runCMake(bs);

            using namespace std::chrono_literals;
            toolInterface.startTask(
                1s,
                [state](Schema::Task task) -> Schema::Task {
                    if (state->finished) {
                        task.status(state->succeeded ? Schema::TaskStatus::completed
                                                     : Schema::TaskStatus::failed);
                        task.statusMessage(
                            state->succeeded ? QStringLiteral("CMake reconfigure succeeded")
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

                    const QString summaryText
                        = state->succeeded
                              ? (warningCount == 0
                                     ? QString("CMake reconfigure succeeded in %1 ms").arg(durationMs)
                                     : QString(
                                           "CMake reconfigure succeeded with %1 warning(s) in "
                                           "%2 ms")
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
