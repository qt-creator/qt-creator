/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef S60DEPLOYCONFIGURATION_H
#define S60DEPLOYCONFIGURATION_H

#include "symbianidevice.h"

#include <projectexplorer/deployconfiguration.h>
#include <qt4projectmanager/qt4projectmanager_global.h>

namespace ProjectExplorer {
class BuildConfiguration;
class RunConfiguration;
class ToolChain;
}

namespace QtSupport {
class BaseQtVersion;
}

namespace Qt4ProjectManager {
class Qt4ProFileNode;
class S60DeployConfigurationFactory;

namespace Internal {
class Qt4SymbianTarget;

const char S60_DEPLOYCONFIGURATION_ID[] = "Qt4ProjectManager.S60DeployConfiguration";
}

class QT4PROJECTMANAGER_EXPORT S60DeployConfiguration : public ProjectExplorer::DeployConfiguration
{
    Q_OBJECT
    friend class S60DeployConfigurationFactory;

public:
    typedef QPair<char, int> DeviceDrive;

    explicit S60DeployConfiguration(ProjectExplorer::Target *parent);
    virtual ~S60DeployConfiguration();

    ProjectExplorer::DeployConfigurationWidget *configurationWidget() const;

    char installationDrive() const;
    void setInstallationDrive(char drive);

    bool silentInstall() const;
    void setSilentInstall(bool silent);

    void setAvailableDeviceDrives(QList<DeviceDrive> drives);
    const QList<DeviceDrive> &availableDeviceDrives() const;

    QStringList signedPackages() const;
    QStringList packageFileNamesWithTargetInfo() const;
    QStringList packageTemplateFileNames() const;
    QStringList appPackageTemplateFileNames() const;

    bool runSmartInstaller() const;
    SymbianIDevice::ConstPtr device() const;

    QVariantMap toMap() const;

signals:
    void deviceChanged();
    void targetInformationChanged();
    void availableDeviceDrivesChanged();
    void installationDriveChanged();

private slots:
    void slotTargetInformationChanged(Qt4ProjectManager::Qt4ProFileNode*,bool success, bool parseInProgress);
    void updateActiveBuildConfiguration(ProjectExplorer::BuildConfiguration *buildConfiguration);
    void updateActiveRunConfiguration(ProjectExplorer::RunConfiguration *runConfiguration);

protected:
    S60DeployConfiguration(ProjectExplorer::Target *parent, S60DeployConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);
    QString defaultDisplayName() const;

private:
    void ctor();
    bool isSigned() const;
    QString symbianTarget() const;
    QString createPackageName(const QString &baseName) const;
    bool isDebug() const;
    bool isStaticLibrary(const Qt4ProFileNode &projectNode) const;
    bool isApplication(const Qt4ProFileNode &projectNode) const;
    bool hasSisPackage(const Qt4ProFileNode &projectNode) const;

private:
    ProjectExplorer::BuildConfiguration *m_activeBuildConfiguration;
    Core::Id m_deviceId;

    char m_installationDrive;
    bool m_silentInstall;
    QList<DeviceDrive> m_availableDeviceDrives;
};

class S60DeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_OBJECT

public:
    explicit S60DeployConfigurationFactory(QObject *parent = 0);
    ~S60DeployConfigurationFactory();

    bool canCreate(ProjectExplorer::Target *parent, const Core::Id id) const;
    ProjectExplorer::DeployConfiguration *create(ProjectExplorer::Target *parent, const Core::Id id);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::DeployConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source) const;
    ProjectExplorer::DeployConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source);

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent) const;
    // used to translate the ids to names to display to the user
    QString displayNameForId(const Core::Id id) const;
};

} // namespace Qt4ProjectManager

#endif // S60DEPLOYCONFIGURATION_H
