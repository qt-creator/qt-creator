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
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
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
        setExecutionTypeId(ProjectExplorer::Constants::STDPROCESS_EXECUTION_TYPE_ID);
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
            &ProjectManager::aboutToRemoveProject,
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

    void startDeviceForProject(Project *project, DevContainer::InstanceConfig instanceConfig);

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
    /*
    Possible locations:
        .devcontainer/devcontainer.json
        .devcontainer.json
        .devcontainer/<folder>/devcontainer.json (where <folder> is a sub-folder, one level deep)
*/
    const FilePath containerFolder = project->projectDirectory() / ".devcontainer";
    const FilePath rootDevcontainer = project->projectDirectory() / ".devcontainer.json";

    const FilePaths paths{rootDevcontainer, containerFolder / ".devcontainer"};
    const FilePaths devContainerFiles = filtered(
        transform(
            containerFolder.dirEntries(QDir::Dirs | QDir::NoDotAndDotDot),
            [](const FilePath &path) { return path / "devcontainer.json"; })
            << rootDevcontainer << containerFolder / "devcontainer.json",
        &FilePath::isFile);

    if (!devContainerFiles.isEmpty()) {
        const QList<DevContainer::InstanceConfig> instanceConfigs
            = transform(devContainerFiles, [project](const FilePath &path) {
                  return DevContainer::InstanceConfig{
                      .dockerCli = "docker",
                      .workspaceFolder = project->projectDirectory(),
                      .configFilePath = path,
                      .mounts = {},
                  };
              });

        if (instanceConfigs.isEmpty())
            return;

        const Id infoBarId = Id("DevContainer.Instantiate.InfoBar.")
                                 .withSuffix(project->projectDirectory().toUrlishString());

        if (instanceConfigs.size() == 1) {
            InfoBarEntry entry(
                infoBarId,
                Tr::tr("Found Devcontainer in project, would you like to start it?"),
                InfoBarEntry::GlobalSuppression::Enabled);

            InfoBar *infoBar = Core::ICore::popupInfoBar();
            entry.setTitle(Tr::tr("Configure devcontainer?"));
            entry.setInfoType(InfoLabel::Information);
            entry.addCustomButton(
                Tr::tr("Yes"),
                [this, project, instanceConfig = instanceConfigs.first(), infoBarId, infoBar] {
                    infoBar->removeInfo(infoBarId);
                    startDeviceForProject(project, instanceConfig);
                },
                Tr::tr("Start DevContainer"));

            infoBar->addInfo(entry);
            return;
        }

        InfoBarEntry entry(
            infoBarId,
            Tr::tr("Found Devcontainers in project, would you like to start one of them?"),
            InfoBarEntry::GlobalSuppression::Enabled);

        const auto toComboInfo = [](const DevContainer::InstanceConfig &config) {
            Result<std::unique_ptr<Instance>> instance = DevContainer::Instance::fromFile(config);
            if (!instance) {
                return InfoBarEntry::ComboInfo{
                    .displayText = instance.error(),
                    .data = QVariant(),
                };
            }
            const QString relativeDisplayPath
                = config.configFilePath.relativeChildPath(config.workspaceFolder).toUrlishString();
            QString displayName = relativeDisplayPath;
            if ((*instance)->config().common.name) {
                displayName = (*(*instance)->config().common.name) + "(" + relativeDisplayPath
                              + ")";
            }

            return InfoBarEntry::ComboInfo{
                .displayText = displayName,
                .data = QVariant::fromValue(config),
            };
        };

        const QList<InfoBarEntry::ComboInfo> comboInfos
            = transform(instanceConfigs, toComboInfo)
              << InfoBarEntry::ComboInfo{"All", QVariant::fromValue(InstanceConfig())};

        std::shared_ptr<InstanceConfig> selectedConfig = std::make_shared<InstanceConfig>(instanceConfigs.first());

        InfoBar *infoBar = Core::ICore::popupInfoBar();
        entry.setTitle(Tr::tr("Configure devcontainer?"));
        entry.setInfoType(InfoLabel::Information);
        entry.setComboInfo(comboInfos, [selectedConfig](const InfoBarEntry::ComboInfo &comboInfo) {
            *selectedConfig = comboInfo.data.value<InstanceConfig>();
        });

        entry.addCustomButton(
            Tr::tr("Yes"),
            [selectedConfig, this, project, instanceConfigs, infoBarId, infoBar] {
                infoBar->removeInfo(infoBarId);

                if (selectedConfig->configFilePath.isEmpty()) {
                    for (const auto &instanceConfig : instanceConfigs)
                        startDeviceForProject(project, instanceConfig);
                    return;
                }

                startDeviceForProject(project, *selectedConfig);
            },
            Tr::tr("Start DevContainer"));

        infoBar->addInfo(entry);
    };
}

void DevContainerPlugin::startDeviceForProject(
    Project *project, DevContainer::InstanceConfig instanceConfig)
{
    std::shared_ptr<QString> log = std::make_shared<QString>();
    instanceConfig.logFunction = [log](const QString &message) {
        *log += message + '\n';
        Core::MessageManager::writeSilently(message);
    };

    std::shared_ptr<Device> device = std::make_shared<DevContainer::Device>();
    device->setDisplayName(Tr::tr("DevContainer for %1").arg(project->displayName()));
    DeviceManager::addDevice(device);

    const auto onDone = Utils::guardedCallback(&guard, [this, project, log, device](Result<> result) {
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
    });

    Result<> result = device->up(instanceConfig, onDone);

    if (!result) {
        QMessageBox::critical(
            Core::ICore::dialogParent(), Tr::tr("DevContainer Error"), result.error());
        DeviceManager::removeDevice(device->id());
    }
}

} // namespace DevContainer::Internal

#include <devcontainerplugin.moc>
