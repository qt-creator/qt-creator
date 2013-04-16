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
#ifndef VCPROJECTMANAGER_INTERNAL_REFERENCECONFIGURATION_H
#define VCPROJECTMANAGER_INTERNAL_REFERENCECONFIGURATION_H

#include "ivcprojectnodemodel.h"

#include "configurationtype.h"

namespace VcProjectManager {
namespace Internal {

class ReferenceConfiguration : public IVcProjectXMLNode
{
    friend class ReferenceConfigurationFactory;

public:
    typedef QSharedPointer<ReferenceConfiguration>  Ptr;

    ~ReferenceConfiguration();
    void processNode(const QDomNode &node);
    void processNodeAttributes(const QDomElement &element);
    VcNodeWidget* createSettingsWidget();
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    virtual ReferenceConfiguration::Ptr clone() const = 0;

    void addTool(Tool::Ptr tool);
    void removeTool(Tool::Ptr tool);
    Tool::Ptr tool(const QString &toolName);
    QString name() const;
    void setName(const QString &name);
    QString attributeValue(const QString &attributeName) const;
    void setAttribute(const QString &attributeName, const QString &attributeValue);
    void clearAttribute(const QString &attributeName);
    void removeAttribute(const QString &attributeName);

protected:
    ReferenceConfiguration();
    ReferenceConfiguration(const ReferenceConfiguration &refConf);
    ReferenceConfiguration& operator=(const ReferenceConfiguration &refConf);
    virtual void processToolNode(const QDomNode &toolNode);
    virtual void init() = 0;

    ConfigurationType::Ptr m_configType;
};

class ReferenceConfiguration2003 : public ReferenceConfiguration
{
    friend class ReferenceConfigurationFactory;

public:
    ReferenceConfiguration2003(const ReferenceConfiguration2003 &refConf);
    ReferenceConfiguration2003& operator=(const ReferenceConfiguration2003 &refConf);
    ~ReferenceConfiguration2003();

    ReferenceConfiguration::Ptr clone() const;

protected:
    ReferenceConfiguration2003();
    void init();
};

class ReferenceConfiguration2005 : public ReferenceConfiguration2003
{
    friend class ReferenceConfigurationFactory;

public:
    ReferenceConfiguration2005(const ReferenceConfiguration2005 &refConf);
    ReferenceConfiguration2005& operator=(const ReferenceConfiguration2005 &refConf);
    ~ReferenceConfiguration2005();

    ReferenceConfiguration::Ptr clone() const;

    void addDeploymentTool(DeploymentTool::Ptr tool);
    void removeDeploymentTool(DeploymentTool::Ptr tool);
    QList<DeploymentTool::Ptr > deploymentTools() const;
    QList<DeploymentTool::Ptr > deploymentTools(const QString &attributeName, const QString &attributeValue) const;

protected:
    ReferenceConfiguration2005();
    void init();
};

class ReferenceConfiguration2008 : public ReferenceConfiguration2005
{
    friend class ReferenceConfigurationFactory;

public:
    ReferenceConfiguration2008(const ReferenceConfiguration2008 &refConf);
    ReferenceConfiguration2008& operator=(const ReferenceConfiguration2008 &refConf);
    ~ReferenceConfiguration2008();

    ReferenceConfiguration::Ptr clone() const;

    void addDebuggerTool(DebuggerTool::Ptr tool);
    void removeDebuggerTool(DebuggerTool::Ptr tool);
    QList<DebuggerTool::Ptr > debuggerTools() const;
    QList<DebuggerTool::Ptr > debuggerTools(const QString &attributeName, const QString &attributeValue) const;

protected:
    ReferenceConfiguration2008();
    void init();
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_REFERENCECONFIGURATION_H
