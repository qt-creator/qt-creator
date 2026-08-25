// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpsupport.h"

#include "baseqtversion.h"
#include "qtkitaspect.h"
#include "qtversionfactory.h"
#include "qtversionmanager.h"

#include <mcp/server/toolregistry.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/kitaspect.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/result.h>

#include <QJsonArray>
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
                    "project_name",
                    QJsonObject{
                        {"type", "string"},
                        {"description",
                         "Name of the project to query. Defaults to the active startup "
                         "project."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "qt_directory",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Qt installation prefix directory"}})
                    .addProperty(
                        "qt_version",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Qt version string (e.g. \"6.8.0\")"}})
                    .addProperty(
                        "bin_path",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Path to Qt target bin directory"}})
                    .addProperty(
                        "header_path",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Path to Qt header directory"}})
                    .addProperty(
                        "library_path",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Path to Qt library directory"}})),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QString projectName = params.arguments()->value("project_name").toString();
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
                    {"qt_directory", qt->prefix().toUserOutput()},
                    {"qt_version", qt->qtVersionString()},
                    {"bin_path", qt->binPath().toUserOutput()},
                    {"header_path", qt->headerPath().toUserOutput()},
                    {"library_path", qt->libraryPath().toUserOutput()},
                });
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("add_qt_version")
            .title("Register a Qt version")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .description(
                "Registers the Qt version a qmake belongs to, as \"Add...\" on the Qt Versions "
                "preferences page does. Returns what the version turned out to be, so that a "
                "caller can tell which platform it was recognized as. A qmake that is already "
                "registered is returned as it stands.")
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "qmake_path",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Path to the qmake executable of the Qt to add"}})
                    .addProperty(
                        "name",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Display name for the version. Defaults to the name the "
                             "detection came up with."}})
                    .addRequired("qmake_path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "id",
                        QJsonObject{{"type", "integer"}, {"description", "Unique id"}})
                    .addProperty(
                        "name", QJsonObject{{"type", "string"}, {"description", "Display name"}})
                    .addProperty(
                        "qt_version",
                        QJsonObject{{"type", "string"}, {"description", "Qt version string"}})
                    .addProperty(
                        "type",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Qt version type, as the factories named it"}})
                    .addProperty(
                        "mkspec",
                        QJsonObject{{"type", "string"}, {"description", "Default mkspec"}})
                    .addProperty(
                        "abis",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "string"}}},
                            {"description", "ABIs the version was found to build for"}})
                    .addProperty(
                        "valid", QJsonObject{{"type", "boolean"}, {"description", "Is it usable"}})
                    .addProperty(
                        "already_registered",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Was this qmake known before the call"}})),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const FilePath qmake = FilePath::fromUserInput(
                params.arguments()->value("qmake_path").toString());
            if (qmake.isEmpty()) {
                return CallToolResult{}.isError(true).addContent(
                    TextContent{}.text("No qmake_path given"));
            }
            if (!qmake.isExecutableFile()) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("Not an executable file: %1").arg(qmake.toUserOutput())));
            }

            bool alreadyRegistered = true;
            QtVersion *version = QtVersionManager::version(
                [&qmake](const QtVersion *v) { return v->qmakeFilePath() == qmake; });
            if (!version) {
                QString error;
                version = QtVersionFactory::createQtVersionFromQMakePath(
                    qmake, ProjectExplorer::DetectionSource::Manual, &error);
                if (!version) {
                    return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                        error.isEmpty()
                            ? QString("No Qt version could be made of %1").arg(qmake.toUserOutput())
                            : error));
                }
                alreadyRegistered = false;
                const QString name = params.arguments()->value("name").toString();
                if (!name.isEmpty())
                    version->setUnexpandedDisplayName(name);
                QtVersionManager::addVersion(version);
            }

            QJsonArray abis;
            for (const ProjectExplorer::Abi &abi : version->qtAbis())
                abis.append(abi.toString());

            return CallToolResult{}.isError(false).structuredContent(QJsonObject{
                {"id", version->uniqueId()},
                {"name", version->displayName()},
                {"qt_version", version->qtVersionString()},
                {"type", version->type()},
                {"mkspec", version->mkspec()},
                {"abis", abis},
                {"valid", version->isValid()},
                {"already_registered", alreadyRegistered},
            });
        });
}

} // namespace QtSupport::Internal
