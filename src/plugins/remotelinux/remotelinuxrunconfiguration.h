/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef REMOTELINUXRUNCONFIGURATION_H
#define REMOTELINUXRUNCONFIGURATION_H

#include "remotelinux_export.h"

#include <projectexplorer/runconfiguration.h>
#include <utils/environment.h>

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;
class Qt4BaseTarget;
class Qt4ProFileNode;
} // namespace Qt4ProjectManager

namespace Utils { class PortList; }

namespace RemoteLinux {
class LinuxDeviceConfiguration;
class RemoteLinuxRunConfigurationWidget;
class RemoteLinuxDeployConfiguration;

namespace Internal {
class RemoteLinuxRunConfigurationPrivate;
class RemoteLinuxRunConfigurationFactory;
} // namespace Internal

class REMOTELINUX_EXPORT RemoteLinuxRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    Q_DISABLE_COPY(RemoteLinuxRunConfiguration)
    friend class Internal::RemoteLinuxRunConfigurationFactory;
    friend class RemoteLinuxRunConfigurationWidget;

public:
    enum BaseEnvironmentType {
        CleanBaseEnvironment = 0,
        RemoteBaseEnvironment = 1
    };

    enum DebuggingType { DebugCppOnly, DebugQmlOnly, DebugCppAndQml };

    RemoteLinuxRunConfiguration(Qt4ProjectManager::Qt4BaseTarget *parent, const Core::Id id,
        const QString &proFilePath);

    bool isEnabled() const;
    QString disabledReason() const;
    QWidget *createConfigurationWidget();
    Utils::OutputFormatter *createOutputFormatter() const;
    Qt4ProjectManager::Qt4BaseTarget *qt4Target() const;
    Qt4ProjectManager::Qt4BuildConfiguration *activeQt4BuildConfiguration() const;

    RemoteLinuxDeployConfiguration *deployConfig() const;

    virtual QString environmentPreparationCommand() const;
    virtual QString commandPrefix() const;
    virtual Utils::PortList freePorts() const;

    QString localExecutableFilePath() const;
    QString defaultRemoteExecutableFilePath() const;
    QString remoteExecutableFilePath() const;
    QString arguments() const;
    void setArguments(const QString &args);
    QString workingDirectory() const;
    void setWorkingDirectory(const QString &wd);
    void setAlternateRemoteExecutable(const QString &exe);
    QString alternateRemoteExecutable() const;
    void setUseAlternateExecutable(bool useAlternate);
    bool useAlternateExecutable() const;
    QSharedPointer<const LinuxDeviceConfiguration> deviceConfig() const;
    QString gdbCmd() const;

    virtual QVariantMap toMap() const;

    QString baseEnvironmentText() const;
    BaseEnvironmentType baseEnvironmentType() const;
    Utils::Environment environment() const;
    Utils::Environment baseEnvironment() const;
    QList<Utils::EnvironmentItem> userEnvironmentChanges() const;
    Utils::Environment remoteEnvironment() const;

    int portsUsedByDebuggers() const;

    QString proFilePath() const;

    static const QString Id;

signals:
    void deviceConfigurationChanged(ProjectExplorer::Target *target);
    void deploySpecsChanged();
    void targetInformationChanged() const;
    void baseEnvironmentChanged();
    void remoteEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &diff);

protected:
    RemoteLinuxRunConfiguration(Qt4ProjectManager::Qt4BaseTarget *parent,
        RemoteLinuxRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);
    QString defaultDisplayName();
    void setDisabledReason(const QString &reason) const;
    QString userEnvironmentChangesAsString() const;

protected slots:
    void updateEnabledState() { emit enabledChanged(); }

private slots:
    void proFileUpdate(Qt4ProjectManager::Qt4ProFileNode *pro, bool success, bool parseInProgress);
    void updateDeviceConfigurations();
    void handleDeployConfigChanged();
    void handleDeployablesUpdated();

private:
    void init();

    void setBaseEnvironmentType(BaseEnvironmentType env);
    void setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff);
    void setRemoteEnvironment(const Utils::Environment &environment);

    Internal::RemoteLinuxRunConfigurationPrivate * const d;
};

} // namespace RemoteLinux

#endif // REMOTELINUXRUNCONFIGURATION_H
