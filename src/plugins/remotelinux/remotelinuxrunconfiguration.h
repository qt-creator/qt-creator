/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef MAEMORUNCONFIGURATION_H
#define MAEMORUNCONFIGURATION_H

#include "linuxdeviceconfiguration.h"
#include "maemoconstants.h"
#include "maemodeployable.h"
#include "remotelinux_export.h"

#include <utils/environment.h>

#include <projectexplorer/runconfiguration.h>

#include <QtCore/QDateTime>
#include <QtCore/QStringList>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;
class Qt4Project;
class Qt4BaseTarget;
class Qt4ProFileNode;
} // namespace Qt4ProjectManager

namespace RemoteLinux {
namespace Internal {
class AbstractLinuxDeviceDeployStep;
class MaemoDeviceConfigListModel;
class MaemoRemoteMountsModel;
class MaemoRunConfigurationFactory;
class MaemoRunConfigurationWidget;
class MaemoToolChain;
class Qt4MaemoDeployConfiguration;
class RemoteLinuxRunConfigurationPrivate;
} // namespace Internal

class REMOTELINUX_EXPORT RemoteLinuxRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class Internal::MaemoRunConfigurationFactory;
    friend class Internal::MaemoRunConfigurationWidget;

public:
    enum BaseEnvironmentType {
        CleanBaseEnvironment = 0,
        SystemBaseEnvironment = 1
    };

    enum DebuggingType { DebugCppOnly, DebugQmlOnly, DebugCppAndQml };

    RemoteLinuxRunConfiguration(Qt4ProjectManager::Qt4BaseTarget *parent,
        const QString &proFilePath);
    virtual ~RemoteLinuxRunConfiguration();

    bool isEnabled() const;
    QString disabledReason() const;
    QWidget *createConfigurationWidget();
    Utils::OutputFormatter *createOutputFormatter() const;
    Qt4ProjectManager::Qt4BaseTarget *qt4Target() const;
    Qt4ProjectManager::Qt4BuildConfiguration *activeQt4BuildConfiguration() const;

    Internal::Qt4MaemoDeployConfiguration *deployConfig() const;
    Internal::MaemoRemoteMountsModel *remoteMounts() const;

    QString localExecutableFilePath() const;
    QString remoteExecutableFilePath() const;
    QString arguments() const;
    QString commandPrefix() const;
    QSharedPointer<const LinuxDeviceConfiguration> deviceConfig() const;
    PortList freePorts() const;
    bool useRemoteGdb() const;
    void updateFactoryState() { emit isEnabledChanged(isEnabled()); }
    DebuggingType debuggingType() const;

    QString gdbCmd() const;
    QString localDirToMountForRemoteGdb() const;
    QString remoteProjectSourcesMountPoint() const;

    virtual QVariantMap toMap() const;

    QString baseEnvironmentText() const;
    BaseEnvironmentType baseEnvironmentType() const;
    Utils::Environment environment() const;
    Utils::Environment baseEnvironment() const;
    QList<Utils::EnvironmentItem> userEnvironmentChanges() const;
    Utils::Environment systemEnvironment() const;

    int portsUsedByDebuggers() const;
    bool hasEnoughFreePorts(const QString &mode) const;

    QString proFilePath() const;

signals:
    void deviceConfigurationChanged(ProjectExplorer::Target *target);
    void targetInformationChanged() const;
    void baseEnvironmentChanged();
    void systemEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &diff);

protected:
    RemoteLinuxRunConfiguration(Qt4ProjectManager::Qt4BaseTarget *parent,
        RemoteLinuxRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);
    QString defaultDisplayName();

private slots:
    void proFileUpdate(Qt4ProjectManager::Qt4ProFileNode *pro, bool success);
    void proFileInvalidated(Qt4ProjectManager::Qt4ProFileNode *pro);
    void updateDeviceConfigurations();
    void handleDeployConfigChanged();

private:
    void init();
    void handleParseState(bool success);
    Internal::AbstractLinuxDeviceDeployStep *deployStep() const;

    void setArguments(const QString &args);
    void setUseRemoteGdb(bool useRemoteGdb);
    void setBaseEnvironmentType(BaseEnvironmentType env);
    void setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff);
    void setSystemEnvironment(const Utils::Environment &environment);

    Internal::RemoteLinuxRunConfigurationPrivate * const m_d;
};

} // namespace RemoteLinux

#endif // MAEMORUNCONFIGURATION_H
