// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagerrunconfiguration.h"

#include "appmanagerconstants.h"
#include "appmanagerstringaspect.h"
#include "appmanagertargetinformation.h"
#include "appmanagertr.h"
#include "appmanagerutilities.h"

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <boot2qt/qdbconstants.h>
#include <remotelinux/remotelinux_constants.h>

#include <utils/algorithm.h>
#include <utils/filesystemwatcher.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AppManager::Internal {

class AppManagerRunConfiguration : public RunConfiguration
{
public:
    AppManagerRunConfiguration(BuildConfiguration *bc, Id id)
        : RunConfiguration(bc, id)
    {
        setDefaultDisplayName(Tr::tr("Run an Application Manager Package"));

        setUpdater([this, bc] {
            QList<TargetInformation> tis = TargetInformation::readFromProject(bc, buildKey());
            if (tis.isEmpty())
                return;
            const TargetInformation targetInformation = tis.at(0);

            controller.setValue(getToolFilePath(Constants::APPMAN_CONTROLLER, kit(),
                                                RunDeviceKitAspect::device(kit())));

            appId.setValue(targetInformation.manifest.id);
            appId.setReadOnly(true);
        });

        connect(buildSystem(), &BuildSystem::parsingFinished, this, &RunConfiguration::update);
    }

    AppManagerControllerAspect controller{this};
    AppManagerIdAspect appId{this};
    AppManagerDocumentUrlAspect documentUrl{this};
    AppManagerRestartIfRunningAspect restartIfRunning{this};
    AppManagerInstanceIdAspect instanceId{this};
};

class AppManagerRunAndDebugConfiguration final : public AppManagerRunConfiguration
{
public:
    AppManagerRunAndDebugConfiguration(BuildConfiguration *bc, Id id)
        : AppManagerRunConfiguration(bc, id)
    {
        setDefaultDisplayName(Tr::tr("Run and Debug an Application Manager Package"));
        environment.addPreferredBaseEnvironment(Tr::tr("Clean Environment"), {});
    }

    EnvironmentAspect environment{this};
};

class AppManagerRunConfigurationFactory : public RunConfigurationFactory
{
public:
    AppManagerRunConfigurationFactory()
    {
        registerRunConfiguration<AppManagerRunConfiguration>(Constants::RUNCONFIGURATION_ID);
        addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
        addSupportedTargetDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
        addSupportedTargetDeviceType(Qdb::Constants::QdbLinuxOsType);
    }

    bool supportsBuildKey(BuildConfiguration *bc, const QString &key) const final
    {
        return !TargetInformation::readFromProject(bc, key).isEmpty();
    }

    virtual bool filterTarget(Kit *kit, const TargetInformation &ti) const
    {
        return !ti.manifest.supportsDebugging() ||
               RunDeviceKitAspect::device(kit)->osType() != OsType::OsTypeLinux;
    }

    QList<RunConfigurationCreationInfo> availableCreators(BuildConfiguration *bc) const final
    {
        QObject::connect(&m_fileSystemWatcher, &FileSystemWatcher::fileChanged,
                         bc->project(), &Project::displayNameChanged,
                         Qt::UniqueConnection);

        const auto buildTargets = TargetInformation::readFromProject(bc);
        const auto filteredTargets = Utils::filtered(buildTargets, [this, bc](const TargetInformation &ti) {
            return filterTarget(bc->kit(), ti);
        });
        auto result = Utils::transform(filteredTargets, [this, bc](const TargetInformation &ti) {
            RunConfigurationCreationInfo rci;
            rci.factory = this;
            rci.buildKey = ti.buildKey;
            rci.displayName = decoratedTargetName(ti.displayName, bc->kit());
            rci.displayNameUniquifier = ti.displayNameUniquifier;
            rci.creationMode = RunConfigurationCreationInfo::AlwaysCreate;
            rci.projectFilePath = ti.manifest.filePath;
            rci.useTerminal = false;
            if (!m_fileSystemWatcher.files().contains(ti.manifest.filePath)) {
                m_fileSystemWatcher.addFile(ti.manifest.filePath, FileSystemWatcher::WatchAllChanges);
            }
            return rci;
        });

        return result;
    }

    mutable FileSystemWatcher m_fileSystemWatcher;
};

class AppManagerRunAndDebugConfigurationFactory final : public AppManagerRunConfigurationFactory
{
public:
    AppManagerRunAndDebugConfigurationFactory()
    {
        registerRunConfiguration<AppManagerRunAndDebugConfiguration>(Constants::RUNANDDEBUGCONFIGURATION_ID);
        addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
        addSupportedTargetDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
        addSupportedTargetDeviceType(Qdb::Constants::QdbLinuxOsType);
    }

    virtual bool filterTarget(Kit *kit, const TargetInformation &ti) const final
    {
        return !AppManagerRunConfigurationFactory::filterTarget(kit, ti);
    }
};

void setupAppManagerRunConfiguration()
{
    static AppManagerRunConfigurationFactory theAppManagerRunConfigurationFactory;
    static AppManagerRunAndDebugConfigurationFactory theAppManagerRunAndDebugConfigurationFactory;
}

} // AppManager::Internal
