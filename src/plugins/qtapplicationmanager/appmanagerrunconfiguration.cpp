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

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/environmentaspect.h>

#include <remotelinux/remotelinux_constants.h>

#include <utils/algorithm.h>
#include <utils/filesystemwatcher.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AppManager::Internal {

class AppManagerRunConfiguration : public RunConfiguration
{
public:
    AppManagerRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        setDefaultDisplayName(Tr::tr("Run an Appman Package"));

        setUpdater([this, target] {
            QList<TargetInformation> tis = TargetInformation::readFromProject(target, buildKey());
            if (tis.isEmpty())
                return;
            const TargetInformation targetInformation = tis.at(0);

            controller.setValue(FilePath::fromString(getToolFilePath(Constants::APPMAN_CONTROLLER, target->kit(),
                                                                     targetInformation.device)));

            appId.setValue(targetInformation.manifest.id);
            appId.setReadOnly(true);
        });

        connect(target, &Target::parsingFinished, this, &RunConfiguration::update);
        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
        connect(target, &Target::deploymentDataChanged, this, &RunConfiguration::update);
        connect(target, &Target::kitChanged, this, &RunConfiguration::update);
    }

    AppManagerControllerAspect controller{this};
    AppManagerIdAspect appId{this};
    AppManagerDocumentUrlAspect documentUrl{this};
    AppManagerInstanceIdAspect instanceId{this};
};

class AppManagerRunAndDebugConfiguration final : public AppManagerRunConfiguration
{
public:
    AppManagerRunAndDebugConfiguration(Target *target, Id id)
        : AppManagerRunConfiguration(target, id)
    {
        setDefaultDisplayName(Tr::tr("Run and Debug an Appman Package"));
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
    }

    virtual bool supportsBuildKey(Target *target, const QString &key) const final
    {
        QList<TargetInformation> tis = TargetInformation::readFromProject(target);
        return Utils::anyOf(tis, [key](const TargetInformation &ti) {
            return ti.buildKey == key;
        });
    }

    virtual bool filterTarget(const TargetInformation &ti) const
    {
        return !ti.manifest.supportsDebugging();
    }

    QList<RunConfigurationCreationInfo> availableCreators(Target *target) const
    {
        QObject::connect(&m_fileSystemWatcher, &FileSystemWatcher::fileChanged,
                         target->project(), &Project::displayNameChanged,
                         Qt::UniqueConnection);

        const auto buildTargets = TargetInformation::readFromProject(target);
        const auto filteredTargets = Utils::filtered(buildTargets, [this](const TargetInformation &ti){
            return filterTarget(ti);
        });
        auto result = Utils::transform(filteredTargets, [this, target](const TargetInformation &ti) {

            QVariantMap settings;
            // ti.buildKey is currently our app id
            settings.insert("id", ti.buildKey);
            target->setNamedSettings("runConfigurationSettings", settings);

            RunConfigurationCreationInfo rci;
            rci.factory = this;
            rci.buildKey = ti.buildKey;
            rci.displayName = ti.displayName;
            rci.displayNameUniquifier = ti.displayNameUniquifier;
            rci.creationMode = RunConfigurationCreationInfo::AlwaysCreate;
            rci.projectFilePath = Utils::FilePath::fromString(ti.manifest.fileName);
            rci.useTerminal = false;
            if (!m_fileSystemWatcher.files().contains(ti.manifest.fileName)) {
                m_fileSystemWatcher.addFile(ti.manifest.fileName, FileSystemWatcher::WatchAllChanges);
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
    }

    virtual bool filterTarget(const TargetInformation &ti) const final
    {
        return ti.manifest.supportsDebugging();
    }
};

void setupAppManagerRunConfiguration()
{
    static AppManagerRunConfigurationFactory theAppManagerRunConfigurationFactory;
    static AppManagerRunAndDebugConfigurationFactory theAppManagerRunAndDebugConfigurationFactory;
}

} // AppManager::Internal
