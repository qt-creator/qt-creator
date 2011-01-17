/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef S60DEPLOYCONFIGURATION_H
#define S60DEPLOYCONFIGURATION_H

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/toolchaintype.h>

namespace ProjectExplorer {
class BuildConfiguration;
class RunConfiguration;
}

namespace Qt4ProjectManager {
class QtVersion;

namespace Internal {
class Qt4SymbianTarget;
class Qt4ProFileNode;
class S60DeployConfigurationFactory;
class S60DeviceRunConfiguration;

class S60DeployConfiguration : public ProjectExplorer::DeployConfiguration
{
    Q_OBJECT
    friend class S60DeployConfigurationFactory;

public:
    enum CommunicationChannel {
        CommunicationSerialConnection,
        CommunicationTcpConnection
    };

    explicit S60DeployConfiguration(ProjectExplorer::Target *parent);
    virtual ~S60DeployConfiguration();

    bool isEnabled(ProjectExplorer::BuildConfiguration *configuration) const;

    ProjectExplorer::DeployConfigurationWidget *configurationWidget() const;

    const QtVersion *qtVersion() const;
    Qt4SymbianTarget *qt4Target() const;
    ProjectExplorer::ToolChainType toolChainType() const;

    QString serialPortName() const;
    void setSerialPortName(const QString &name);

    char installationDrive() const;
    void setInstallationDrive(char drive);

    bool silentInstall() const;
    void setSilentInstall(bool silent);

    QString deviceAddress() const;
    void setDeviceAddress(const QString &address);

    void setDevicePort(const QString &port);
    QString devicePort() const;

    void setCommunicationChannel(CommunicationChannel channel);
    S60DeployConfiguration::CommunicationChannel communicationChannel() const;

    QStringList signedPackages() const;
    QStringList packageFileNamesWithTargetInfo() const;
    QStringList packageTemplateFileNames() const;
    QStringList appPackageTemplateFileNames() const;

    bool runSmartInstaller() const;

    QVariantMap toMap() const;

signals:
    void targetInformationChanged();
    void serialPortNameChanged();
    void communicationChannelChanged();
    void deviceAddressChanged();
    void devicePortChanged();

private slots:
    void updateActiveBuildConfiguration(ProjectExplorer::BuildConfiguration *buildConfiguration);
    void updateActiveRunConfiguration(ProjectExplorer::RunConfiguration *runConfiguration);

protected:
    S60DeployConfiguration(ProjectExplorer::Target *parent, S60DeployConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);
    QString defaultDisplayName() const;

private:
    void ctor();
    bool isSigned() const;
    QString symbianPlatform() const;
    QString symbianTarget() const;
    QString createPackageName(const QString &baseName) const;
    bool isDebug() const;
    bool isStaticLibrary(const Qt4ProFileNode &projectNode) const;

private:
    ProjectExplorer::BuildConfiguration *m_activeBuildConfiguration;
    QString m_serialPortName;

    char m_installationDrive;
    bool m_silentInstall;
    QString m_deviceAddress;
    QString m_devicePort;
    CommunicationChannel m_communicationChannel;
};

class S60DeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_OBJECT

public:
    explicit S60DeployConfigurationFactory(QObject *parent = 0);
    ~S60DeployConfigurationFactory();

    bool canCreate(ProjectExplorer::Target *parent, const QString &id) const;
    ProjectExplorer::DeployConfiguration *create(ProjectExplorer::Target *parent, const QString &id);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::DeployConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source) const;
    ProjectExplorer::DeployConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source);

    QStringList availableCreationIds(ProjectExplorer::Target *parent) const;
    // used to translate the ids to names to display to the user
    QString displayNameForId(const QString &id) const;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60DEPLOYCONFIGURATION_H
