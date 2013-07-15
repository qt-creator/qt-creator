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
#ifndef VCPROJECTMANAGER_INTERNAL_CONFIGURATIONTYPE_H
#define VCPROJECTMANAGER_INTERNAL_CONFIGURATIONTYPE_H

#include <QList>
#include <QDomElement>

#include "tools/tool.h"
#include "deploymenttool.h"
#include "debuggertool.h"

namespace VcProjectManager {
namespace Internal {

class ConfigurationType
{
    friend class Configuration;
    friend class ReferenceConfiguration;
    friend class FileConfiguration;
public:
    typedef QSharedPointer<ConfigurationType>   Ptr;

    virtual ~ConfigurationType();
    virtual void processNodeAttributes(const QDomElement &element);
    virtual void processToolNode(const QDomNode &toolNode);
    virtual QDomNode toXMLDomNode(QDomDocument &domXMLDocument, const QString &nodeName) const;

    void addTool(Tool::Ptr tool);
    void removeTool(Tool::Ptr tool);
    Tool::Ptr tool(const QString &toolName) const;
    QList<Tool::Ptr> tools() const;
    QString name() const;
    void setName(const QString &name);
    QString oldName() const;
    void setOldName(const QString &oldName);

    QString attributeValue(const QString &attributeName) const;
    void setAttribute(const QString &attributeName, const QString &attributeValue);
    void clearAttribute(const QString &attributeName);
    void removeAttribute(const QString &attributeName);

    /*!
     * Implement in order to support creating a clone of a configuration type instance.
     * \return A shared pointer to newly created configuration type instance.
     */
    virtual ConfigurationType::Ptr clone() const = 0;

protected:
    ConfigurationType();
    ConfigurationType(const ConfigurationType &configType);
    ConfigurationType& operator =(const ConfigurationType &configType);

    QString m_name;
    QString m_oldName;
    QList<Tool::Ptr> m_tools;
    QHash<QString, QString> m_anyAttribute;
};

class ConfigurationType2003 : public ConfigurationType
{
    friend class Configuration2003;
    friend class ReferenceConfiguration2003;
    friend class FileConfiguration2003;

public:
    ~ConfigurationType2003();
    ConfigurationType::Ptr clone() const;

protected:
    ConfigurationType2003();
    ConfigurationType2003(const ConfigurationType2003 &configType);
    ConfigurationType2003& operator=(const ConfigurationType2003 &configType);
};

class ConfigurationType2005 : public ConfigurationType2003
{
    friend class Configuration2005;
    friend class ReferenceConfiguration2005;
    friend class FileConfiguration2005;

public:
    ~ConfigurationType2005();
    void processToolNode(const QDomNode &toolNode);
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument, const QString &nodeName) const;

    ConfigurationType::Ptr clone() const;

    void addDeploymentTool(DeploymentTool::Ptr tool);
    void removeDeploymentTool(DeploymentTool::Ptr tool);
    QList<DeploymentTool::Ptr> deploymentTools() const;
    QList<DeploymentTool::Ptr> deploymentTools(const QString &attributeName, const QString &attributeValue) const;

protected:
    ConfigurationType2005();
    ConfigurationType2005(const ConfigurationType2005 &configType);
    ConfigurationType2005& operator=(const ConfigurationType2005 &configType);
    QList<DeploymentTool::Ptr> m_deploymentTools;
};

class ConfigurationType2008 : public ConfigurationType2005
{
    friend class Configuration2008;
    friend class ReferenceConfiguration2008;
    friend class FileConfiguration2008;

public:
    ~ConfigurationType2008();
    void processToolNode(const QDomNode &toolNode);
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument, const QString &nodeName) const;

    ConfigurationType::Ptr clone() const;

    void addDebuggerTool(DebuggerTool::Ptr tool);
    void removeDebuggerTool(DebuggerTool::Ptr tool);
    QList<DebuggerTool::Ptr> debuggerTools() const;
    QList<DebuggerTool::Ptr> debuggerTools(const QString &attributeName, const QString &attributeValue) const;

protected:
    ConfigurationType2008();
    ConfigurationType2008(const ConfigurationType2008 &configType);
    ConfigurationType2008& operator=(const ConfigurationType2008 &configType);
    QList<DebuggerTool::Ptr> m_debuggerTools;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_CONFIGURATIONTYPE_H
