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
#include "configuration.h"

#include "debuggertool.h"
#include "deploymenttool.h"
#include "tools/tool_constants.h"
#include "../widgets/configurationbasewidget.h"
#include "tools/toolattributes/tooldescriptiondatamanager.h"
#include "tools/toolattributes/tooldescription.h"
#include "tools/configurationtool.h"
#include "generalattributecontainer.h"
#include "configurationbuildtools.h"
#include "tools.h"
#include "deploymenttools.h"
#include "debuggertools.h"

namespace VcProjectManager {
namespace Internal {

using namespace ToolConstants;

Configuration::Configuration(const QString &nodeName)
    : m_nodeName(nodeName),
      m_tools(new Tools),
      m_attributeContainer(new GeneralAttributeContainer)
{
}

Configuration::Configuration(const Configuration &config) : IConfiguration(config)
{
    m_fullName = config.m_fullName;
    m_nodeName = config.m_nodeName;
    m_attributeContainer = new GeneralAttributeContainer;
    *m_attributeContainer = *config.m_attributeContainer;

    m_tools = new Tools;
    *m_tools = *config.m_tools;
}

Configuration &Configuration::operator =(const Configuration &config)
{
    if (this != &config) {
        m_fullName = config.m_fullName;
        m_nodeName = config.m_nodeName;
        *m_attributeContainer = *config.m_attributeContainer;
        *m_tools = *config.m_tools;
    }
    return *this;
}

Configuration::~Configuration()
{
    delete m_attributeContainer;
    delete m_tools;
}

void Configuration::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.nodeType() == QDomNode::ElementNode)
        processNodeAttributes(node.toElement());

    if (node.hasChildNodes()) {
        QDomNode firstChild = node.firstChild();
        if (!firstChild.isNull())
            processToolNode(firstChild);
    }
}

void Configuration::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("Name")) {
                m_fullName = domElement.value();
            }

            else
                m_attributeContainer->setAttribute(domElement.name(), domElement.value());
        }
    }
}

QDomNode Configuration::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement configurationNode = domXMLDocument.createElement(m_nodeName);
    configurationNode.setAttribute(QLatin1String("Name"), m_fullName);
    m_attributeContainer->appendToXMLNode(configurationNode);
    m_tools->configurationBuildTools()->appendToXMLNode(configurationNode, domXMLDocument);
    m_tools->deploymentTools()->appendToXMLNode(configurationNode, domXMLDocument);
    m_tools->debuggerTools()->appendToXMLNode(configurationNode, domXMLDocument);

    return configurationNode;
}

IAttributeContainer *Configuration::attributeContainer() const
{
    return m_attributeContainer;
}

QString Configuration::fullName() const
{
    return m_fullName;
}

QString Configuration::name() const
{
    return m_configurationName;
}

QString Configuration::platform() const
{
    return m_platformName;
}

void Configuration::setFullName(const QString &fullName)
{
    m_fullName = fullName;
    emit nameChanged();
}

void Configuration::setName(const QString &name)
{
    m_configurationName = name;
}

void Configuration::setPlatform(const QString &platform)
{
    m_platformName = platform;
}

ITools *Configuration::tools() const
{
    return m_tools;
}

IConfiguration *Configuration::clone() const
{
    return new Configuration(*this);
}

VcNodeWidget *Configuration::createSettingsWidget()
{
    return new ConfigurationBaseWidget(this);
}

void Configuration::processToolNode(const QDomNode &toolNode)
{
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

        // process next sibling
        QDomNode nextSibling = toolNode.nextSibling();
        if (!nextSibling.isNull())
            processToolNode(nextSibling);
    }
}

} // namespace Internal
} // namespace VcProjectManager
