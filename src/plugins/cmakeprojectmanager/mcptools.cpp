// Copyright (C) 2026 Jeff Heller
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcptools.h"

#include "cmakebuildsystem.h"
#include "cmakeprojectmanager.h"

#include <mcp/server/mcpserver.h>
#include <mcp/server/toolregistry.h>

#include <projectexplorer/buildmanager.h>
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

// The CMake build system a tool call acts on, or the reason there is none.
struct CMakeTarget
{
    CMakeBuildSystem *buildSystem = nullptr;
    Project *project = nullptr;
    QString reason;
    QString message;
};

// Resolves the build system to act on: the one of the project named `projectName`, or of the
// startup project when that is empty.
static CMakeTarget resolveCMakeTarget(const QString &projectName)
{
    CMakeTarget target;
    if (projectName.isEmpty()) {
        target.project = ProjectManager::startupProject();
        if (!target.project) {
            target.reason = "no_startup_project";
            target.message = "No startup project. Open a CMake project or pass 'project'.";
            return target;
        }
    } else {
        target.project = Utils::findOrDefault(ProjectManager::projects(), [&](Project *p) {
            return p->displayName() == projectName;
        });
        if (!target.project) {
            target.reason = "project_not_found";
            target.message
                = QString("No open project named '%1'. Run 'list_projects' to see available names.")
                      .arg(projectName);
            return target;
        }
    }

    BuildSystem *buildSystem = target.project->activeBuildSystem();
    if (!buildSystem) {
        target.reason = "no_build_config";
        target.message = QString("Project '%1' has no active build configuration.")
                             .arg(target.project->displayName());
        return target;
    }
    target.buildSystem = qobject_cast<CMakeBuildSystem *>(buildSystem);
    if (!target.buildSystem) {
        target.reason = "no_cmake_project";
        target.message = QString("Project '%1' does not use the CMake build system.")
                             .arg(target.project->displayName());
    }
    return target;
}

// Runs CMake on `bs` and reports the verdict once the reparse finishes. `label` names the
// operation in the progress and summary messages; `elapsed` is already running, so work done
// before the reparse (clearing the configuration) counts towards the reported duration.
static void runCMakeAndReportVerdict(
    CMakeBuildSystem *bs,
    IssuesManager &issuesManager,
    const ToolInterface &toolInterface,
    const Schema::CallToolRequestParams &params,
    const QString &label,
    const std::shared_ptr<QElapsedTimer> &elapsed,
    bool profiling)
{
    using CallToolResult = Schema::CallToolResult;

    struct State
    {
        bool finished = false;
        bool succeeded = false;
        QString error;
    };
    auto state = std::make_shared<State>();

    // Connect BEFORE the reparse to avoid losing a synchronously emitted
    // parsingFinished. SingleShotConnection captures exactly one verdict.
    QObject::connect(
        bs,
        &BuildSystem::parsingFinished,
        bs,
        [state, bs](bool success) {
            // A CMake run that fails while the build directory still holds the reply of an
            // earlier successful one reports a SUCCESSFUL parse, of that stale data. The
            // build system's error, set just before this signal, is what tells them apart.
            state->error = bs->error();
            state->succeeded = success && state->error.isEmpty();
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
        [state, label](Schema::Task task) -> Schema::Task {
            if (state->finished) {
                task.status(
                    state->succeeded ? Schema::TaskStatus::completed : Schema::TaskStatus::failed);
                task.statusMessage(
                    label + (state->succeeded ? QString(" succeeded") : QString(" failed")));
                Mcp::letTaskDieIn(task, 1min);
                return task;
            }
            return task.status(Schema::TaskStatus::working).statusMessage(label + "...");
        },
        [state, label, elapsed, &issuesManager]() -> Utils::Result<CallToolResult> {
            const QJsonObject issuesData = issuesManager.getCurrentIssues();
            const QJsonObject summary = issuesData.value("summary").toObject();
            const int errorCount = summary.value("errorCount").toInt();
            const int warningCount = summary.value("warningCount").toInt();
            const qint64 durationMs = elapsed->elapsed();

            QString summaryText;
            if (state->succeeded) {
                summaryText = warningCount == 0
                                  ? QString("%1 succeeded in %2 ms").arg(label).arg(durationMs)
                                  : QString("%1 succeeded with %2 warning(s) in %3 ms")
                                        .arg(label)
                                        .arg(warningCount)
                                        .arg(durationMs);
            } else {
                summaryText = QString("%1 failed with %2 error(s) in %3 ms")
                                  .arg(label)
                                  .arg(errorCount)
                                  .arg(durationMs);
                if (!state->error.isEmpty())
                    summaryText += ": " + state->error;
            }

            QJsonObject verdict{
                {"succeeded", state->succeeded},
                {"error_count", errorCount},
                {"warning_count", warningCount},
                {"duration_ms", durationMs},
                {"issues", issuesData.value("issues")},
                {"summary_text", summaryText},
            };
            if (!state->succeeded && !state->error.isEmpty())
                verdict.insert("reason", "cmake_failed");

            return CallToolResult{}.structuredContent(verdict).isError(!state->succeeded);
        },
        []() {}, // CMake reparse has no cancellation API
        Mcp::progressToken(params));
}

void registerMcpTools()
{
    using Tool = Schema::Tool;
    using ToolAnnotations = Schema::ToolAnnotations;
    using CallToolResult = Schema::CallToolResult;
    using ToolExecution = Schema::ToolExecution;

    static IssuesManager issuesManager;

    const auto verdictOutputSchema =
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
                "reason",
                QJsonObject{
                    {"type", "string"},
                    {"description",
                     "Machine-readable failure cause, only present when succeeded is false, "
                     "e.g. \"cmake_failed\" or \"no_startup_project\"."}})
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
            .outputSchema(verdictOutputSchema)
            .annotations(ToolAnnotations{}.readOnlyHint(false)),
        [](const Schema::CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            const QJsonObject args = params.argumentsAsObject();

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

            const CMakeTarget target = resolveCMakeTarget(args.value("project").toString());
            if (!target.buildSystem) {
                fail(target.reason, target.message);
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

            auto elapsed = std::make_shared<QElapsedTimer>();
            elapsed->start();
            runCMakeAndReportVerdict(
                target.buildSystem,
                issuesManager,
                toolInterface,
                params,
                "CMake reconfigure",
                elapsed,
                args.value("mode").toString() == QLatin1String("profiling"));

            return ResultOk;
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("reset_cmake_configuration")
            .title("Reset a project's CMake configuration")
            .description(
                "Discards the CMake configuration of the project's ACTIVE build configuration "
                "(equivalent to Build > Clear CMake Configuration) and, unless reconfigure is "
                "false, configures it again from scratch, blocking until CMake finishes. Deletes "
                "CMakeCache.txt, CMakeFiles and the file-api reply directory in that build "
                "directory, so cache values set outside the configuration's initial CMake "
                "arguments are lost. Use this when a build directory is stale or broken - after "
                "a failed first configure, a plain reconfigure keeps re-running CMake without "
                "the initial arguments and cannot recover on its own. Switch configurations with "
                "switch_build_config to reset another one. Returns the same verdict as "
                "reconfigure.")
            .execution(ToolExecution().taskSupport(ToolExecution::TaskSupport::optional))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "project",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Name of the project to reset. Uses the startup project if "
                             "omitted. Run 'list_projects' to see available names."}})
                    .addProperty(
                        "reconfigure",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description",
                             "Run CMake again after clearing the configuration. Defaults to "
                             "true; pass false to leave the project unconfigured."}}))
            .outputSchema(verdictOutputSchema)
            .annotations(ToolAnnotations{}.readOnlyHint(false).destructiveHint(true)),
        [](const Schema::CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            const QJsonObject args = params.argumentsAsObject();
            const bool reconfigure = args.value("reconfigure").toBool(true);

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

            const CMakeTarget target = resolveCMakeTarget(args.value("project").toString());
            if (!target.buildSystem) {
                fail(target.reason, target.message);
                return ResultOk;
            }
            // Clearing the configuration under a running build would pull the build directory
            // out from under it. The Clear CMake Configuration action is hidden while building
            // for the same reason.
            if (BuildManager::isBuilding(target.project)) {
                fail("building",
                     QString("Project '%1' is building. Wait for the build to finish.")
                         .arg(target.project->displayName()));
                return ResultOk;
            }
            if (reconfigure && !ProjectExplorerPlugin::saveModifiedFiles()) {
                fail("save_cancelled", "Reset aborted: saving modified files was cancelled.");
                return ResultOk;
            }

            auto elapsed = std::make_shared<QElapsedTimer>();
            elapsed->start();
            target.buildSystem->clearCMakeCache();

            if (!reconfigure) {
                // Leaves the project unconfigured, so the build actions must go with it.
                target.buildSystem->disableCMakeBuildMenuActions();
                toolInterface.finish(CallToolResult{}.structuredContent(QJsonObject{
                    {"succeeded", true},
                    {"error_count", 0},
                    {"warning_count", 0},
                    {"duration_ms", elapsed->elapsed()},
                    {"issues", QJsonArray{}},
                    {"summary_text", "CMake configuration cleared"}}));
                return ResultOk;
            }

            runCMakeAndReportVerdict(
                target.buildSystem,
                issuesManager,
                toolInterface,
                params,
                "CMake configuration reset",
                elapsed,
                false);

            return ResultOk;
        });
}

} // namespace CMakeProjectManager::Internal
