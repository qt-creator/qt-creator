/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "assetexporterplugin.h"

#include "assetexportpluginconstants.h"
#include "assetexportdialog.h"
#include "assetexporter.h"
#include "assetexporterview.h"
#include "filepathmodel.h"
#include "componentexporter.h"

#include "parsers/modelitemnodeparser.h"
#include "parsers/textnodeparser.h"
#include "parsers/assetnodeparser.h"

#include "coreplugin/actionmanager/actionmanager.h"
#include "coreplugin/actionmanager/actioncontainer.h"
#include "coreplugin/documentmanager.h"
#include "qmldesigner/qmldesignerplugin.h"
#include "projectexplorer/projectexplorerconstants.h"
#include "projectexplorer/session.h"
#include "projectexplorer/project.h"
#include "projectexplorer/session.h"
#include "projectexplorer/taskhub.h"

#include "extensionsystem/pluginmanager.h"
#include "extensionsystem/pluginspec.h"

#include "utils/algorithm.h"

#include <QCoreApplication>
#include <QAction>

#include <QLoggingCategory>

namespace QmlDesigner {

AssetExporterPlugin::AssetExporterPlugin() :
    m_view(new AssetExporterView)
{
    ProjectExplorer::TaskHub::addCategory( Constants::TASK_CATEGORY_ASSET_EXPORT,
                                           tr("Asset Export"), false);

    auto *designerPlugin = QmlDesigner::QmlDesignerPlugin::instance();
    auto &viewManager = designerPlugin->viewManager();
    viewManager.registerViewTakingOwnership(m_view);

    // Add parsers templates for factory instantiation.
    Component::addNodeParser<ItemNodeParser>();
    Component::addNodeParser<TextNodeParser>();
    Component::addNodeParser<AssetNodeParser>();

    // Instantiate actions created by the plugin.
    addActions();

    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::startupProjectChanged,
            this, &AssetExporterPlugin::updateActions);

    updateActions();
}

QString AssetExporterPlugin::pluginName() const
{
    return QLatin1String("AssetExporterPlugin");
}

void AssetExporterPlugin::onExport()
{
    auto startupProject = ProjectExplorer::SessionManager::startupProject();
    if (!startupProject)
        return;

    FilePathModel model(startupProject);
    QString exportDirName = startupProject->displayName() + "_export";
    auto exportDir = startupProject->projectFilePath().parentDir().pathAppended(exportDirName);
    AssetExporter assetExporter(m_view, startupProject);
    AssetExportDialog assetExporterDialog(exportDir, assetExporter, model);
    assetExporterDialog.exec();
}

void AssetExporterPlugin::addActions()
{
    auto exportAction = new QAction(tr("Export QML"));
    exportAction->setToolTip(tr("Export QML code of the current project."));
    connect(exportAction, &QAction::triggered, this, &AssetExporterPlugin::onExport);
    Core::Command *cmd = Core::ActionManager::registerAction(exportAction, Constants::EXPORT_QML);

    // Add action to build menu
    Core::ActionContainer *buildMenu =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    buildMenu->addAction(cmd, ProjectExplorer::Constants::G_BUILD_RUN);
}

void AssetExporterPlugin::updateActions()
{
    auto project = ProjectExplorer::SessionManager::startupProject();
    QAction* const exportAction = Core::ActionManager::command(Constants::EXPORT_QML)->action();
    exportAction->setEnabled(project && !project->needsConfiguration());
}

QString AssetExporterPlugin::metaInfo() const
{
    return QLatin1String(":/assetexporterplugin/assetexporterplugin.metainfo");
}

} //QmlDesigner
