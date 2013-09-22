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
#include "../widgets/configurationwidgets.h"
#include "tools/toolattributes/tooldescriptiondatamanager.h"
#include "tools/toolattributes/tooldescription.h"
#include "tools/configurationtool.h"
#include "generalattributecontainer.h"
#include "configurationtools.h"

namespace VcProjectManager {
namespace Internal {

using namespace ToolConstants;

Configuration::Configuration(const QString &nodeName)
    : m_nodeName(nodeName)
{
    m_attributeContainer = new GeneralAttributeContainer;
    m_tools = new ConfigurationTools;
}

Configuration::Configuration(const Configuration &config)
{
    m_fullName = config.m_fullName;
    m_nodeName = config.m_nodeName;
    m_attributeContainer = new GeneralAttributeContainer;
    *m_attributeContainer = *config.m_attributeContainer;

    m_tools = new ConfigurationTools;
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
    m_tools->appendToXMLNode(configurationNode, domXMLDocument);
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

void Configuration::processToolNode(const QDomNode &toolNode)
{
    ITool *toolConf = 0;
    QDomNamedNodeMap namedNodeMap = toolNode.toElement().attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domAttribute = domNode.toAttr();
            if (domAttribute.name() == QLatin1String("Name")) {
                ToolDescriptionDataManager *tDDM = ToolDescriptionDataManager::instance();
                ToolDescription *toolDesc = tDDM->toolDescription(domAttribute.value());
                toolConf = toolDesc->createTool();
                break;
            }
        }
    }

    if (toolConf) {
        toolConf->processNode(toolNode);
        m_tools->addTool(toolConf);

        // process next sibling
        QDomNode nextSibling = toolNode.nextSibling();
        if (!nextSibling.isNull())
            processToolNode(nextSibling);
    }
}


Configuration2003::Configuration2003(const QString &nodeName)
    : Configuration(nodeName)
{
}

Configuration2003::Configuration2003(const Configuration2003 &config)
    : Configuration(config)
{
}

Configuration2003::~Configuration2003()
{
}

VcNodeWidget *Configuration2003::createSettingsWidget()
{
    return new Configuration2003Widget(this);
}

IConfiguration *Configuration2003::clone() const
{
    return new Configuration2003(*this);
}


Configuration2005::Configuration2005(const QString &nodeName)
    : Configuration2003(nodeName)
{
}

Configuration2005::~Configuration2005()
{
    m_deploymentTools.clear();
}

Configuration2005::Configuration2005(const Configuration2005 &config)
    : Configuration2003(config)
{
    foreach (const DeploymentTool::Ptr &tool, config.m_deploymentTools)
        m_deploymentTools.append(DeploymentTool::Ptr(new DeploymentTool(*tool)));

}

Configuration2005 &Configuration2005::operator=(const Configuration2005 &config)
{
    if (this != &config) {
        Configuration2003::operator =(config);

        m_deploymentTools.clear();
        foreach (const DeploymentTool::Ptr &tool, config.m_deploymentTools)
            m_deploymentTools.append(DeploymentTool::Ptr(new DeploymentTool(*tool)));
    }
    return *this;
}

void Configuration2005::processToolNode(const QDomNode &toolNode)
{
    if (toolNode.nodeName() == QLatin1String("Tool")) {
        ITool *toolConf = 0;
        QDomNamedNodeMap namedNodeMap = toolNode.toElement().attributes();

        for (int i = 0; i < namedNodeMap.size(); ++i) {
            QDomNode domNode = namedNodeMap.item(i);

            if (domNode.nodeType() == QDomNode::AttributeNode) {
                QDomAttr domAttribute = domNode.toAttr();
                if (domAttribute.name() == QLatin1String("Name")) {
                    ToolDescriptionDataManager *tDDM = ToolDescriptionDataManager::instance();

                    if (tDDM) {
                        ToolDescription *toolDesc = tDDM->toolDescription(domAttribute.value());
                        toolConf = toolDesc->createTool();
                        break;
                    }
                }
            }
        }

        if (toolConf) {
            toolConf->processNode(toolNode);
            m_tools->addTool(toolConf);
        }
    } else {
        DeploymentTool::Ptr deplTool(new DeploymentTool);
        deplTool->processNode(toolNode);
        m_deploymentTools.append(deplTool);
    }

    // process next sibling
    QDomNode nextSibling = toolNode.nextSibling();
    if (!nextSibling.isNull())
        processToolNode(nextSibling);
}

QDomNode Configuration2005::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement configurationNode = domXMLDocument.createElement(m_nodeName);
    configurationNode.setAttribute(QLatin1String("Name"), m_fullName);

    m_attributeContainer->appendToXMLNode(configurationNode);
    m_tools->appendToXMLNode(configurationNode, domXMLDocument);

    foreach (const DeploymentTool::Ptr &tool, m_deploymentTools)
        configurationNode.appendChild(tool->toXMLDomNode(domXMLDocument));

    return configurationNode;
}


VcNodeWidget *Configuration2005::createSettingsWidget()
{
    return new Configuration2005Widget(this);
}

IConfiguration *Configuration2005::clone() const
{
    return new Configuration2005(*this);
}

void Configuration2005::addDeploymentTool(DeploymentTool::Ptr tool)
{
    if (m_deploymentTools.contains(tool))
        return;
    m_deploymentTools.append(tool);
}

void Configuration2005::removeDeploymentTool(DeploymentTool::Ptr tool)
{
    m_deploymentTools.removeAll(tool);
}

QList<DeploymentTool::Ptr> Configuration2005::deploymentTools() const
{
    return m_deploymentTools;
}

QList<DeploymentTool::Ptr> Configuration2005::deploymentTools(const QString &attributeName, const QString &attributeValue) const
{
    QList<DeploymentTool::Ptr> deploymentTools;

    foreach (const DeploymentTool::Ptr &tool, m_deploymentTools) {
        if (tool->attributeValue(attributeName) == attributeValue)
            deploymentTools.append(tool);
    }
    return deploymentTools;
}


Configuration2008::Configuration2008(const QString &nodeName)
    : Configuration2005(nodeName)
{
}

Configuration2008::Configuration2008(const Configuration2008 &config)
    : Configuration2005(config)
{
    foreach (const DebuggerTool::Ptr &tool, config.m_debuggerTools)
        m_debuggerTools.append(DebuggerTool::Ptr(new DebuggerTool(*tool)));
}

Configuration2008 &Configuration2008::operator=(const Configuration2008 &config)
{
    if (this != &config) {
        Configuration2005::operator =(config);
        m_debuggerTools.clear();

        foreach (const DebuggerTool::Ptr &tool, config.m_debuggerTools)
            m_debuggerTools.append(DebuggerTool::Ptr(new DebuggerTool(*tool)));
    }
    return *this;
}

Configuration2008::~Configuration2008()
{
}

void Configuration2008::processToolNode(const QDomNode &toolNode)
{
    if (toolNode.nodeName() == QLatin1String("Tool")) {
        ITool *toolConf = 0;
        QDomNamedNodeMap namedNodeMap = toolNode.toElement().attributes();

        for (int i = 0; i < namedNodeMap.size(); ++i) {
            QDomNode domNode = namedNodeMap.item(i);

            if (domNode.nodeType() == QDomNode::AttributeNode) {
                QDomAttr domAttribute = domNode.toAttr();
                if (domAttribute.name() == QLatin1String("Name")) {
                    ToolDescriptionDataManager *tDDM = ToolDescriptionDataManager::instance();

                    if (tDDM) {
                        ToolDescription *toolDesc = tDDM->toolDescription(domAttribute.value());

                        if (toolDesc) {
                            toolConf = toolDesc->createTool();
                            break;
                        }
                    }
                }
            }
        }

        if (toolConf) {
            toolConf->processNode(toolNode);
            m_tools->addTool(toolConf);
        }
    }

    else if (toolNode.nodeName() == QLatin1String("DeploymentTool")) {
        DeploymentTool::Ptr deplTool(new DeploymentTool);
        deplTool->processNode(toolNode);
        m_deploymentTools.append(deplTool);
    }

    else {
        DebuggerTool::Ptr deplTool(new DebuggerTool);
        deplTool->processNode(toolNode);
        m_debuggerTools.append(deplTool);
    }

    // process next sibling
    QDomNode nextSibling = toolNode.nextSibling();
    if (!nextSibling.isNull())
        processToolNode(nextSibling);
}

QDomNode Configuration2008::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement configurationNode = domXMLDocument.createElement(m_nodeName);

    configurationNode.setAttribute(QLatin1String("Name"), m_fullName);

    m_attributeContainer->appendToXMLNode(configurationNode);

    m_tools->appendToXMLNode(configurationNode, domXMLDocument);

    foreach (const DeploymentTool::Ptr &tool, m_deploymentTools)
        configurationNode.appendChild(tool->toXMLDomNode(domXMLDocument));

    foreach (const DebuggerTool::Ptr &tool, m_debuggerTools)
        configurationNode.appendChild(tool->toXMLDomNode(domXMLDocument));

    return configurationNode;
}

VcNodeWidget *Configuration2008::createSettingsWidget()
{
    return new Configuration2008Widget(this);
}

IConfiguration *Configuration2008::clone() const
{
    return new Configuration2008(*this);
}

void Configuration2008::addDebuggerTool(DebuggerTool::Ptr tool)
{
    if (m_debuggerTools.contains(tool))
        return;
    m_debuggerTools.append(tool);
}

void Configuration2008::removeDebuggerTool(DebuggerTool::Ptr tool)
{
    m_debuggerTools.removeAll(tool);
}

QList<DebuggerTool::Ptr> Configuration2008::debuggerTools() const
{
    return m_debuggerTools;
}

QList<DebuggerTool::Ptr> Configuration2008::debuggerTools(const QString &attributeName, const QString &attributeValue) const
{
    QList<DebuggerTool::Ptr> debuggerTools;

    foreach (const DebuggerTool::Ptr &tool, m_debuggerTools) {
        if (tool->attributeValue(attributeName) == attributeValue)
            debuggerTools.append(tool);
    }
    return debuggerTools;
}

} // namespace Internal
} // namespace VcProjectManager
