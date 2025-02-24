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

    virtual bool supportsBuildKey(Target *target, const QString &key) const final
    {
        QList<TargetInformation> tis
            = TargetInformation::readFromProject(target->activeBuildConfiguration(), key);
        return !tis.isEmpty();
    }

    virtual bool filterTarget(Target *target, const TargetInformation &ti) const
    {
        return !ti.manifest.supportsDebugging() ||
               RunDeviceKitAspect::device(target->kit())->osType() != OsType::OsTypeLinux;
    }

    QList<RunConfigurationCreationInfo> availableCreators(Target *target) const
    {
        QObject::connect(&m_fileSystemWatcher, &FileSystemWatcher::fileChanged,
                         target->project(), &Project::displayNameChanged,
                         Qt::UniqueConnection);

        const auto buildTargets = TargetInformation::readFromProject(target->activeBuildConfiguration());
        const auto filteredTargets = Utils::filtered(buildTargets, [this, target](const TargetInformation &ti) {
            return filterTarget(target, ti);
        });
        auto result = Utils::transform(filteredTargets, [this, target](const TargetInformation &ti) {

            QVariantMap settings;
            // ti.buildKey is currently our app id
            settings.insert("id", ti.buildKey);
            target->setNamedSettings("runConfigurationSettings", settings);

            RunConfigurationCreationInfo rci;
            rci.factory = this;
            rci.buildKey = ti.buildKey;
            rci.displayName = decoratedTargetName(ti.displayName, target);
            rci.displayNameUniquifier = ti.displayNameUniquifier;
            rci.creationMode = RunConfigurationCreationInfo::AlwaysCreate;
            rci.projectFilePath = ti.manifest.filePath;
            rci.useTerminal = false;
            if (!m_fileSystemWatcher.files().contains(ti.manifest.filePath.toFSPathString())) {
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

    virtual bool filterTarget(Target *target, const TargetInformation &ti) const final
    {
        return !AppManagerRunConfigurationFactory::filterTarget(target, ti);
    }
};

void setupAppManagerRunConfiguration()
{
    static AppManagerRunConfigurationFactory theAppManagerRunConfigurationFactory;
    static AppManagerRunAndDebugConfigurationFactory theAppManagerRunAndDebugConfigurationFactory;
}

} // AppManager::Internal
