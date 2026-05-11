// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baseqtversion.h"
#include "qtkitaspect.h"

#include <mcp/server/toolregistry.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/result.h>

#include <QJsonObject>

using namespace Utils;

namespace QtSupport::Internal {

void registerMcpTools()
{
    using namespace Mcp::Schema;
    using Mcp::ToolRegistry;

    ToolRegistry::registerTool(
        Tool{}
            .name("get_qt_directory")
            .title("Get Qt directory for the current project")
            .description(
                "Returns the Qt installation paths for the Qt version used by the active kit "
                "of the current project. Includes the installation prefix, version string, "
                "bin, header, and library paths.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}.addProperty(
                    "projectName",
                    QJsonObject{
                        {"type", "string"},
                        {"description",
                         "Name of the project to query. Defaults to the active startup "
                         "project."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "qtDirectory",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Qt installation prefix directory"}})
                    .addProperty(
                        "qtVersion",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Qt version string (e.g. \"6.8.0\")"}})
                    .addProperty(
                        "binPath",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Path to Qt target bin directory"}})
                    .addProperty(
                        "headerPath",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Path to Qt header directory"}})
                    .addProperty(
                        "libraryPath",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Path to Qt library directory"}})),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QString projectName = params.arguments()->value("projectName").toString();
            ProjectExplorer::Project *project = nullptr;
            if (projectName.isEmpty()) {
                project = ProjectExplorer::ProjectManager::startupProject();
            } else {
                const auto all = ProjectExplorer::ProjectManager::projects();
                const auto it = Utils::findOrDefault(all, Utils::equal(&ProjectExplorer::Project::displayName, projectName));
                project = it;
            }
            if (!project) {
                return CallToolResult{}
                    .isError(true)
                    .addContent(TextContent{}.text(
                        projectName.isEmpty()
                            ? QString("No active startup project")
                            : QString("No project named '%1' found").arg(projectName)));
            }
            const QtVersion *qt = QtKitAspect::qtVersion(project->activeKit());
            if (!qt) {
                return CallToolResult{}
                    .isError(true)
                    .addContent(TextContent{}.text("No Qt version found for the active project"));
            }
            return CallToolResult{}
                .isError(false)
                .structuredContent(QJsonObject{
                    {"qtDirectory", qt->prefix().toUserOutput()},
                    {"qtVersion", qt->qtVersionString()},
                    {"binPath", qt->binPath().toUserOutput()},
                    {"headerPath", qt->headerPath().toUserOutput()},
                    {"libraryPath", qt->libraryPath().toUserOutput()},
                });
        });
}

} // namespace QtSupport::Internal
