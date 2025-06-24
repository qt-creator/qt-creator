// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devcontainerdevice.h"
#include "devcontainerplugin_constants.h"
#include "devcontainerplugintr.h"

#include <coreplugin/icore.h>

#include <devcontainer/devcontainer.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/infobar.h>

#include <QMessageBox>

using namespace ProjectExplorer;

namespace DevContainer::Internal {

struct Private
{
    std::map<ProjectExplorer::Project *, std::shared_ptr<Device>> devices;
};

Q_GLOBAL_STATIC(Private, devContainerPluginPrivate)

static Utils::Result<std::shared_ptr<DevContainer::Device>> startDeviceForProject(
    const Utils::FilePath &path,
    ProjectExplorer::Project *project,
    const DevContainer::InstanceConfig &instanceConfig)
{
    auto device = std::make_shared<DevContainer::Device>();
    ProjectExplorer::DeviceManager::addDevice(device);
    Utils::Result<> result = device->up(path, instanceConfig);
    if (!result) {
        ProjectExplorer::DeviceManager::removeDevice(device->id());
        QMessageBox::critical(
            Core::ICore::dialogParent(),
            Tr::tr("DevContainer Error"),
            Tr::tr("Failed to start DevContainer: %1").arg(result.error()));
        return Utils::ResultError(Tr::tr("Failed to start DevContainer for project '%1': %2")
                                      .arg(project->displayName(), result.error()));
    }

    return device;
}

static void onProjectAdded(ProjectExplorer::Project *project)
{
    const Utils::FilePath path = project->projectDirectory() / ".devcontainer"
                                 / "devcontainer.json";
    if (path.exists()) {
        DevContainer::InstanceConfig instanceConfig = {
            .dockerCli = "docker",
            .dockerComposeCli = "docker-compose",
            .workspaceFolder = project->projectDirectory(),
            .configFilePath = path,
            .mounts = {},
        };

        const Utils::Id infoBarId = Utils::Id::fromString(
            QString("DevContainer.Instantiate.InfoBar." + instanceConfig.devContainerId()));

        Utils::InfoBarEntry entry(
            infoBarId,
            Tr::tr("Found devcontainers in project, would you like to start them?"),
            Utils::InfoBarEntry::GlobalSuppression::Enabled);

        entry.setTitle(Tr::tr("Configure devcontainer?"));
        entry.setInfoType(Utils::InfoLabel::Information);
        entry.addCustomButton(
            Tr::tr("Yes"),
            [project, path, instanceConfig, infoBarId] {
                auto device = startDeviceForProject(path, project, instanceConfig);
                if (device) {
                    devContainerPluginPrivate->devices.insert({project, *device});
                    Core::ICore::infoBar()->removeInfo(infoBarId);
                }
            },
            Tr::tr("Start DevContainer"));

        Core::ICore::infoBar()->addInfo(entry);
    };
}

static void onProjectRemoved(ProjectExplorer::Project *project)
{
    auto it = devContainerPluginPrivate->devices.find(project);
    if (it == devContainerPluginPrivate->devices.end())
        return;
    devContainerPluginPrivate->devices.erase(it);
}

class DevContainerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "DevContainerPlugin.json")

public:
    DevContainerPlugin() {}

    void initialize() final
    {
        connect(ProjectManager::instance(), &ProjectManager::projectAdded, this, &onProjectAdded);
        connect(ProjectManager::instance(), &ProjectManager::projectRemoved, this, &onProjectRemoved);

        for (auto project : ProjectManager::instance()->projects())
            onProjectAdded(project);
    }
    void extensionsInitialized() final {}
};

} // namespace DevContainer::Internal

#include <devcontainerplugin.moc>
