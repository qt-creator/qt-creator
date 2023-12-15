// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagerrunconfiguration.h"

#include "appmanagerconstants.h"
#include "appmanagertargetinformation.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <remotelinux/remotelinux_constants.h>

#include <utils/algorithm.h>
#include <utils/filesystemwatcher.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AppManager {
namespace Internal {

AppManagerRunConfiguration::AppManagerRunConfiguration(Target *target, Id id)
    : RunConfiguration(target, id)
{
    setDefaultDisplayName(tr("Run on AM Device"));
}

QString AppManagerRunConfiguration::disabledReason() const
{
    if (activeBuildSystem()->isParsing())
        return tr("The project file \"%1\" is currently being parsed.").arg(project()->projectFilePath().toString());
    return QString();
}

class AppManagerRunConfigurationFactoryPrivate
{
public:
    FileSystemWatcher fileSystemWatcher;
};

AppManagerRunConfigurationFactory::AppManagerRunConfigurationFactory()
    : RunConfigurationFactory()
    , d(new AppManagerRunConfigurationFactoryPrivate())
{
    registerRunConfiguration<AppManagerRunConfiguration>(Constants::RUNCONFIGURATION_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
    addSupportedTargetDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
}

AppManagerRunConfigurationFactory::~AppManagerRunConfigurationFactory() = default;

QList<RunConfigurationCreationInfo> AppManagerRunConfigurationFactory::availableCreators(Target *target) const
{
    QObject::connect(&d->fileSystemWatcher, &FileSystemWatcher::fileChanged, target->project(), &Project::displayNameChanged, Qt::UniqueConnection);

    const auto buildTargets = TargetInformation::readFromProject(target);
    const auto result = Utils::transform(buildTargets, [this](const TargetInformation &ti) {
        RunConfigurationCreationInfo rci;
        rci.factory = this;
        rci.buildKey = ti.buildKey;
        rci.displayName = ti.displayName;
        rci.displayNameUniquifier = ti.displayNameUniquifier;
        rci.creationMode = RunConfigurationCreationInfo::AlwaysCreate;
        rci.useTerminal = false;
        if (!this->d->fileSystemWatcher.files().contains(ti.manifest.fileName)) {
            this->d->fileSystemWatcher.addFile(ti.manifest.fileName, Utils::FileSystemWatcher::WatchAllChanges);
        }
        return rci;
    });
    return result;
}

} // namespace Internal
} // namespace AppManager
