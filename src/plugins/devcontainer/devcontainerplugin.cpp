// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devcontainerdevice.h"
#include "devcontainerplugin_constants.h"
#include "devcontainerplugintr.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <devcontainer/devcontainer.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/fsengine/fsengine.h>
#include <utils/guardedcallback.h>
#include <utils/infobar.h>

#include <QMessageBox>

using namespace ProjectExplorer;
using namespace Utils;

namespace DevContainer::Internal {

#ifdef WITH_TESTS
QObject *createDevcontainerTest();
#endif

class DevContainerDeviceFactory final : public IDeviceFactory
{
public:
    DevContainerDeviceFactory()
        : IDeviceFactory(Constants::DEVCONTAINER_DEVICE_TYPE)
    {
        setDisplayName(Tr::tr("Development Container Device"));
        setIcon(QIcon());
    }
};

class DevContainerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "DevContainerPlugin.json")

public:
    DevContainerPlugin()
    {
        FSEngine::registerDeviceScheme(Constants::DEVCONTAINER_FS_SCHEME);
    }

    ~DevContainerPlugin() final
    {
        FSEngine::unregisterDeviceScheme(Constants::DEVCONTAINER_FS_SCHEME);
    }

    void initialize() final
    {
        connect(
            ProjectManager::instance(),
            &ProjectManager::projectAdded,
            this,
            &DevContainerPlugin::onProjectAdded);
        connect(
            ProjectManager::instance(),
            &ProjectManager::projectRemoved,
            this,
            &DevContainerPlugin::onProjectRemoved);

        for (auto project : ProjectManager::instance()->projects())
            onProjectAdded(project);

#ifdef WITH_TESTS
        addTestCreator(&createDevcontainerTest);
#endif
    }
    void onProjectAdded(Project *project);
    void onProjectRemoved(Project *project);

    void startDeviceForProject(
        const FilePath &path, Project *project, DevContainer::InstanceConfig instanceConfig);

#ifdef WITH_TESTS
signals:
    void deviceUpDone();
#endif

private:
    std::map<Project *, std::shared_ptr<Device>> devices;
    DevContainerDeviceFactory deviceFactory;
    QObject guard;
};

void DevContainerPlugin::onProjectRemoved(Project *project)
{
    auto it = devices.find(project);
    if (it == devices.end())
        return;

    for (auto device : devices)
        device.second->down();

    DeviceManager::removeDevice(it->second->id());
    devices.erase(it);
}

void DevContainerPlugin::onProjectAdded(Project *project)
{
    const FilePath path = project->projectDirectory() / ".devcontainer" / "devcontainer.json";
    if (path.exists()) {
        DevContainer::InstanceConfig instanceConfig = {
            .dockerCli = "docker",
            .workspaceFolder = project->projectDirectory(),
            .configFilePath = path,
            .mounts = {},
        };

        const Id infoBarId = Id("DevContainer.Instantiate.InfoBar.")
            .withSuffix(instanceConfig.devContainerId());

        InfoBarEntry entry(
            infoBarId,
            Tr::tr("Found devcontainers in project, would you like to start them?"),
            InfoBarEntry::GlobalSuppression::Enabled);

        entry.setTitle(Tr::tr("Configure devcontainer?"));
        entry.setInfoType(InfoLabel::Information);
        entry.addCustomButton(
            Tr::tr("Yes"),
            [this, project, path, instanceConfig, infoBarId] {
                Core::ICore::infoBar()->removeInfo(infoBarId);
                startDeviceForProject(path, project, instanceConfig);
            },
            Tr::tr("Start DevContainer"));

        Core::ICore::infoBar()->addInfo(entry);
    };
}

void DevContainerPlugin::startDeviceForProject(
    const FilePath &path,
    Project *project,
    DevContainer::InstanceConfig instanceConfig)
{
    std::shared_ptr<QString> log = std::make_shared<QString>();
    instanceConfig.logFunction = [log](const QString &message) {
        *log += message + '\n';
        Core::MessageManager::writeSilently(message);
    };

    std::shared_ptr<Device> device = std::make_shared<DevContainer::Device>();
    device->setDisplayName(Tr::tr("DevContainer for %1").arg(project->displayName()));
    DeviceManager::addDevice(device);
    Result<> result = device->up(
        path,
        instanceConfig,
        Utils::guardedCallback(&guard, [this, project, log, device](Result<> result) {
            if (result) {
                devices.insert({project, device});
                log->clear();
#ifdef WITH_TESTS
                emit deviceUpDone();
#endif
                return;
            }

            DeviceManager::removeDevice(device->id());

            QMessageBox box(Core::ICore::dialogParent());
            box.setWindowTitle(Tr::tr("DevContainer Error"));
            box.setIcon(QMessageBox::Critical);
            box.setText(result.error());
            box.setDetailedText(*log);
            box.exec();

#ifdef WITH_TESTS
            emit deviceUpDone();
#endif

            log->clear();
        }));

    if (!result) {
        QMessageBox::critical(
            Core::ICore::dialogParent(), Tr::tr("DevContainer Error"), result.error());
        DeviceManager::removeDevice(device->id());
    }
}

} // namespace DevContainer::Internal

#include <devcontainerplugin.moc>
