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
#include "referenceconfiguration.h"

#include "deploymenttool.h"
#include "debuggertool.h"
#include "tools/tool.h"
#include "tools/toolfactory.h"

namespace VcProjectManager {
namespace Internal {

ReferenceConfiguration::~ReferenceConfiguration()
{
}

void ReferenceConfiguration::processNode(const QDomNode &node)
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

void ReferenceConfiguration::processNodeAttributes(const QDomElement &element)
{
    m_configType->processNodeAttributes(element);
}

VcNodeWidget *ReferenceConfiguration::createSettingsWidget()
{
    return 0;
}

QDomNode ReferenceConfiguration::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    return m_configType->toXMLDomNode(domXMLDocument, QLatin1String("ReferenceConfiguration"));
}

void ReferenceConfiguration::addTool(Tool::Ptr tool)
{
    m_configType->addTool(tool);
}

void ReferenceConfiguration::removeTool(Tool::Ptr tool)
{
    m_configType->removeTool(tool);
}

Tool::Ptr ReferenceConfiguration::tool(const QString &toolName)
{
    return m_configType->tool(toolName);
}

QString ReferenceConfiguration::name() const
{
    return m_configType->name();
}

void ReferenceConfiguration::setName(const QString &name)
{
    m_configType->setName(name);
}

QString ReferenceConfiguration::oldName() const
{
    return m_configType->name();
}

void ReferenceConfiguration::setOldName(const QString &name)
{
    m_configType->setOldName(name);
}

QString ReferenceConfiguration::attributeValue(const QString &attributeName) const
{
    return m_configType->attributeValue(attributeName);
}

void ReferenceConfiguration::setAttribute(const QString &attributeName, const QString &attributeValue)
{
    m_configType->setAttribute(attributeName, attributeValue);
}

void ReferenceConfiguration::clearAttribute(const QString &attributeName)
{
    m_configType->clearAttribute(attributeName);
}

void ReferenceConfiguration::removeAttribute(const QString &attributeName)
{
    m_configType->removeAttribute(attributeName);
}

ReferenceConfiguration::ReferenceConfiguration()
    : m_configType(0)
{
}

ReferenceConfiguration::ReferenceConfiguration(const ReferenceConfiguration &refConf)
{
    m_configType = refConf.m_configType->clone();
}

ReferenceConfiguration &ReferenceConfiguration::operator =(const ReferenceConfiguration &refConf)
{
    if (this != &refConf)
        m_configType = refConf.m_configType->clone();
    return *this;
}

void ReferenceConfiguration::processToolNode(const QDomNode &toolNode)
{
    m_configType->processToolNode(toolNode);
}


ReferenceConfiguration2003::ReferenceConfiguration2003(const ReferenceConfiguration2003 &refConf)
    : ReferenceConfiguration(refConf)
{
}

ReferenceConfiguration2003 &ReferenceConfiguration2003::operator =(const ReferenceConfiguration2003 &refConf)
{
    ReferenceConfiguration::operator =(refConf);
    return *this;
}

ReferenceConfiguration2003::~ReferenceConfiguration2003()
{
}

ReferenceConfiguration::Ptr ReferenceConfiguration2003::clone() const
{
    return ReferenceConfiguration::Ptr(new ReferenceConfiguration2003(*this));
}

ReferenceConfiguration2003::ReferenceConfiguration2003()
{
}

void ReferenceConfiguration2003::init()
{
    m_configType = QSharedPointer<ConfigurationType2003>(new ConfigurationType2003);
}


ReferenceConfiguration2005::ReferenceConfiguration2005(const ReferenceConfiguration2005 &refConf)
    : ReferenceConfiguration2003(refConf)
{
}

ReferenceConfiguration2005 &ReferenceConfiguration2005::operator =(const ReferenceConfiguration2005 &refConf)
{
    ReferenceConfiguration::operator =(refConf);
    return *this;
}

ReferenceConfiguration2005::~ReferenceConfiguration2005()
{
}

ReferenceConfiguration::Ptr ReferenceConfiguration2005::clone() const
{
    return ReferenceConfiguration::Ptr(new ReferenceConfiguration2005(*this));
}

void ReferenceConfiguration2005::addDeploymentTool(DeploymentTool::Ptr tool)
{
    QSharedPointer<ConfigurationType2005> conf = m_configType.staticCast<ConfigurationType2005>();
    conf->addDeploymentTool(tool);
}

void ReferenceConfiguration2005::removeDeploymentTool(DeploymentTool::Ptr tool)
{
    QSharedPointer<ConfigurationType2005> conf = m_configType.staticCast<ConfigurationType2005>();
    conf->removeDeploymentTool(tool);
}

QList<DeploymentTool::Ptr > ReferenceConfiguration2005::deploymentTools() const
{
    QSharedPointer<ConfigurationType2005> conf = m_configType.staticCast<ConfigurationType2005>();
    return conf->deploymentTools();
}

QList<DeploymentTool::Ptr > ReferenceConfiguration2005::deploymentTools(const QString &attributeName, const QString &attributeValue) const
{
    QSharedPointer<ConfigurationType2005> conf = m_configType.staticCast<ConfigurationType2005>();
    return conf->deploymentTools(attributeName, attributeValue);
}

ReferenceConfiguration2005::ReferenceConfiguration2005()
{
}

void ReferenceConfiguration2005::init()
{
    m_configType = QSharedPointer<ConfigurationType2005>(new ConfigurationType2005);
}


ReferenceConfiguration2008::ReferenceConfiguration2008(const ReferenceConfiguration2008 &refConf)
    : ReferenceConfiguration2005(refConf)
{
}

ReferenceConfiguration2008 &ReferenceConfiguration2008::operator =(const ReferenceConfiguration2008 &refConf)
{
    ReferenceConfiguration::operator =(refConf);
    return *this;
}

ReferenceConfiguration2008::~ReferenceConfiguration2008()
{
}

ReferenceConfiguration::Ptr ReferenceConfiguration2008::clone() const
{
    return ReferenceConfiguration::Ptr(new ReferenceConfiguration2008(*this));
}

void ReferenceConfiguration2008::addDebuggerTool(DebuggerTool::Ptr tool)
{
    QSharedPointer<ConfigurationType2008> conf = m_configType.staticCast<ConfigurationType2008>();
    conf->addDebuggerTool(tool);
}

void ReferenceConfiguration2008::removeDebuggerTool(DebuggerTool::Ptr tool)
{
    QSharedPointer<ConfigurationType2008> conf = m_configType.staticCast<ConfigurationType2008>();
    conf->removeDebuggerTool(tool);
}

QList<DebuggerTool::Ptr > ReferenceConfiguration2008::debuggerTools() const
{
    QSharedPointer<ConfigurationType2008> conf = m_configType.staticCast<ConfigurationType2008>();
    return conf->debuggerTools();
}

QList<DebuggerTool::Ptr > ReferenceConfiguration2008::debuggerTools(const QString &attributeName, const QString &attributeValue) const
{
    QSharedPointer<ConfigurationType2008> conf = m_configType.staticCast<ConfigurationType2008>();
    return conf->debuggerTools(attributeName, attributeValue);
}

ReferenceConfiguration2008::ReferenceConfiguration2008()
{
}

void ReferenceConfiguration2008::init()
{
    m_configType = QSharedPointer<ConfigurationType2008>(new ConfigurationType2008);
}

} // namespace Internal
} // namespace VcProjectManager
