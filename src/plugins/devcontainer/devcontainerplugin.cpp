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

namespace DevContainer::Internal {

class DevContainerDeviceFactory;

class Private : public QObject
{
    Q_OBJECT

public:
    void init()
    {
        deviceFactory = std::make_unique<DevContainerDeviceFactory>();

        connect(
            ProjectManager::instance(),
            &ProjectManager::projectAdded,
            this,
            &Private::onProjectAdded);
        connect(
            ProjectManager::instance(),
            &ProjectManager::projectRemoved,
            this,
            &Private::onProjectRemoved);

        for (auto project : ProjectManager::instance()->projects())
            onProjectAdded(project);
    }

public slots:
    void onProjectAdded(ProjectExplorer::Project *project);
    void onProjectRemoved(ProjectExplorer::Project *project);

private:
    static void startDeviceForProject(
        Private *d,
        const Utils::FilePath &path,
        ProjectExplorer::Project *project,
        DevContainer::InstanceConfig instanceConfig);

#ifdef WITH_TESTS
signals:
    void deviceUpDone();
#endif

private:
    std::map<ProjectExplorer::Project *, std::shared_ptr<Device>> devices;
    std::unique_ptr<DevContainerDeviceFactory> deviceFactory;
};

void Private::onProjectRemoved(ProjectExplorer::Project *project)
{
    auto it = devices.find(project);
    if (it == devices.end())
        return;

    for (auto device : devices)
        device.second->down();

    DeviceManager::removeDevice(it->second->id());
    devices.erase(it);
}

void Private::onProjectAdded(ProjectExplorer::Project *project)
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
            [this, project, path, instanceConfig, infoBarId] {
                Core::ICore::infoBar()->removeInfo(infoBarId);
                startDeviceForProject(this, path, project, instanceConfig);
            },
            Tr::tr("Start DevContainer"));

        Core::ICore::infoBar()->addInfo(entry);
    };
}

void Private::startDeviceForProject(
    Private *d,
    const Utils::FilePath &path,
    ProjectExplorer::Project *project,
    DevContainer::InstanceConfig instanceConfig)
{
    std::shared_ptr<QString> log = std::make_shared<QString>();
    instanceConfig.logFunction = [log](const QString &message) {
        *log += message + '\n';
        Core::MessageManager::writeSilently(message);
    };

    std::shared_ptr<Device> device = std::make_shared<DevContainer::Device>();
    device->setDisplayName(Tr::tr("DevContainer for %1").arg(project->displayName()));
    ProjectExplorer::DeviceManager::addDevice(device);
    Utils::Result<> result = device->up(
        path,
        instanceConfig,
        Utils::guardedCallback(d, [d, project, log, device](Utils::Result<> result) {
            if (result) {
                d->devices.insert({project, device});
                log->clear();
#ifdef WITH_TESTS
                emit d->deviceUpDone();
#endif
                return;
            }

            ProjectExplorer::DeviceManager::removeDevice(device->id());

            QMessageBox box(Core::ICore::dialogParent());
            box.setWindowTitle(Tr::tr("DevContainer Error"));
            box.setIcon(QMessageBox::Critical);
            box.setText(result.error());
            box.setDetailedText(*log);
            box.exec();

#ifdef WITH_TESTS
            emit d->deviceUpDone();
#endif

            log->clear();
        }));

    if (!result) {
        QMessageBox::critical(
            Core::ICore::dialogParent(), Tr::tr("DevContainer Error"), result.error());
        ProjectExplorer::DeviceManager::removeDevice(device->id());
    }
}

#ifdef WITH_TESTS
QObject *createDevcontainerTest();
#endif

class DevContainerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "DevContainerPlugin.json")

public:
    DevContainerPlugin()
        : d(std::make_unique<Private>())
    {
        Utils::FSEngine::registerDeviceScheme(Constants::DEVCONTAINER_FS_SCHEME);
    }
    ~DevContainerPlugin() final
    {
        Utils::FSEngine::unregisterDeviceScheme(Constants::DEVCONTAINER_FS_SCHEME);
    }

    std::unique_ptr<DevContainer::Internal::Private> d;

    void initialize() final
    {
        d->init();

#ifdef WITH_TESTS
        addTestCreator([this]() {
            QObject *tests = createDevcontainerTest();
            QObject::connect(d.get(), SIGNAL(deviceUpDone()), tests, SIGNAL(deviceUpDone()));
            return tests;
        });
#endif
    }
    void extensionsInitialized() final {}
};

class DevContainerDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    DevContainerDeviceFactory()
        : IDeviceFactory(Constants::DEVCONTAINER_DEVICE_TYPE)
    {
        setDisplayName(Tr::tr("Development Container Device"));
        setIcon(QIcon());
    }
};

} // namespace DevContainer::Internal

#include <devcontainerplugin.moc>
