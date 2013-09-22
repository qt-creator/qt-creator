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
#ifndef VCPROJECTMANAGER_INTERNAL_CONFIGURATION_H
#define VCPROJECTMANAGER_INTERNAL_CONFIGURATION_H

#include "ivcprojectnodemodel.h"

#include "deploymenttool.h"
#include "debuggertool.h"
#include "../interfaces/iconfiguration.h"

namespace VcProjectManager {
namespace Internal {

class ConfigurationTool;
class GeneralAttributeContainer;

class Configuration : public IConfiguration
{
public:
    typedef QSharedPointer<Configuration>   Ptr;

    Configuration(const QString &nodeName);
    Configuration(const Configuration &config);
    Configuration& operator=(const Configuration &config);
    ~Configuration();
    void processNode(const QDomNode &node);
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    // IConfiguration interface
    IAttributeContainer *attributeContainer() const;
    QString fullName() const;
    QString name() const;
    QString platform() const;
    void setFullName(const QString &fullName);
    void setName(const QString &name);
    void setPlatform(const QString &platform);
    ITools *tools() const;

protected:
    virtual void processToolNode(const QDomNode &toolNode);
    void processNodeAttributes(const QDomElement &element);

    QString m_fullName;
    QString m_platformName;
    QString m_configurationName;
    QString m_nodeName;
    GeneralAttributeContainer *m_attributeContainer;
    ITools *m_tools;
};

class Configuration2003 : public Configuration
{
public:
    Configuration2003(const QString &nodeName);
    Configuration2003(const Configuration2003 &config);
    ~Configuration2003();

    VcNodeWidget *createSettingsWidget();
    IConfiguration *clone() const;
};

class Configuration2005 : public Configuration2003
{
public:
    Configuration2005(const QString &nodeName);
    Configuration2005(const Configuration2005 &config);
    Configuration2005& operator=(const Configuration2005 &config);
    ~Configuration2005();
    void processToolNode(const QDomNode &toolNode);
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    VcNodeWidget* createSettingsWidget();
    IConfiguration* clone() const;

    void addDeploymentTool(DeploymentTool::Ptr tool);
    void removeDeploymentTool(DeploymentTool::Ptr tool);
    QList<DeploymentTool::Ptr> deploymentTools() const;
    QList<DeploymentTool::Ptr> deploymentTools(const QString &attributeName, const QString &attributeValue) const;

    QList<DeploymentTool::Ptr> m_deploymentTools;
};

class Configuration2008 : public Configuration2005
{
public:
    Configuration2008(const QString &nodeName);
    Configuration2008(const Configuration2008 &config);
    Configuration2008& operator=(const Configuration2008 &config);
    ~Configuration2008();

    void processToolNode(const QDomNode &toolNode);
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;
    VcNodeWidget* createSettingsWidget();
    IConfiguration* clone() const;

    void addDebuggerTool(DebuggerTool::Ptr tool);
    void removeDebuggerTool(DebuggerTool::Ptr tool);
    QList<DebuggerTool::Ptr> debuggerTools() const;
    QList<DebuggerTool::Ptr> debuggerTools(const QString &attributeName, const QString &attributeValue) const;

    QList<DebuggerTool::Ptr> m_debuggerTools;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_CONFIGURATION_H
