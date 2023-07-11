// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "assetexporterplugin.h"

#include "assetexportpluginconstants.h"
#include "assetexportdialog.h"
#include "assetexporter.h"
#include "assetexporterview.h"
#include "filepathmodel.h"
#include "componentexporter.h"

#include "dumpers/itemnodedumper.h"
#include "dumpers/textnodedumper.h"
#include "dumpers/assetnodedumper.h"

#include "coreplugin/actionmanager/actionmanager.h"
#include "coreplugin/actionmanager/actioncontainer.h"
#include "coreplugin/documentmanager.h"
#include "qmldesigner/qmldesignerplugin.h"
#include "projectexplorer/projectexplorerconstants.h"
#include "projectexplorer/projectmanager.h"
#include "projectexplorer/project.h"
#include "projectexplorer/projectmanager.h"
#include "projectexplorer/taskhub.h"

#include "extensionsystem/pluginmanager.h"
#include "extensionsystem/pluginspec.h"

#include "utils/algorithm.h"

#include <QCoreApplication>
#include <QAction>

#include <QLoggingCategory>

namespace QmlDesigner {

AssetExporterPlugin::AssetExporterPlugin()
{
    ProjectExplorer::TaskHub::addCategory({Constants::TASK_CATEGORY_ASSET_EXPORT,
                                           tr("Asset Export"),
                                           tr("Issues with exporting assets."),
                                           false});

    auto *designerPlugin = QmlDesigner::QmlDesignerPlugin::instance();
    auto &viewManager = designerPlugin->viewManager();
    m_view = viewManager.registerView(std::make_unique<AssetExporterView>(
        designerPlugin->externalDependenciesForPluginInitializationOnly()));

    // Add dumper templates for factory instantiation.
    Component::addNodeDumper<ItemNodeDumper>();
    Component::addNodeDumper<TextNodeDumper>();
    Component::addNodeDumper<AssetNodeDumper>();

    // Instantiate actions created by the plugin.
    addActions();

    connect(ProjectExplorer::ProjectManager::instance(),
            &ProjectExplorer::ProjectManager::startupProjectChanged,
            this, &AssetExporterPlugin::updateActions);

    updateActions();
}

QString AssetExporterPlugin::pluginName() const
{
    return QLatin1String("AssetExporterPlugin");
}

void AssetExporterPlugin::onExport()
{
    auto startupProject = ProjectExplorer::ProjectManager::startupProject();
    if (!startupProject)
        return;

    FilePathModel model(startupProject);
    auto exportDir = startupProject->projectFilePath().parentDir();
    if (!exportDir.parentDir().isEmpty())
        exportDir = exportDir.parentDir();
    exportDir = exportDir.pathAppended(startupProject->displayName() + "_export");
    AssetExporter assetExporter(m_view, startupProject);
    AssetExportDialog assetExporterDialog(exportDir, assetExporter, model);
    assetExporterDialog.exec();
}

void AssetExporterPlugin::addActions()
{
    auto exportAction = new QAction(tr("Export Components"), this);
    exportAction->setToolTip(tr("Export components in the current project."));
    connect(exportAction, &QAction::triggered, this, &AssetExporterPlugin::onExport);
    Core::Command *cmd = Core::ActionManager::registerAction(exportAction, Constants::EXPORT_QML);

    // Add action to build menu
    Core::ActionContainer *buildMenu =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    buildMenu->addAction(cmd, ProjectExplorer::Constants::G_BUILD_RUN);
}

void AssetExporterPlugin::updateActions()
{
    auto project = ProjectExplorer::ProjectManager::startupProject();
    QAction* const exportAction = Core::ActionManager::command(Constants::EXPORT_QML)->action();
    exportAction->setEnabled(project && !project->needsConfiguration());
}

QString AssetExporterPlugin::metaInfo() const
{
    return QLatin1String(":/assetexporterplugin/assetexporterplugin.metainfo");
}

} //QmlDesigner
