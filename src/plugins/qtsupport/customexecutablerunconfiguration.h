/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef CUSTOMEXECUTABLERUNCONFIGURATION_H
#define CUSTOMEXECUTABLERUNCONFIGURATION_H

#include "qtsupport_global.h"

#include <projectexplorer/localapplicationrunconfiguration.h>

#include <utils/environment.h>

#include <QVariantMap>

namespace ProjectExplorer { class Target; }

namespace QtSupport {
namespace Internal { class CustomExecutableConfigurationWidget; }

class CustomExecutableRunConfigurationFactory;

class QTSUPPORT_EXPORT CustomExecutableRunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration
{
    Q_OBJECT
    // the configuration widget needs to setExecutable setWorkingDirectory and setCommandLineArguments
    friend class Internal::CustomExecutableConfigurationWidget;
    friend class CustomExecutableRunConfigurationFactory;

public:
    explicit CustomExecutableRunConfiguration(ProjectExplorer::Target *parent);
    ~CustomExecutableRunConfiguration();

    /**
     * Returns the executable, looks in the environment for it and might even
     * ask the user if none is specified
     */
    QString executable() const;

    /** Returns whether this runconfiguration ever was configured with a executable
     */
    bool isConfigured() const;

    RunMode runMode() const;
    QString workingDirectory() const;
    QString commandLineArguments() const;
    Utils::Environment environment() const;

    QWidget *createConfigurationWidget();
    QString dumperLibrary() const;
    QStringList dumperLibraryLocations() const;

    ProjectExplorer::Abi abi() const;

    QVariantMap toMap() const;

    bool ensureConfigured(QString *errorMessage);

signals:
    void changed();

    void baseEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &diff);

protected:
    CustomExecutableRunConfiguration(ProjectExplorer::Target *parent,
                                     CustomExecutableRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);
    QString defaultDisplayName() const;

private:
    void ctor();

    enum BaseEnvironmentBase { CleanEnvironmentBase = 0,
                               SystemEnvironmentBase = 1,
                               BuildEnvironmentBase = 2};
    void setBaseEnvironmentBase(BaseEnvironmentBase env);
    BaseEnvironmentBase baseEnvironmentBase() const;
    Utils::Environment baseEnvironment() const;
    QString baseEnvironmentText() const;
    void setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff);
    QList<Utils::EnvironmentItem> userEnvironmentChanges() const;

    void setExecutable(const QString &executable);
    QString rawExecutable() const;
    void setCommandLineArguments(const QString &commandLineArguments);
    QString rawCommandLineArguments() const;
    void setBaseWorkingDirectory(const QString &workingDirectory);
    QString baseWorkingDirectory() const;
    void setUserName(const QString &name);
    void setRunMode(ProjectExplorer::LocalApplicationRunConfiguration::RunMode runMode);
    bool validateExecutable(QString *executable = 0, QString *errorMessage = 0) const;

    QString m_executable;
    QString m_workingDirectory;
    QString m_cmdArguments;
    RunMode m_runMode;
    bool m_userSetName;
    QString m_userName;
    QList<Utils::EnvironmentItem> m_userEnvironmentChanges;
    BaseEnvironmentBase m_baseEnvironmentBase;
};

class CustomExecutableRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit CustomExecutableRunConfigurationFactory(QObject *parent = 0);
    ~CustomExecutableRunConfigurationFactory();

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent) const;
    QString displayNameForId(const Core::Id id) const;

    bool canCreate(ProjectExplorer::Target *parent, const Core::Id id) const;
    ProjectExplorer::RunConfiguration *create(ProjectExplorer::Target *parent, const Core::Id id);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::RunConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *product) const;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent,
                                             ProjectExplorer::RunConfiguration *source);

private:
    bool canHandle(ProjectExplorer::Target *parent) const;
};

} // namespace QtSupport

#endif // CUSTOMEXECUTABLERUNCONFIGURATION_H
