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
#include "tools/toolfactory.h"
#include "tools/tool_constants.h"
#include "../widgets/configurationwidgets.h"

namespace VcProjectManager {
namespace Internal {

using namespace ToolConstants;

Configuration::~Configuration()
{
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
    m_configType->processNodeAttributes(element);
}

QString Configuration::nodeWidgetName() const
{
    return m_configType->name();
}

QDomNode Configuration::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    return m_configType->toXMLDomNode(domXMLDocument, QLatin1String("Configuration"));
}

void Configuration::addTool(Tool::Ptr tool)
{
    if (tool)
        m_configType->addTool(tool);
}

void Configuration::removeTool(Tool::Ptr tool)
{
    m_configType->removeTool(tool);
}

Tool::Ptr Configuration::tool(const QString &toolName) const
{
    return m_configType->tool(toolName);
}

QList<Tool::Ptr> Configuration::tools() const
{
    return m_configType->tools();
}

QString Configuration::name() const
{
    return m_configType->name();
}

void Configuration::setName(const QString &name)
{
    m_configType->setName(name);
    emit nameChanged();
}

QString Configuration::oldName() const
{
    return m_configType->oldName();
}

void Configuration::setOldName(const QString &oldName)
{
    m_configType->setOldName(oldName);
}

QString Configuration::attributeValue(const QString &attributeName) const
{
    return m_configType->attributeValue(attributeName);
}

void Configuration::setAttribute(const QString &attributeName, const QString &attributeValue)
{
    m_configType->setAttribute(attributeName, attributeValue);
}

void Configuration::clearAttribute(const QString &attributeName)
{
    m_configType->clearAttribute(attributeName);
}

void Configuration::removeAttribute(const QString &attributeName)
{
    m_configType->removeAttribute(attributeName);
}

Configuration::Configuration()
{
}

Configuration::Configuration(const Configuration &config)
{
    m_configType = config.m_configType->clone();
}

Configuration &Configuration::operator =(const Configuration &config)
{
    if (this != &config)
        m_configType = config.m_configType->clone();
    return *this;
}

void Configuration::processToolNode(const QDomNode &toolNode)
{
    m_configType->processToolNode(toolNode);
}


Configuration2003::Configuration2003(const Configuration2003 &config)
    : Configuration(config)
{
}

Configuration2003 &Configuration2003::operator =(const Configuration2003 &config)
{
    if (this != &config)
        Configuration::operator =(config);
    return *this;
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

Configuration2003::Configuration2003()
{
    m_configType = QSharedPointer<ConfigurationType2003>(new ConfigurationType2003);
}


Configuration2005::Configuration2005()
{
    m_configType = QSharedPointer<ConfigurationType2005>(new ConfigurationType2005);
}

Configuration2005::Configuration2005(const Configuration2005 &config)
    : Configuration2003(config)
{
}

Configuration2005 &Configuration2005::operator=(const Configuration2005 &config)
{
    if (this != &config)
        Configuration2003::operator =(config);
    return *this;
}

Configuration2005::~Configuration2005()
{
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
    QSharedPointer<ConfigurationType2005> conf = m_configType.staticCast<ConfigurationType2005>();
    conf->addDeploymentTool(tool);
}

void Configuration2005::removeDeploymentTool(DeploymentTool::Ptr tool)
{
    QSharedPointer<ConfigurationType2005> conf = m_configType.staticCast<ConfigurationType2005>();
    conf->removeDeploymentTool(tool);
}

QList<DeploymentTool::Ptr> Configuration2005::deploymentTools() const
{
    QSharedPointer<ConfigurationType2005> conf = m_configType.staticCast<ConfigurationType2005>();
    return conf->deploymentTools();
}

QList<DeploymentTool::Ptr> Configuration2005::deploymentTools(const QString &attributeName, const QString &attributeValue) const
{
    QSharedPointer<ConfigurationType2005> conf = m_configType.staticCast<ConfigurationType2005>();
    return conf->deploymentTools(attributeName, attributeValue);
}


Configuration2008::Configuration2008(const Configuration2008 &config)
    : Configuration2005(config)
{
}

Configuration2008 &Configuration2008::operator=(const Configuration2008 &config)
{
    if (this != &config)
        Configuration2005::operator =(config);
    return *this;
}

Configuration2008::~Configuration2008()
{
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
    QSharedPointer<ConfigurationType2008> conf = m_configType.staticCast<ConfigurationType2008>();
    conf->addDebuggerTool(tool);
}

void Configuration2008::removeDebuggerTool(DebuggerTool::Ptr tool)
{
    QSharedPointer<ConfigurationType2008> conf = m_configType.staticCast<ConfigurationType2008>();
    conf->removeDebuggerTool(tool);
}

QList<DebuggerTool::Ptr> Configuration2008::debuggerTools() const
{
    QSharedPointer<ConfigurationType2008> conf = m_configType.staticCast<ConfigurationType2008>();
    return conf->debuggerTools();
}

QList<DebuggerTool::Ptr> Configuration2008::debuggerTools(const QString &attributeName, const QString &attributeValue) const
{
    QSharedPointer<ConfigurationType2008> conf = m_configType.staticCast<ConfigurationType2008>();
    return conf->debuggerTools(attributeName, attributeValue);
}

Configuration2008::Configuration2008()
{
    m_configType = QSharedPointer<ConfigurationType2008>(new ConfigurationType2008);
}

} // namespace Internal
} // namespace VcProjectManager
