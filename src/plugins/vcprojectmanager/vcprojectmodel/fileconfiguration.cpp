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
#include "fileconfiguration.h"

#include "configurationtype.h"
#include "deploymenttool.h"
#include "debuggertool.h"
#include "tools/tool.h"
#include "tools/toolfactory.h"

namespace VcProjectManager {
namespace Internal {

FileConfiguration::~FileConfiguration()
{
}

void FileConfiguration::processNode(const QDomNode &node)
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

void FileConfiguration::processNodeAttributes(const QDomElement &element)
{
    m_configType->processNodeAttributes(element);
}

VcNodeWidget *FileConfiguration::createSettingsWidget()
{
    return 0;
}

QDomNode FileConfiguration::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    return m_configType->toXMLDomNode(domXMLDocument, QLatin1String("FileConfiguration"));
}

void FileConfiguration::addTool(Tool::Ptr tool)
{
    m_configType->addTool(tool);
}

void FileConfiguration::removeTool(Tool::Ptr tool)
{
    m_configType->removeTool(tool);
}

Tool::Ptr FileConfiguration::tool(const QString &toolName)
{
    return m_configType->tool(toolName);
}

QString FileConfiguration::name() const
{
    return m_configType->name();
}

void FileConfiguration::setName(const QString &name)
{
    m_configType->setName(name);
}

QString FileConfiguration::oldName() const
{
    return m_configType->oldName();
}

void FileConfiguration::setOldName(const QString &name)
{
    m_configType->setName(name);
}

QString FileConfiguration::attributeValue(const QString &attributeName) const
{
    return m_configType->attributeValue(attributeName);
}

void FileConfiguration::setAttribute(const QString &attributeName, const QString &attributeValue)
{
    m_configType->setAttribute(attributeName, attributeValue);
}

void FileConfiguration::clearAttribute(const QString &attributeName)
{
    m_configType->clearAttribute(attributeName);
}

void FileConfiguration::removeAttribute(const QString &attributeName)
{
    m_configType->removeAttribute(attributeName);
}

FileConfiguration::FileConfiguration(ConfigurationType *configType)
    : m_configType(configType)
{
}

FileConfiguration::FileConfiguration(const FileConfiguration &fileConfig)
{
    m_configType = fileConfig.m_configType->clone();
}

FileConfiguration &FileConfiguration::operator=(const FileConfiguration &fileConfig)
{
    if (this != &fileConfig)
        m_configType = fileConfig.m_configType->clone();
    return *this;
}

void FileConfiguration::processToolNode(const QDomNode &toolNode)
{
    m_configType->processToolNode(toolNode);
}


FileConfiguration2003::FileConfiguration2003()
    : FileConfiguration(new ConfigurationType2003)
{
}

FileConfiguration2003::FileConfiguration2003(const FileConfiguration2003 &fileConfig)
    : FileConfiguration(fileConfig)
{
}

FileConfiguration2003 &FileConfiguration2003::operator =(const FileConfiguration2003 &fileConfig)
{
    if (this != &fileConfig)
        FileConfiguration::operator =(fileConfig);
    return *this;
}

FileConfiguration2003::~FileConfiguration2003()
{
}

FileConfiguration::Ptr FileConfiguration2003::clone() const
{
    return FileConfiguration::Ptr(new FileConfiguration2003(*this));
}


FileConfiguration2005::FileConfiguration2005()
    : FileConfiguration(new ConfigurationType2005)
{
}

FileConfiguration2005::FileConfiguration2005(const FileConfiguration2005 &fileConfig)
    : FileConfiguration(fileConfig)
{
}

FileConfiguration2005 &FileConfiguration2005::operator=(const FileConfiguration2005 &fileConfig)
{
    if (this != &fileConfig)
        FileConfiguration::operator =(fileConfig);
    return *this;
}

FileConfiguration2005::~FileConfiguration2005()
{
}

FileConfiguration::Ptr FileConfiguration2005::clone() const
{
    return FileConfiguration::Ptr(new FileConfiguration2005(*this));
}

void FileConfiguration2005::addDeploymentTool(DeploymentTool::Ptr tool)
{
    QSharedPointer<ConfigurationType2005> conf = m_configType.staticCast<ConfigurationType2005>();
    conf->addDeploymentTool(tool);
}

void FileConfiguration2005::removeDeploymentTool(DeploymentTool::Ptr tool)
{
    QSharedPointer<ConfigurationType2005> conf = m_configType.staticCast<ConfigurationType2005>();
    conf->removeDeploymentTool(tool);
}

QList<DeploymentTool::Ptr > FileConfiguration2005::deploymentTools() const
{
    QSharedPointer<ConfigurationType2005> conf = m_configType.staticCast<ConfigurationType2005>();
    return conf->deploymentTools();
}

QList<DeploymentTool::Ptr > FileConfiguration2005::deploymentTools(const QString &attributeName, const QString &attributeValue) const
{
    QSharedPointer<ConfigurationType2005> conf = m_configType.staticCast<ConfigurationType2005>();
    return conf->deploymentTools(attributeName, attributeValue);
}

FileConfiguration2008::FileConfiguration2008()
    : FileConfiguration(new ConfigurationType2008)
{
}

FileConfiguration2008::FileConfiguration2008(const FileConfiguration2008 &fileConfig)
    : FileConfiguration(fileConfig)
{
}

FileConfiguration2008 &FileConfiguration2008::operator =(const FileConfiguration2008 &fileConfig)
{
    if (this != &fileConfig)
        FileConfiguration::operator =(fileConfig);
    return *this;
}

FileConfiguration2008::~FileConfiguration2008()
{
}

FileConfiguration::Ptr FileConfiguration2008::clone() const
{
    return FileConfiguration::Ptr(new FileConfiguration2008(*this));
}

void FileConfiguration2008::addDebuggerTool(DebuggerTool::Ptr tool)
{
    QSharedPointer<ConfigurationType2008> conf = m_configType.staticCast<ConfigurationType2008>();
    conf->addDebuggerTool(tool);
}

void FileConfiguration2008::removeDebuggerTool(DebuggerTool::Ptr tool)
{
    QSharedPointer<ConfigurationType2008> conf = m_configType.staticCast<ConfigurationType2008>();
    conf->removeDebuggerTool(tool);
}

QList<DeploymentTool::Ptr > FileConfiguration2008::deploymentTools() const
{
    QSharedPointer<ConfigurationType2008> conf = m_configType.staticCast<ConfigurationType2008>();
    return conf->deploymentTools();
}

QList<DeploymentTool::Ptr > FileConfiguration2008::deploymentTools(const QString &attributeName, const QString &attributeValue) const
{
    QSharedPointer<ConfigurationType2008> conf = m_configType.staticCast<ConfigurationType2008>();
    return conf->deploymentTools(attributeName, attributeValue);
}

} // namespace Internal
} // namespace VcProjectManager
