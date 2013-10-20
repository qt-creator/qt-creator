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

FileBuildConfiguration::FileBuildConfiguration()
    : Configuration(QLatin1String("FileConfiguration"))
{
}

FileBuildConfiguration::FileBuildConfiguration(const FileBuildConfiguration &fileBuildConfig)
    : Configuration(fileBuildConfig)
{
}

FileBuildConfiguration &FileBuildConfiguration::operator =(const FileBuildConfiguration &fileBuildConfig)
{
    Configuration::operator =(fileBuildConfig);
    return *this;
}

VcNodeWidget *FileBuildConfiguration::createSettingsWidget()
{
    return new FileConfigurationSettingsWidget(this);
}

void FileBuildConfiguration::processToolNode(const QDomNode &toolNode)
{
    if (toolNode.nodeName() == QLatin1String("Tool")) {
        IConfigurationBuildTool *toolConf = 0;
        QDomNamedNodeMap namedNodeMap = toolNode.toElement().attributes();

        QDomNode domNode = namedNodeMap.item(0);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domAttribute = domNode.toAttr();
            if (domAttribute.name() == QLatin1String("Name")) {
                ToolDescriptionDataManager *tDDM = ToolDescriptionDataManager::instance();

                if (tDDM) {
                    IToolDescription *toolDesc = tDDM->toolDescription(domAttribute.value());

                    if (toolDesc)
                        toolConf = toolDesc->createTool();
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

} // namespace Internal
} // namespace VcProjectManager
