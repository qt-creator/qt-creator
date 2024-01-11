// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagerrunconfiguration.h"

#include "appmanagerconstants.h"
#include "appmanagertargetinformation.h"
#include "appmanagertr.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <remotelinux/remotelinux_constants.h>

#include <utils/algorithm.h>
#include <utils/filesystemwatcher.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AppManager::Internal {

class AppManagerRunConfiguration final : public RunConfiguration
{
public:
    AppManagerRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        setDefaultDisplayName(Tr::tr("Run on AM Device"));
    }

    QString disabledReason() const override
    {
        if (activeBuildSystem()->isParsing()) {
            return Tr::tr("The project file \"%1\" is currently being parsed.")
                .arg(project()->projectFilePath().toString());
        }
        return QString();
    }
};

class AppManagerRunConfigurationFactory final : public RunConfigurationFactory
{
public:
    AppManagerRunConfigurationFactory()
    {
        registerRunConfiguration<AppManagerRunConfiguration>(Constants::RUNCONFIGURATION_ID);
        addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
        addSupportedTargetDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
    }

    QList<RunConfigurationCreationInfo> availableCreators(Target *target) const final
    {
        QObject::connect(&m_fileSystemWatcher, &FileSystemWatcher::fileChanged,
                         target->project(), &Project::displayNameChanged,
                         Qt::UniqueConnection);

        const auto buildTargets = TargetInformation::readFromProject(target);
        auto result = Utils::transform(buildTargets, [this, target](const TargetInformation &ti) {

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

void setupAppManagerRunConfiguration()
{
    static AppManagerRunConfigurationFactory theAppManagerRunConfigurationFactory;
}

} // AppManager::Internal
