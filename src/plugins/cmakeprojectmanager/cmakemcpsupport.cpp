// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakemcpsupport.h"
#include "cmakebuildsystem.h"
#include "cmakeprojectmanager.h"

#include <mcp/server/toolregistry.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/result.h>

#include <QJsonArray>
#include <QJsonObject>

using namespace ProjectExplorer;

namespace CMakeProjectManager::Internal {

void setupCMakeMcpSupport()
{
    namespace Schema = Mcp::Generated::Schema::_2025_11_25;
    using Mcp::ToolRegistry;
    using Tool = Schema::Tool;
    using CallToolRequestParams = Schema::CallToolRequestParams;
    using CallToolResult = Schema::CallToolResult;

    using SimplifiedCallback = std::function<QJsonObject(const QJsonObject &)>;
    const auto wrap = [](const SimplifiedCallback &cb) {
        return [cb](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            return CallToolResult{}.structuredContent(cb(params.argumentsAsObject())).isError(false);
        };
    };

    ToolRegistry::registerTool(
        Tool{}
            .name("reconfigure")
            .title("Trigger a CMake reconfigure on a project")
            .description(
                "Re-runs CMake on a project (equivalent to Build → Run CMake). "
                "Use after adding a new build target or test to CMakeLists.txt so the "
                "next build/run_tests sees the refreshed target list. Fire-and-forget: "
                "returns when the reparse is queued, not when CMake finishes. The "
                "natural pattern is: reconfigure → build → run_tests. "
                "If 'project' is omitted, uses the current startup project. "
                "Returns triggered:false with a structured reason if the project is "
                "not found or does not use the CMake build system.")
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "project",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Name of the project to reconfigure. Uses the startup project if "
                             "omitted. Run 'list_projects' to see available project names."}})
                    .addProperty(
                        "mode",
                        QJsonObject{
                            {"type", "string"},
                            {"enum", QJsonArray{"normal", "profiling"}},
                            {"description",
                             "How to re-run CMake. Defaults to 'normal' if omitted. "
                             "normal: standard reconfigure. "
                             "profiling: reconfigure with profiling enabled; opens the "
                             "CTF Visualizer with the resulting profile when done."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "triggered",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description",
                             "True if the reconfigure was queued (async — CMake may not "
                             "have finished yet)."}})
                    .addProperty(
                        "project",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Display name of the project that was reconfigured."}})
                    .addProperty(
                        "reason",
                        QJsonObject{
                            {"type", "string"},
                            {"enum",
                             QJsonArray{
                                 "ok",
                                 "no_startup_project",
                                 "project_not_found",
                                 "no_cmake_project"}}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addRequired("triggered")
                    .addRequired("reason")
                    .addRequired("message")),
        wrap([](const QJsonObject &params) -> QJsonObject {
            const QString projectName = params.value("project").toString();

            Project *project = nullptr;
            if (projectName.isEmpty()) {
                project = ProjectManager::startupProject();
                if (!project)
                    return {
                        {"triggered", false},
                        {"reason", "no_startup_project"},
                        {"message",
                         "No startup project set. Open a CMake project or specify the "
                         "'project' parameter."}};
            } else {
                const QList<Project *> all = ProjectManager::projects();
                for (Project *p : all) {
                    if (p->displayName() == projectName) {
                        project = p;
                        break;
                    }
                }
                if (!project)
                    return {
                        {"triggered", false},
                        {"reason", "project_not_found"},
                        {"message",
                         QString("No open project named '%1'. Run 'list_projects' to see "
                                 "available names.")
                             .arg(projectName)}};
            }

            auto *cbs = qobject_cast<CMakeBuildSystem *>(project->activeBuildSystem());
            if (!cbs)
                return {
                    {"triggered", false},
                    {"reason", "no_cmake_project"},
                    {"message",
                     QString("Project '%1' does not use the CMake build system.")
                         .arg(project->displayName())}};

            const QString mode = params.value("mode").toString();
            if (mode == "profiling")
                runCMakeWithProfiling(cbs);
            else
                runCMake(cbs);
            return {
                {"triggered", true},
                {"reason", "ok"},
                {"project", project->displayName()},
                {"message",
                 QString("CMake reconfigure queued for '%1'. Runs asynchronously — "
                         "call build afterward to wait for the reparse to complete "
                         "before running tests.")
                     .arg(project->displayName())}};
        }));
}

} // namespace CMakeProjectManager::Internal
