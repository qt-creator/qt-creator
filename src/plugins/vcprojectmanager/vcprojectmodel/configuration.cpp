/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
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

namespace VcProjectManager {
namespace Internal {

using namespace ToolConstants;

Configuration::Configuration(const Configuration &config)
{
    m_name = config.m_name;
    m_anyAttribute = config.m_anyAttribute;
    m_nodeName = config.m_nodeName;
    qDeleteAll(m_configurationTools);
    m_configurationTools.clear();

    foreach (const ConfigurationTool *confTool, config.m_configurationTools)
        m_configurationTools.append(new ConfigurationTool(*confTool));
}

Configuration &Configuration::operator =(const Configuration &config)
{
    if (this != &config) {
        m_name = config.m_name;
        m_anyAttribute = config.m_anyAttribute;
        m_nodeName = config.m_nodeName;
        qDeleteAll(m_configurationTools);
        m_configurationTools.clear();

        foreach (const ConfigurationTool *confTool, m_configurationTools)
            m_configurationTools.append(new ConfigurationTool(*confTool));
    }
    return *this;
}

Configuration::~Configuration()
{
    qDeleteAll(m_configurationTools);
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
                m_name = domElement.value();
            }

            else
                m_anyAttribute.insert(domElement.name(), domElement.value());
        }
    }
}

QString Configuration::nodeWidgetName() const
{
    return m_name;
}

QDomNode Configuration::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement configurationNode = domXMLDocument.createElement(m_nodeName);

    configurationNode.setAttribute(QLatin1String("Name"), m_name);

    QHashIterator<QString, QString> it(m_anyAttribute);

    while (it.hasNext()) {
        it.next();
        configurationNode.setAttribute(it.key(), it.value());
    }

    foreach (const ConfigurationTool *confTool, m_configurationTools)
        configurationNode.appendChild(confTool->toXMLDomNode(domXMLDocument));

    return configurationNode;
}

QString Configuration::name() const
{
    return m_name;
}

void Configuration::setName(const QString &name)
{
    m_name = name;
    emit nameChanged();
}

QString Configuration::attributeValue(const QString &attributeName) const
{
    return m_anyAttribute.value(attributeName);
}

void Configuration::setAttribute(const QString &attributeName, const QString &attributeValue)
{
    m_anyAttribute.insert(attributeName, attributeValue);
}

void Configuration::clearAttribute(const QString &attributeName)
{
    if (m_anyAttribute.contains(attributeName))
        m_anyAttribute.insert(attributeName, QString());
}

void Configuration::removeAttribute(const QString &attributeName)
{
    if (m_anyAttribute.contains(attributeName))
        m_anyAttribute.remove(attributeName);
}

void Configuration::addConfigurationTool(ConfigurationTool *tool)
{
    if (!tool || m_configurationTools.contains(tool))
        return;

    foreach (ConfigurationTool *confTool, m_configurationTools) {
        if (confTool->toolDescription()->toolKey() == tool->toolDescription()->toolKey())
            return;
    }

    m_configurationTools.append(tool);
}

void Configuration::removeConfigurationTool(const QString &toolKey)
{
    QList<ConfigurationTool *>::iterator it = m_configurationTools.begin();

    while (it != m_configurationTools.end()) {
        ConfigurationTool *confTool = *it;
        if (confTool && confTool->toolDescription()->toolKey() == toolKey) {
            m_configurationTools.erase(it);
            delete confTool;
            return;
        }
        ++it;
    }
}

int Configuration::configurationToolCount() const
{
    return m_configurationTools.size();
}

ConfigurationTool *Configuration::configurationTool(int index) const
{
    if (0 <= index && index < m_configurationTools.size())
        return m_configurationTools[index];
    return 0;
}

ConfigurationTool *Configuration::configurationTool(const QString &toolKey) const
{
    foreach (ConfigurationTool *confTool, m_configurationTools) {
        if (confTool && confTool->toolDescription() && confTool->toolDescription()->toolKey() == toolKey)
            return confTool;
    }

    return 0;
}

Configuration::Configuration(const QString &nodeName)
    : m_nodeName(nodeName)
{
}

void Configuration::processToolNode(const QDomNode &toolNode)
{
    ConfigurationTool *toolConf = 0;
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
        m_configurationTools.append(toolConf);

        // process next sibling
        QDomNode nextSibling = toolNode.nextSibling();
        if (!nextSibling.isNull())
            processToolNode(nextSibling);
    }
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

Configuration::Ptr Configuration2003::clone() const
{
    return Configuration::Ptr(new Configuration2003(*this));
}

Configuration2003::Configuration2003(const QString &nodeName)
    : Configuration(nodeName)
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
        ConfigurationTool *toolConf = 0;
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
            m_configurationTools.append(toolConf);
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
    configurationNode.setAttribute(QLatin1String("Name"), m_name);
    QHashIterator<QString, QString> it(m_anyAttribute);

    while (it.hasNext()) {
        it.next();
        configurationNode.setAttribute(it.key(), it.value());
    }

    foreach (const ConfigurationTool *tool, m_configurationTools)
        configurationNode.appendChild(tool->toXMLDomNode(domXMLDocument));

    foreach (const DeploymentTool::Ptr &tool, m_deploymentTools)
        configurationNode.appendChild(tool->toXMLDomNode(domXMLDocument));

    return configurationNode;
}


VcNodeWidget *Configuration2005::createSettingsWidget()
{
    return new Configuration2005Widget(this);
}

Configuration::Ptr Configuration2005::clone() const
{
    return Configuration::Ptr(new Configuration2005(*this));
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

Configuration2005::Configuration2005(const QString &nodeName)
    : Configuration2003(nodeName)
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
        ConfigurationTool *toolConf = 0;
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
            m_configurationTools.append(toolConf);
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

    configurationNode.setAttribute(QLatin1String("Name"), m_name);

    QHashIterator<QString, QString> it(m_anyAttribute);

    while (it.hasNext()) {
        it.next();
        configurationNode.setAttribute(it.key(), it.value());
    }

    foreach (const ConfigurationTool *tool, m_configurationTools)
        configurationNode.appendChild(tool->toXMLDomNode(domXMLDocument));

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

Configuration::Ptr Configuration2008::clone() const
{
    return Configuration::Ptr(new Configuration2008(*this));
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

Configuration2008::Configuration2008(const QString &nodeName)
    : Configuration2005(nodeName)
{
}

} // namespace Internal
} // namespace VcProjectManager
