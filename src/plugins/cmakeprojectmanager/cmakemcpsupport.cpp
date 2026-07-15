// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakemcpsupport.h"
#include "cmakebuildsystem.h"
#include "cmakeproject.h"
#include "cmakeprojectmanager.h"
#include "presetsparser.h"

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

    using ToolAnnotations = Schema::ToolAnnotations;

    // Resolves a project (by name, or the startup project) as a CMakeProject.
    const auto resolveCMakeProject = [](const QString &projectName) -> CMakeProject * {
        Project *project = nullptr;
        if (projectName.isEmpty()) {
            project = ProjectManager::startupProject();
        } else {
            for (Project *p : ProjectManager::projects()) {
                if (p->displayName() == projectName) {
                    project = p;
                    break;
                }
            }
        }
        return qobject_cast<CMakeProject *>(project);
    };

    ToolRegistry::registerTool(
        Tool{}
            .name("list_cmake_presets")
            .title("List CMake presets of a project")
            .description(
                "Lists the presets defined in a project's CMakePresets.json - configure, "
                "build and test presets - each with its type, name, display name and "
                "hidden flag. If 'project' is omitted, uses the current startup project. "
                "Read-only.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}.addProperty(
                    "project",
                    QJsonObject{
                        {"type", "string"},
                        {"description", "Project name. Uses the startup project if omitted."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("havePresets", QJsonObject{{"type", "boolean"}})
                    .addProperty("presets", QJsonObject{{"type", "array"}})
                    .addProperty("reason", QJsonObject{{"type", "string"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})),
        wrap([resolveCMakeProject](const QJsonObject &params) -> QJsonObject {
            CMakeProject *project = resolveCMakeProject(params.value("project").toString());
            if (!project)
                return {{"reason", "no_cmake_project"},
                        {"message", "No CMake project (open one or pass 'project')."}};

            const PresetsData data = project->presetsData();
            QJsonArray presets;
            const auto add = [&presets](const QString &type, const auto &list) {
                for (const auto &p : list) {
                    presets.append(QJsonObject{
                        {"type", type},
                        {"name", p.name},
                        {"displayName", p.displayName.value_or(p.name)},
                        {"hidden", p.hidden}});
                }
            };
            add("configure", data.configurePresets);
            add("build", data.buildPresets);
            add("test", data.testPresets);
            return {{"havePresets", data.havePresets},
                    {"presets", presets},
                    {"reason", "ok"},
                    {"message", "ok"}};
        }));
}

} // namespace CMakeProjectManager::Internal
