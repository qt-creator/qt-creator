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
#include "configurationtype.h"

#include "tools/prebuildeventtool.h"
#include "tools/toolfactory.h"

namespace VcProjectManager {
namespace Internal {

ConfigurationType::~ConfigurationType()
{
    m_tools.clear();
}

void ConfigurationType::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("Name")) {
                m_name = domElement.value();
                m_oldName = m_name;
            }

            else
                m_anyAttribute.insert(domElement.name(), domElement.value());
        }
    }
}

void ConfigurationType::processToolNode(const QDomNode &toolNode)
{
    Tool::Ptr tool;
    QDomNamedNodeMap namedNodeMap = toolNode.toElement().attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domAttribute = domNode.toAttr();
            if (domAttribute.name() == QLatin1String("Name")) {
                tool = ToolFactory::createTool(domAttribute.value());
                break;
            }
        }
    }

    if (tool) {
        tool->processNode(toolNode);
        m_tools.append(tool);

        // process next sibling
        QDomNode nextSibling = toolNode.nextSibling();
        if (!nextSibling.isNull())
            processToolNode(nextSibling);
    }
}

QDomNode ConfigurationType::toXMLDomNode(QDomDocument &domXMLDocument, const QString &nodeName) const
{
    QDomElement configurationNode = domXMLDocument.createElement(nodeName);

    configurationNode.setAttribute(QLatin1String("Name"), m_name);

    QHashIterator<QString, QString> it(m_anyAttribute);

    while (it.hasNext()) {
        it.next();
        configurationNode.setAttribute(it.key(), it.value());
    }

    foreach (const Tool::Ptr &tool, m_tools)
        configurationNode.appendChild(tool->toXMLDomNode(domXMLDocument));

    return configurationNode;
}

void ConfigurationType::addTool(Tool::Ptr tool)
{
    if (m_tools.contains(tool))
        return;
    m_tools.append(tool);
}

void ConfigurationType::removeTool(Tool::Ptr tool)
{
    if (m_tools.contains(tool))
        m_tools.removeAll(tool);
}

Tool::Ptr ConfigurationType::tool(const QString &toolName) const
{
    foreach (const Tool::Ptr &tool, m_tools) {
        if (tool && tool->name() == toolName)
            return tool;
    }
    return Tool::Ptr();
}

QList<Tool::Ptr> ConfigurationType::tools() const
{
    return m_tools;
}

QString ConfigurationType::name() const
{
    return m_name;
}

void ConfigurationType::setName(const QString &name)
{
    m_name = name;
}

QString ConfigurationType::oldName() const
{
    return m_oldName;
}

void ConfigurationType::setOldName(const QString &oldName)
{
    m_oldName = oldName;
}

QString ConfigurationType::attributeValue(const QString &attributeName) const
{
    return m_anyAttribute.value(attributeName);
}

void ConfigurationType::setAttribute(const QString &attributeName, const QString &attributeValue)
{
    m_anyAttribute.insert(attributeName, attributeValue);
}

void ConfigurationType::clearAttribute(const QString &attributeName)
{
    if (m_anyAttribute.contains(attributeName))
        m_anyAttribute.insert(attributeName, QString());
}

void ConfigurationType::removeAttribute(const QString &attributeName)
{
    if (m_anyAttribute.contains(attributeName))
        m_anyAttribute.remove(attributeName);
}

ConfigurationType::ConfigurationType()
{
}

ConfigurationType::ConfigurationType(const ConfigurationType &configType)
{
    m_name = configType.m_name;
    m_anyAttribute = configType.m_anyAttribute;

    foreach (const Tool::Ptr &tool, configType.m_tools)
        m_tools.append(tool->clone());
}

ConfigurationType &ConfigurationType::operator =(const ConfigurationType &configType)
{
    if (this != &configType) {
        m_name = configType.m_name;
        m_anyAttribute = configType.m_anyAttribute;
        m_tools.clear();

        foreach (const Tool::Ptr &tool, configType.m_tools)
            m_tools.append(tool->clone());
    }
    return *this;
}


ConfigurationType2003::~ConfigurationType2003()
{
}

ConfigurationType::Ptr ConfigurationType2003::clone() const
{
    return ConfigurationType::Ptr(new ConfigurationType2003(*this));
}

ConfigurationType2003::ConfigurationType2003()
{
}

ConfigurationType2003::ConfigurationType2003(const ConfigurationType2003 &configType)
    : ConfigurationType(configType)
{
}

ConfigurationType2003 &ConfigurationType2003::operator=(const ConfigurationType2003 &configType)
{
    if (this != &configType)
        ConfigurationType::operator =(configType);
    return *this;
}


ConfigurationType2005::~ConfigurationType2005()
{
    m_deploymentTools.clear();
}

void ConfigurationType2005::processToolNode(const QDomNode &toolNode)
{
    if (toolNode.nodeName() == QLatin1String("Tool")) {
        Tool::Ptr tool;
        QDomNamedNodeMap namedNodeMap = toolNode.toElement().attributes();

        for (int i = 0; i < namedNodeMap.size(); ++i) {
            QDomNode domNode = namedNodeMap.item(i);

            if (domNode.nodeType() == QDomNode::AttributeNode) {
                QDomAttr domAttribute = domNode.toAttr();
                if (domAttribute.name() == QLatin1String("Name")) {
                    tool = ToolFactory::createTool(domAttribute.value());
                    break;
                }
            }
        }

        if (tool) {
            tool->processNode(toolNode);
            m_tools.append(tool);
        }
    }

    else
    {
        DeploymentTool::Ptr deplTool(new DeploymentTool);
        deplTool->processNode(toolNode);
        m_deploymentTools.append(deplTool);
    }

    // process next sibling
    QDomNode nextSibling = toolNode.nextSibling();
    if (!nextSibling.isNull())
        processToolNode(nextSibling);
}

QDomNode ConfigurationType2005::toXMLDomNode(QDomDocument &domXMLDocument, const QString &nodeName) const
{
    QDomElement configurationNode = domXMLDocument.createElement(nodeName);
    configurationNode.setAttribute(QLatin1String("Name"), m_name);
    QHashIterator<QString, QString> it(m_anyAttribute);

    while (it.hasNext()) {
        it.next();
        configurationNode.setAttribute(it.key(), it.value());
    }

    foreach (const Tool::Ptr &tool, m_tools)
        configurationNode.appendChild(tool->toXMLDomNode(domXMLDocument));

    foreach (const DeploymentTool::Ptr &tool, m_deploymentTools)
        configurationNode.appendChild(tool->toXMLDomNode(domXMLDocument));

    return configurationNode;
}

ConfigurationType::Ptr ConfigurationType2005::clone() const
{
    return ConfigurationType::Ptr(new ConfigurationType2005(*this));
}

void ConfigurationType2005::addDeploymentTool(DeploymentTool::Ptr tool)
{
    if (m_deploymentTools.contains(tool))
        return;
    m_deploymentTools.append(tool);
}

void ConfigurationType2005::removeDeploymentTool(DeploymentTool::Ptr tool)
{
    if (m_deploymentTools.contains(tool))
        m_deploymentTools.removeAll(tool);
}

QList<DeploymentTool::Ptr> ConfigurationType2005::deploymentTools() const
{
    return m_deploymentTools;
}

QList<DeploymentTool::Ptr> ConfigurationType2005::deploymentTools(const QString &attributeName, const QString &attributeValue) const
{
    QList<DeploymentTool::Ptr> deploymentTools;

    foreach (const DeploymentTool::Ptr &tool, m_deploymentTools) {
        if (tool->attributeValue(attributeName) == attributeValue)
            deploymentTools.append(tool);
    }
    return deploymentTools;
}

ConfigurationType2005::ConfigurationType2005()
{
}

ConfigurationType2005::ConfigurationType2005(const ConfigurationType2005 &configType)
    : ConfigurationType2003(configType)
{
    foreach (const DeploymentTool::Ptr &tool, configType.m_deploymentTools)
        m_deploymentTools.append(DeploymentTool::Ptr(new DeploymentTool(*tool)));
}

ConfigurationType2005 &ConfigurationType2005::operator=(const ConfigurationType2005 &configType)
{
    if (this != &configType) {
        ConfigurationType2003::operator =(configType);

        m_deploymentTools.clear();
        foreach (const DeploymentTool::Ptr &tool, configType.m_deploymentTools)
            m_deploymentTools.append(DeploymentTool::Ptr(new DeploymentTool(*tool)));
    }
    return *this;
}


ConfigurationType2008::~ConfigurationType2008()
{
    m_debuggerTools.clear();
}

void ConfigurationType2008::processToolNode(const QDomNode &toolNode)
{
    if (toolNode.nodeName() == QLatin1String("Tool")) {
        Tool::Ptr tool;
        QDomNamedNodeMap namedNodeMap = toolNode.toElement().attributes();

        for (int i = 0; i < namedNodeMap.size(); ++i) {
            QDomNode domNode = namedNodeMap.item(i);

            if (domNode.nodeType() == QDomNode::AttributeNode) {
                QDomAttr domAttribute = domNode.toAttr();
                if (domAttribute.name() == QLatin1String("Name")) {
                    tool = ToolFactory::createTool(domAttribute.value());
                    break;
                }
            }
        }

        if (tool) {
            tool->processNode(toolNode);
            m_tools.append(tool);
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

QDomNode ConfigurationType2008::toXMLDomNode(QDomDocument &domXMLDocument, const QString &nodeName) const
{
    QDomElement configurationNode = domXMLDocument.createElement(nodeName);

    configurationNode.setAttribute(QLatin1String("Name"), m_name);

    QHashIterator<QString, QString> it(m_anyAttribute);

    while (it.hasNext()) {
        it.next();
        configurationNode.setAttribute(it.key(), it.value());
    }

    foreach (const Tool::Ptr &tool, m_tools)
        configurationNode.appendChild(tool->toXMLDomNode(domXMLDocument));

    foreach (const DeploymentTool::Ptr &tool, m_deploymentTools)
        configurationNode.appendChild(tool->toXMLDomNode(domXMLDocument));

    foreach (const DebuggerTool::Ptr &tool, m_debuggerTools)
        configurationNode.appendChild(tool->toXMLDomNode(domXMLDocument));

    return configurationNode;
}

ConfigurationType::Ptr ConfigurationType2008::clone() const
{
    return ConfigurationType::Ptr(new ConfigurationType2008(*this));
}

void ConfigurationType2008::addDebuggerTool(DebuggerTool::Ptr tool)
{
    if (m_debuggerTools.contains(tool))
        return;
    m_debuggerTools.append(tool);
}

void ConfigurationType2008::removeDebuggerTool(DebuggerTool::Ptr tool)
{
    m_debuggerTools.removeAll(tool);
}

QList<DebuggerTool::Ptr> ConfigurationType2008::debuggerTools() const
{
    return m_debuggerTools;
}

QList<DebuggerTool::Ptr> ConfigurationType2008::debuggerTools(const QString &attributeName, const QString &attributeValue) const
{
    QList<DebuggerTool::Ptr> debuggerTools;

    foreach (const DebuggerTool::Ptr &tool, m_debuggerTools) {
        if (tool->attributeValue(attributeName) == attributeValue)
            debuggerTools.append(tool);
    }
    return debuggerTools;
}

ConfigurationType2008::ConfigurationType2008()
{
}

ConfigurationType2008::ConfigurationType2008(const ConfigurationType2008 &configType)
    : ConfigurationType2005(configType)
{
    foreach (const DebuggerTool::Ptr &tool, configType.m_debuggerTools)
        m_debuggerTools.append(DebuggerTool::Ptr(new DebuggerTool(*tool)));
}

ConfigurationType2008 &ConfigurationType2008::operator=(const ConfigurationType2008 &configType)
{
    if (this != &configType) {
        ConfigurationType2005::operator =(configType);
        m_debuggerTools.clear();

        foreach (const DebuggerTool::Ptr &tool, configType.m_debuggerTools)
            m_debuggerTools.append(DebuggerTool::Ptr(new DebuggerTool(*tool)));
    }
    return *this;
}

} // namespace Internal
} // namespace VcProjectManager
