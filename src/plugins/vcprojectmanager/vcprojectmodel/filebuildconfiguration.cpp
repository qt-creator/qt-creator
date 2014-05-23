/**************************************************************************
**
** Copyright (c) 2014 Bojan Petrovic
** Copyright (c) 2014 Radovan Zivkovic
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
#include "filebuildconfiguration.h"
#include "tools/toolattributes/tooldescriptiondatamanager.h"
#include "../interfaces/iconfigurationbuildtool.h"
#include "../interfaces/iconfigurationbuildtools.h"
#include "../interfaces/idebuggertools.h"
#include "../interfaces/ideploymenttools.h"
#include "tools.h"
#include "tools/toolattributes/tooldescription.h"
#include "../widgets/fileconfigurationsettingswidget.h"

namespace VcProjectManager {
namespace Internal {

FileBuildConfiguration::FileBuildConfiguration(IVisualStudioProject *parentProject)
    : Configuration(QLatin1String("FileConfiguration")),
      m_parentProjectDoc(parentProject)
{
}

FileBuildConfiguration::FileBuildConfiguration(const FileBuildConfiguration &fileBuildConfig)
    : Configuration(fileBuildConfig)
{
    m_parentProjectDoc = fileBuildConfig.m_parentProjectDoc;
}

FileBuildConfiguration &FileBuildConfiguration::operator =(const FileBuildConfiguration &fileBuildConfig)
{
    Configuration::operator =(fileBuildConfig);

    if (this != &fileBuildConfig)
        m_parentProjectDoc = fileBuildConfig.m_parentProjectDoc;

    return *this;
}

VcNodeWidget *FileBuildConfiguration::createSettingsWidget()
{
    return new FileConfigurationSettingsWidget(this, m_parentProjectDoc);
}

QDomNode FileBuildConfiguration::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    if (tools()->configurationBuildTools()->toolCount()) {
        IConfigurations *configs = m_parentProjectDoc->configurations();

        QDomElement configurationNode = domXMLDocument.createElement(m_nodeName);
        m_attributeContainer->appendToXMLNode(configurationNode);

        if (configs) {
            ConfigurationContainer *configContainer = m_parentProjectDoc->configurations()->configurationContainer();

            if (configContainer) {
                IConfiguration *projectConfig  = configContainer->configuration(fullName());
                toXMLNode(projectConfig, configurationNode, domXMLDocument);
            }
        }

        if (configurationNode.childNodes().size() || configurationNode.attributes().size()) {
            configurationNode.setAttribute(QLatin1String("Name"), m_fullName);
            return configurationNode;
        }
    }

    return QDomNode();
}

void FileBuildConfiguration::processToolNode(const QDomNode &toolNode)
{
    if (toolNode.nodeName() == QLatin1String("Tool")) {
        IConfigurationBuildTool *toolConf = 0;
        QDomNamedNodeMap namedNodeMap = toolNode.toElement().attributes();

        for (int i = 0; i < namedNodeMap.size(); ++i) {
            QDomNode domNode = namedNodeMap.item(i);

            if (domNode.nodeType() == QDomNode::AttributeNode) {
                QDomAttr domAttribute = domNode.toAttr();
                if (domAttribute.name() == QLatin1String("Name")) {
                    ToolDescriptionDataManager *tDDM = ToolDescriptionDataManager::instance();
                    IToolDescription *toolDesc = tDDM->toolDescription(domAttribute.value());

                    if (toolDesc)
                        toolConf = toolDesc->createTool();
                    break;
                }
            }
        }

        if (toolConf) {
            toolConf->processNode(toolNode);
            m_tools->configurationBuildTools()->addTool(toolConf);
        }
    }

    else if (toolNode.nodeName() == QLatin1String("DeploymentTool")) {
        DeploymentTool* deplTool = new DeploymentTool;
        deplTool->processNode(toolNode);
        m_tools->deploymentTools()->addTool(deplTool);
    }

    else {
        DebuggerTool* deplTool = new DebuggerTool;
        deplTool->processNode(toolNode);
        m_tools->debuggerTools()->addTool(deplTool);
    }

    // process next sibling
    QDomNode nextSibling = toolNode.nextSibling();
    if (!nextSibling.isNull())
        processToolNode(nextSibling);
}

void FileBuildConfiguration::toXMLNode(IConfiguration *projectConfig, QDomElement &configurationNode, QDomDocument &domXMLDocument) const
{
    QTC_ASSERT(projectConfig, return);

    ITools *projectTools = projectConfig->tools();

    if (projectTools && projectTools->configurationBuildTools()) {
        IConfigurationBuildTool *tool = tools()->configurationBuildTools()->tool(0);

        if (tool) {
            IConfigurationBuildTool *projectTool = projectTools->configurationBuildTools()->tool(tool->toolDescription()->toolKey());

            if (projectTool && projectTool->toolDescription()) {
                toXMLNode(projectTool, tool, configurationNode, domXMLDocument);
            }
        }
    }
}

void FileBuildConfiguration::toXMLNode(IConfigurationBuildTool *projectConfigTool, IConfigurationBuildTool *tool,
                                       QDomElement &configurationNode, QDomDocument &domXMLDocument) const
{
    QTC_ASSERT(projectConfigTool && tool, return);

    ISectionContainer *projSecContainer = projectConfigTool->sectionContainer();
    ISectionContainer *toolSecContainer = tool->sectionContainer();

    bool isNodeCreated = false;
    QDomElement toolNode;

    if (projSecContainer && toolSecContainer) {
        for (int i = 0; i < projSecContainer->sectionCount(); ++i) {
            IToolSection *projToolSec = projSecContainer->section(i);

            if (projToolSec) {
                IToolSection *toolSec = toolSecContainer->section(projToolSec->sectionDescription()->displayName());

                if (toolSec) {
                    IToolAttributeContainer *projToolAttrContainer = projToolSec->attributeContainer();
                    IToolAttributeContainer *toolAttrContainer = toolSec->attributeContainer();

                    for (int j = 0; j < projToolAttrContainer->toolAttributeCount(); ++j) {
                        IToolAttribute *projToolAttr = projToolAttrContainer->toolAttribute(j);

                        if (projToolAttr && projToolAttr->descriptionDataItem()) {
                            IToolAttribute *toolAttr = toolAttrContainer->toolAttribute(projToolAttr->descriptionDataItem()->key());

                            if (toolAttr && toolAttr->value() != projToolAttr->value()) {
                                if (!isNodeCreated) {
                                    toolNode = domXMLDocument.createElement(QLatin1String("Tool"));
                                    toolNode.setAttribute(QLatin1String("Name"), projectConfigTool->toolDescription()->toolKey());
                                    configurationNode.appendChild(toolNode);
                                    isNodeCreated = true;
                                }

                                toolNode.setAttribute(toolAttr->descriptionDataItem()->key(),
                                                      toolAttr->value());
                            }
                        }
                    }
                }
            }
        }
    }
}

IConfiguration *FileBuildConfiguration::clone() const
{
    return new FileBuildConfiguration(*this);
}

} // namespace Internal
} // namespace VcProjectManager
