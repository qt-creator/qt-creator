// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devcontainerdevice.h"
#include "devcontainerplugin_constants.h"
#include "devcontainerplugintr.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <devcontainer/devcontainer.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/fsengine/fsengine.h>
#include <utils/guardedcallback.h>
#include <utils/icon.h>
#include <utils/infobar.h>
#include <utils/theme/theme.h>

#include <QMessageBox>

using namespace ProjectExplorer;
using namespace Utils;

namespace DevContainer::Internal {

const Icon DEVCONTAINER_ICON({{":/devcontainer/images/container.png", Theme::IconsBaseColor}});

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
        setCombinedIcon(":/devcontainer/images/container.png",
                        ":/devcontainer/images/containerdevice.png");
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
        deviceFactory = std::make_unique<DevContainerDeviceFactory>();

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

        connect(
            Core::EditorManager::instance(),
            &Core::EditorManager::editorCreated,
            this,
            &DevContainerPlugin::onEditorCreated);

        for (auto project : ProjectManager::instance()->projects())
            onProjectAdded(project);

#ifdef WITH_TESTS
        addTestCreator([this]() {
            QObject *tests = createDevcontainerTest();
            QObject::connect(this, SIGNAL(deviceUpDone()), tests, SIGNAL(deviceUpDone()));
            return tests;
        });
#endif
    }

    void onProjectAdded(Project *project);
    void onProjectRemoved(Project *project);

    void onEditorCreated(Core::IEditor *editor, const FilePath &filePath);

    void startDeviceForProject(Project *project, DevContainer::InstanceConfig instanceConfig);

    std::shared_ptr<Device> device(Project *project, const FilePath &configFilePath);

#ifdef WITH_TESTS
signals:
    void deviceUpDone();
#endif

private:
    std::map<Project *, std::vector<std::shared_ptr<Device>>> devicesForProject;
    std::unique_ptr<DevContainerDeviceFactory> deviceFactory;
    QObject guard;
};

Id instantiateInfoBarId(Project *project)
{
    return Id("DevContainer.Instantiate.InfoBar.")
        .withSuffix(project->projectDirectory().toUrlishString());
}

void DevContainerPlugin::onProjectRemoved(Project *project)
{
    const Id infoBarId = instantiateInfoBarId(project);
    InfoBar *infoBar = Core::ICore::popupInfoBar();
    infoBar->removeInfo(infoBarId);

    auto it = devicesForProject.find(project);
    if (it == devicesForProject.end())
        return;

    for (std::shared_ptr<Device> device : it->second) {
        device->down();
        DeviceManager::removeDevice(device->id());
    }

    devicesForProject.erase(it);
}

static FilePaths devContainerFilesForProject(Project *project)
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
    return filtered(
        transform(
            containerFolder.dirEntries(QDir::Dirs | QDir::NoDotAndDotDot),
            [](const FilePath &path) { return path / "devcontainer.json"; })
            << rootDevcontainer << containerFolder / "devcontainer.json",
        &FilePath::isFile);
}

void DevContainerPlugin::onProjectAdded(Project *project)
{
    const FilePaths devContainerFiles = devContainerFilesForProject(project);

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

        const Id infoBarId = instantiateInfoBarId(project);

        InfoBar *infoBar = Core::ICore::popupInfoBar();
        infoBar->removeInfo(infoBarId);

        if (!infoBar->canInfoBeAdded(infoBarId))
            return;

        if (instanceConfigs.size() == 1) {
            InfoBarEntry entry(
                infoBarId,
                Tr::tr(
                    "Found a development container in the project %1, would you like to start it?")
                    .arg(project->displayName()),
                InfoBarEntry::GlobalSuppression::Enabled);

            entry.setTitle(Tr::tr("Configure the development container?"));
            entry.setInfoType(InfoLabel::Information);
            entry.addCustomButton(
                Tr::tr("Yes"),
                [this, project, instanceConfig = instanceConfigs.first(), infoBarId, infoBar] {
                    infoBar->removeInfo(infoBarId);
                    startDeviceForProject(project, instanceConfig);
                },
                Tr::tr("Start the development container."));

            infoBar->addInfo(entry);
            return;
        }

        InfoBarEntry entry(
            infoBarId,
            Tr::tr("Found development containers in the project %1, would you like to start any of them?")
                .arg(project->displayName()),
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

        entry.setTitle(Tr::tr("Configure the development container?"));
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
            Tr::tr("Start the development container."));

        infoBar->addInfo(entry);
    };
}

std::shared_ptr<Device> DevContainerPlugin::device(Project *project, const FilePath &configFilePath)
{
    auto it = devicesForProject.find(project);
    if (it != devicesForProject.end()) {
        return findOrDefault(it->second, [configFilePath](const std::shared_ptr<Device> &device) {
            return device->instanceConfig().configFilePath == configFilePath;
        });
    }
    return nullptr;
}

void DevContainerPlugin::onEditorCreated(Core::IEditor *editor, const FilePath &filePath)
{
    if (filePath.fileName() != "devcontainer.json" && filePath.fileName() != ".devcontainer.json")
        return;

    auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
    if (!textEditor)
        return;

    Project *project = ProjectManager::projectForFile(filePath);
    if (!project)
        return;

    if (!DevContainer::Config::isValidConfigPath(project->rootProjectDirectory(), filePath))
        return;

    TextEditor::TextEditorWidget *textEditorWidget = textEditor->editorWidget();
    if (!textEditorWidget)
        return;

    QAction *restartAction = new QAction(textEditorWidget);
    restartAction->setIcon(DEVCONTAINER_ICON.icon());
    restartAction->setText(Tr::tr("Start or Restart Development Container"));
    restartAction->setToolTip(Tr::tr("Start or stop and restart the development container."));

    const FilePath workspaceFolder = project->rootProjectDirectory();

    connect(restartAction, &QAction::triggered, [this, project, workspaceFolder, filePath]() {
        auto existingDevice = this->device(project, filePath);
        if (existingDevice) {
            existingDevice->restart([](Result<> result) {
                if (!result) {
                    QMessageBox box(Core::ICore::dialogParent());
                    box.setWindowTitle(Tr::tr("Development Container Error"));
                    box.setIcon(QMessageBox::Critical);
                    box.setText(result.error());
                    box.exec();
                }
            });
            return;
        }

        DevContainer::InstanceConfig instanceConfig{
            .dockerCli = "docker",
            .workspaceFolder = workspaceFolder,
            .configFilePath = filePath,
            .mounts = {},
        };

        startDeviceForProject(project, instanceConfig);
    });

    textEditorWidget
        ->insertExtraToolBarAction(TextEditor::TextEditorWidget::Side::Left, restartAction);
}

void DevContainerPlugin::startDeviceForProject(
    Project *project, DevContainer::InstanceConfig instanceConfig)
{
    std::shared_ptr<QString> log = std::make_shared<QString>();
    instanceConfig.logFunction = [log](const QString &message) {
        *log += message + '\n';
        Core::MessageManager::writeSilently(message);
    };

    std::shared_ptr<Device> device = std::make_shared<DevContainer::Device>(project);
    device->setDisplayName(Tr::tr("Development Container for %1").arg(project->displayName()));
    DeviceManager::addDevice(device);

    const auto onDone = guardedCallback(&guard, [this, project, log, device](Result<> result) {
        if (result) {
            auto it = devicesForProject.find(project);
            if (it == devicesForProject.end())
                devicesForProject.insert({project, {device}});
            else
                it->second.push_back(device);

            log->clear();

#ifdef WITH_TESTS
            emit deviceUpDone();
#endif
            return;
        }

        DeviceManager::removeDevice(device->id());

        QMessageBox box(Core::ICore::dialogParent());
        box.setWindowTitle(Tr::tr("Development Container Error"));
        box.setIcon(QMessageBox::Critical);
        box.setText(result.error());
        box.setDetailedText(*log);
        box.exec();

#ifdef WITH_TESTS
        emit deviceUpDone();
#endif

        log->clear();
    });

    device->up(instanceConfig, onDone);
}

} // namespace DevContainer::Internal

#include <devcontainerplugin.moc>
