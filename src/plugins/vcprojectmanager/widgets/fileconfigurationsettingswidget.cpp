/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "fileconfigurationsettingswidget.h"
#include "ui_fileconfigurationsettingswidget.h"
#include "../vcprojectmodel/tools/toolattributes/tooldescriptiondatamanager.h"
#include "../interfaces/itooldescription.h"
#include "../interfaces/iconfigurationbuildtool.h"
#include "../interfaces/iconfigurationbuildtools.h"
#include "../interfaces/iconfiguration.h"
#include "../interfaces/itools.h"
#include "../interfaces/iattributecontainer.h"
#include "../vcprojectmodel/vcprojectdocument_constants.h"

namespace VcProjectManager {
namespace Internal {

FileConfigurationSettingsWidget::FileConfigurationSettingsWidget(IConfiguration *fileBuildConfig, QWidget *parent) :
    VcNodeWidget(parent),
    ui(new Ui::FileConfigurationSettingsWidget),
    m_fileBuildConfig(fileBuildConfig),
    m_configBuildTool(0)
{
    ui->setupUi(this);

    if (m_fileBuildConfig &&
            m_fileBuildConfig->tools() &&
            m_fileBuildConfig->tools()->configurationBuildTools()) {
        IConfigurationBuildTool *configBuildTool = m_fileBuildConfig->tools()->configurationBuildTools()->tool(0);

        if (configBuildTool)
            m_configBuildTool = configBuildTool->clone();

        if (m_configBuildTool) {
            QVBoxLayout *layout = new QVBoxLayout;
            m_toolSettingsWidget = m_configBuildTool->createSettingsWidget();
            layout->addWidget(m_toolSettingsWidget);
            ui->m_toolWidget->setLayout(layout);
        }
    }

    QString excludeFromBuild = m_fileBuildConfig->attributeContainer()->attributeValue(QLatin1String(VcDocConstants::VS_PROJECT_CONFIG_EXCLUDED));

    if (excludeFromBuild.isEmpty() || excludeFromBuild == QLatin1String("false"))
        ui->m_excludedFromBuild->setCurrentIndex(0);
    else
        ui->m_excludedFromBuild->setCurrentIndex(1);


    ToolDescriptionDataManager *tDDM = ToolDescriptionDataManager::instance();
    IToolDescription *toolDesc = tDDM->toolDescription(QLatin1String(VcDocConstants::TOOL_CPP_C_COMPILER));
    if (toolDesc)
        ui->m_tool->addItem(toolDesc->toolDisplayName(), QLatin1String(VcDocConstants::TOOL_CPP_C_COMPILER));

    toolDesc = tDDM->toolDescription(QLatin1String(VcDocConstants::TOOL_CUSTOM));
    if (toolDesc)
        ui->m_tool->addItem(toolDesc->toolDisplayName(), QLatin1String(VcDocConstants::TOOL_CUSTOM));

    toolDesc = tDDM->toolDescription(QLatin1String(VcDocConstants::TOOL_MANAGED_RESOURCE_COMPILER));
    if (toolDesc)
        ui->m_tool->addItem(toolDesc->toolDisplayName(), QLatin1String(VcDocConstants::TOOL_MANAGED_RESOURCE_COMPILER));

    toolDesc = tDDM->toolDescription(QLatin1String(VcDocConstants::TOOL_MIDL));
    if (toolDesc)
        ui->m_tool->addItem(toolDesc->toolDisplayName(), QLatin1String(VcDocConstants::TOOL_MIDL));

    toolDesc = tDDM->toolDescription(QLatin1String(VcDocConstants::TOOL_RESOURCE_COMPILER));
    if (toolDesc)
        ui->m_tool->addItem(toolDesc->toolDisplayName(), QLatin1String(VcDocConstants::TOOL_RESOURCE_COMPILER));

    toolDesc = tDDM->toolDescription(QLatin1String(VcDocConstants::TOOL_WEB_SERVICE_PROXY_GENERATOR));
    if (toolDesc)
        ui->m_tool->addItem(toolDesc->toolDisplayName(), QLatin1String(VcDocConstants::TOOL_WEB_SERVICE_PROXY_GENERATOR));

    toolDesc = tDDM->toolDescription(QLatin1String(VcDocConstants::TOOL_XML_DATA_PROXY_GENERATOR));
    if (toolDesc)
        ui->m_tool->addItem(toolDesc->toolDisplayName(), QLatin1String(VcDocConstants::TOOL_XML_DATA_PROXY_GENERATOR));

    int index = toolIndex(m_configBuildTool->toolDescription()->toolKey());
    if (index != -1)
        ui->m_tool->setCurrentIndex(index);

    connect(ui->m_tool, SIGNAL(currentIndexChanged(int)), this, SLOT(changeTool(int)));
}

FileConfigurationSettingsWidget::~FileConfigurationSettingsWidget()
{
    delete ui;
}

void FileConfigurationSettingsWidget::saveData()
{
    if (m_fileBuildConfig &&
            m_fileBuildConfig->tools() &&
            m_fileBuildConfig->tools()->configurationBuildTools() &&
            m_configBuildTool &&
            m_toolSettingsWidget) {
        // remove old tool
        IConfigurationBuildTool *confBuildTool = m_fileBuildConfig->tools()->configurationBuildTools()->tool(0);
        m_fileBuildConfig->tools()->configurationBuildTools()->removeTool(confBuildTool);

        // add new tool and save it's settings data
        m_fileBuildConfig->tools()->configurationBuildTools()->addTool(m_configBuildTool);
        m_toolSettingsWidget->saveData();
    }
}

void FileConfigurationSettingsWidget::changeTool(int index)
{
    QString toolKey = ui->m_tool->itemData(index).toString();
    ToolDescriptionDataManager *tDDM = ToolDescriptionDataManager::instance();
    IToolDescription *toolDesc = tDDM->toolDescription(toolKey);
    ui->m_toolWidget->setLayout(0);
    m_toolSettingsWidget = 0;
    IConfigurationBuildTool *oldConfigBuildTool = m_configBuildTool;
    m_configBuildTool = 0;

    if (toolDesc) {
        IConfigurationBuildTool *configBuildTool = toolDesc->createTool();

        if (configBuildTool) {
            QVBoxLayout *layout = new QVBoxLayout;
            m_toolSettingsWidget = configBuildTool->createSettingsWidget();
            layout->addWidget(m_toolSettingsWidget);
            m_configBuildTool = configBuildTool;
        }
    }

    delete oldConfigBuildTool;
}

int FileConfigurationSettingsWidget::toolIndex(const QString &toolKey)
{
    for (int i = 0; i < ui->m_tool->count(); ++i) {
        if (ui->m_tool->itemData(i).toString() == toolKey)
            return i;
    }

    return -1;
}

} // namespace Internal
} // namespace VcProjectManager
