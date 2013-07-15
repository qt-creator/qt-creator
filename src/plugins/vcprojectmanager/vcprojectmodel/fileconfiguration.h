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
#ifndef VCPROJECTMANAGER_INTERNAL_FILECONFIGURATION_H
#define VCPROJECTMANAGER_INTERNAL_FILECONFIGURATION_H

#include "ivcprojectnodemodel.h"

#include "configurationtype.h"

namespace VcProjectManager {
namespace Internal {

class FileConfiguration : public IVcProjectXMLNode
{
public:
    typedef QSharedPointer<FileConfiguration>   Ptr;

    ~FileConfiguration();
    void processNode(const QDomNode &node);
    void processNodeAttributes(const QDomElement &element);
    VcNodeWidget* createSettingsWidget();
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    /*!
     * Implement in order to support creating a clone of a FileConfiguration instance.
     * \return A shared pointer to a newly created clone.
     */
    virtual FileConfiguration::Ptr clone() const = 0;

    void addTool(Tool::Ptr tool);
    void removeTool(Tool::Ptr tool);
    Tool::Ptr tool(const QString &toolName);
    QString name() const;
    void setName(const QString &name);
    QString oldName() const;
    void setOldName(const QString &name);
    QString attributeValue(const QString &attributeName) const;
    void setAttribute(const QString &attributeName, const QString &attributeValue);
    void clearAttribute(const QString &attributeName);
    void removeAttribute(const QString &attributeName);

protected:
    FileConfiguration(ConfigurationType *configType);
    FileConfiguration(const FileConfiguration &fileConfig);
    FileConfiguration& operator=(const FileConfiguration &fileConfig);

    virtual void processToolNode(const QDomNode &toolNode);
    ConfigurationType::Ptr m_configType;
};

class FileConfiguration2003 : public FileConfiguration
{
public:
    FileConfiguration2003();
    FileConfiguration2003(const FileConfiguration2003 &fileConfig);
    FileConfiguration2003& operator=(const FileConfiguration2003 &fileConfig);
    ~FileConfiguration2003();

    FileConfiguration::Ptr clone() const;
};

class FileConfiguration2005 : public FileConfiguration
{
public:
    FileConfiguration2005();
    FileConfiguration2005(const FileConfiguration2005 &fileConfig);
    FileConfiguration2005& operator=(const FileConfiguration2005 &fileConfig);
    ~FileConfiguration2005();

    FileConfiguration::Ptr clone() const;

    void addDeploymentTool(DeploymentTool::Ptr tool);
    void removeDeploymentTool(DeploymentTool::Ptr tool);
    QList<DeploymentTool::Ptr> deploymentTools() const;
    QList<DeploymentTool::Ptr> deploymentTools(const QString &attributeName, const QString &attributeValue) const;
};

class FileConfiguration2008 : public FileConfiguration
{
public:
    FileConfiguration2008();
    FileConfiguration2008(const FileConfiguration2008 &fileConfig);
    FileConfiguration2008& operator=(const FileConfiguration2008 &fileConfig);
    ~FileConfiguration2008();

    FileConfiguration::Ptr clone() const;

    void addDebuggerTool(DebuggerTool::Ptr tool);
    void removeDebuggerTool(DebuggerTool::Ptr tool);
    QList<DeploymentTool::Ptr> deploymentTools() const;
    QList<DeploymentTool::Ptr> deploymentTools(const QString &attributeName, const QString &attributeValue) const;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_FILECONFIGURATION_H
