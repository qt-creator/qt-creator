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
#ifndef VCPROJECTMANAGER_INTERNAL_CONFIGURATION_H
#define VCPROJECTMANAGER_INTERNAL_CONFIGURATION_H

#include "ivcprojectnodemodel.h"

#include "configurationtype.h"

namespace VcProjectManager {
namespace Internal {

class Configuration : public QObject, public IVcProjectXMLNode
{
    Q_OBJECT

    friend class ConfigurationsFactory;

public:
    typedef QSharedPointer<Configuration>   Ptr;

    ~Configuration();
    void processNode(const QDomNode &node);
    void processNodeAttributes(const QDomElement &element);
    virtual QString nodeWidgetName() const;
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    /*!
     * Implement in order to support creating a clone of a Configuration instance.
     * \return A shared pointer to newly created Configuration instance.
     */
    virtual Configuration::Ptr clone() const = 0;

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

signals:
    void nameChanged();

protected:
    Configuration();
    Configuration(const Configuration &config);
    Configuration& operator=(const Configuration &config);
    virtual void processToolNode(const QDomNode &toolNode);

    /*!
     * Called after instance of the Configuration is created in order to initialize \b m_private member variable to
     * a proper version of a ConfigurationType implementation (2003, 2005 or 2008).
     */
    virtual void init() = 0;

    ConfigurationType::Ptr m_configType;
};

class Configuration2003 : public Configuration
{
    friend class ConfigurationsFactory;

public:
    Configuration2003(const Configuration2003 &config);
    Configuration2003& operator=(const Configuration2003 &config);
    ~Configuration2003();

    VcNodeWidget *createSettingsWidget();
    Configuration::Ptr clone() const;

protected:
    Configuration2003();
    void init();
};

class Configuration2005 : public Configuration2003
{
    friend class ConfigurationsFactory;

public:
    Configuration2005(const Configuration2005 &config);
    Configuration2005& operator=(const Configuration2005 &config);
    ~Configuration2005();

    VcNodeWidget* createSettingsWidget();
    Configuration::Ptr clone() const;

    void addDeploymentTool(DeploymentTool::Ptr tool);
    void removeDeploymentTool(DeploymentTool::Ptr tool);
    QList<DeploymentTool::Ptr > deploymentTools() const;
    QList<DeploymentTool::Ptr > deploymentTools(const QString &attributeName, const QString &attributeValue) const;

protected:
    Configuration2005();
    void init();
};

class Configuration2008 : public Configuration2005
{
    friend class ConfigurationsFactory;

public:
    Configuration2008(const Configuration2008 &config);
    Configuration2008& operator=(const Configuration2008 &config);
    ~Configuration2008();

    VcNodeWidget* createSettingsWidget();
    Configuration::Ptr clone() const;

    void addDebuggerTool(DebuggerTool::Ptr tool);
    void removeDebuggerTool(DebuggerTool::Ptr tool);
    QList<DebuggerTool::Ptr > debuggerTools() const;
    QList<DebuggerTool::Ptr > debuggerTools(const QString &attributeName, const QString &attributeValue) const;

private:
    Configuration2008();
    void init();
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_CONFIGURATION_H
