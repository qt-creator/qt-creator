// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devcontainerdevice.h"

#include "devcontainerplugin_constants.h"
#include "devcontainerplugintr.h"

#include <client/bridgedfileaccess.h>
#include <client/cmdbridgeclient.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <devcontainer/devcontainerconfig.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitaspect.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <QtTaskTree/qconditional.h>

#include <utils/algorithm.h>
#include <utils/fsengine/fsengine.h>
#include <utils/infobar.h>

#include <QtTaskTree/qconditional.h>

#include <QLoggingCategory>
#include <QMessageBox>
#include <QProgressDialog>

Q_LOGGING_CATEGORY(devContainerDeviceLog, "qtc.devcontainer.device", QtWarningMsg)

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace DevContainer {

Device::Device()
{
    setDisplayType(Tr::tr("Development Container"));
    setOsType(OsTypeLinux);
    setupId(IDevice::AutoDetected, Id::generate());
    setType(Constants::DEVCONTAINER_DEVICE_TYPE);
    setMachineType(IDevice::Hardware);
    setFileAccessFactory([this] { return m_fileAccess; });
}

Device::~Device() {} // Necessary for forward declared unique_ptr

IDeviceWidget *Device::createWidget()
{
    return nullptr;
}

Result<> Device::handlesFile(const FilePath &filePath) const
{
    const FilePath root = rootPath();
    if (filePath.scheme() == root.scheme() && filePath.host() == root.host())
        return ResultOk;
    return IDevice::handlesFile(filePath);
}

class ProgressPromise : public QPromise<void>
{
public:
    struct Progress
    {
        int value;
        int max;
        Progress operator+(const Progress &other) const
        {
            return {value + other.value, max + other.max};
        }
    };

    ProgressPromise(QTaskTree &tree, const QString title, Id id)
    {
        Core::FutureProgress *futureProgress = Core::ProgressManager::addTask(future(), title, id);
        QObject::connect(futureProgress, &Core::FutureProgress::canceled, &tree, &QTaskTree::cancel);
        QObject::connect(futureProgress, &Core::FutureProgress::clicked, &Core::MessageManager::popup);

        addSource(tree);
        start();
    }

    void addSource(QTaskTree &taskTree)
    {
        sources = [tt = QPointer<QTaskTree>(&taskTree),
                   max = taskTree.progressMaximum(),
                   lastSource = sources]() -> Progress {
            Progress last = lastSource();
            if (tt)
                return last + Progress{tt->progressValue(), max};
            return last + Progress{max, max};
        };
        update();
        QObject::connect(&taskTree, &QTaskTree::progressValueChanged, &taskTree, [this] {
            update();
        });
    }

    void update()
    {
        Progress total = sources();
        setProgressRange(0, total.max);
        setProgressValue(total.value);
    }

private:
    using Source = std::function<Progress()>;
    Source sources = []() -> Progress { return {0, 0}; };
};

class FileAccess : public CmdBridge::FileAccess
{
public:
    FileAccess(const FilePath &workspaceFolder, const FilePath &workspaceFolderMountPoint)
        : m_workspaceFolder(workspaceFolder)
        , m_workspaceFolderMountPoint(workspaceFolderMountPoint)
    {}

    QString mapToDevicePath(const QString &hostPath) const override
    {
        if (hostPath.startsWith(m_workspaceFolder.path())) {
            return (m_workspaceFolderMountPoint / hostPath.mid(m_workspaceFolder.path().size()))
                .path();
        }
        return hostPath;
    }

    FilePath workspaceFolderMountPoint() const { return m_workspaceFolderMountPoint; }
    FilePath workspaceFolder() const { return m_workspaceFolder; }

private:
    const FilePath m_workspaceFolder;
    const FilePath m_workspaceFolderMountPoint;
};

static auto setupProgress(const Storage<ProgressPtr> &progressStorage, const QString &title, Id id)
{
    return [progressStorage, title, id](QTaskTree &taskTree) {
        taskTree.onStorageSetup(progressStorage, [&taskTree, title, id](ProgressPtr &promise) {
            promise.reset(new ProgressPromise(taskTree, title, id));
        });
        taskTree.onStorageDone(progressStorage, [](const ProgressPtr &promise) {
            promise->finish();
        });
    };
}

void Device::onConfigChanged()
{
    const Id infoBarId = Id("DevContainer.Reload.InfoBar.")
                             .withSuffix(m_instanceConfig.workspaceFolder.toUrlishString());

    InfoBarEntry entry{infoBarId, Tr::tr("Rebuild the development container?")};

    InfoBar *infoBar = Core::ICore::popupInfoBar();
    entry.setTitle(Tr::tr("The Development Container Configuration Changed"));
    entry.setInfoType(InfoLabel::Information);

    entry.addCustomButton(
        Tr::tr("Rebuild"),
        [this, infoBarId, infoBar] {
            infoBar->removeInfo(infoBarId);

            auto oldLogFunction = m_instanceConfig.logFunction;
            std::shared_ptr<QString> log = std::make_shared<QString>();
            m_instanceConfig.logFunction = [oldLogFunction, log](const QString &message) {
                *log += message + '\n';
                oldLogFunction(message);
            };

            restart([this, log, oldLogFunction](Result<> result) {
                m_instanceConfig.logFunction = oldLogFunction;

                if (!result) {
                    QMessageBox box(Core::ICore::dialogParent());
                    box.setWindowTitle(Tr::tr("Development Container Error"));
                    box.setIcon(QMessageBox::Critical);
                    box.setText(result.error());
                    box.setDetailedText(*log);
                    box.exec();
                }
            });
        },
        Tr::tr("Rebuild and restart the development container."));

    infoBar->addInfo(entry);
}

Group Device::upRecipe(InstanceConfig instanceConfig, Storage<ProgressPtr> progressStorage)
{
    QTC_ASSERT(m_project, return {});

    struct Options
    {
        bool mountLibExec = true;
        bool copyCmdBridge = false;
        QString libExecMountPoint = "/devcontainer/libexec";
        QString workspaceFolderMountPoint;
        bool runProcessesInTerminal = false;
        bool autoDetectKits = true;
    };

    const Storage<std::shared_ptr<Instance>> instance;
    const Storage<Options> options;

    auto runningInstance = std::make_shared<DevContainer::RunningInstanceData>();

    const auto init = [instanceConfig, this]() {
        m_instanceConfig = instanceConfig;
        m_processInterfaceCreator = nullptr;
        m_fileAccess.reset();
        m_systemEnvironment.reset();
        m_downRecipe.reset();
        m_dockerFileWatcher.reset();
        m_devContainerJsonWatcher.reset();

        DeviceManager::addDevice(shared_from_this());

        instanceConfig.configFilePath.watch()
            .and_then([this](std::unique_ptr<Utils::FilePathWatcher> jsonWatcher) {
                connect(
                    jsonWatcher.get(),
                    &FilePathWatcher::pathChanged,
                    this,
                    &Device::onConfigChanged);
                m_devContainerJsonWatcher = std::move(jsonWatcher);
                return ResultOk;
            })
            .or_else(m_instanceConfig.logFunction);
    };

    const auto loadConfig = [instanceConfig, instance, options, this]() -> DoneResult {
        const Result<> result = [&]() -> Result<> {
            InstanceConfig modifiedConfig = instanceConfig;
            Result<Config> config = Instance::configFromFile(modifiedConfig);
            if (!config)
                return ResultError(config.error());

            if (!config->containerConfig) {
                return ResultError(
                    Tr::tr(
                        "The configuration does not contain a \"build\", \"image\" or "
                        "\"dockerComposeFile\" entry."));
            }

            options->mountLibExec
                = DevContainer::customization(*config, "qt-creator/device/mount-libexec")
                      .toBool(true);

            options->libExecMountPoint
                = DevContainer::customization(*config, "qt-creator/device/libexec-mount-point")
                      .toString("/devcontainer/libexec");

            options->copyCmdBridge
                = DevContainer::customization(*config, "qt-creator/device/copy-cmd-bridge")
                      .toBool(false);

            options->runProcessesInTerminal
                = DevContainer::customization(*config, "qt-creator/device/run-processes-in-terminal")
                      .toBool(false);

            options->autoDetectKits
                = DevContainer::customization(*config, "qt-creator/auto-detect-kits").toBool(true);

            modifiedConfig.runProcessesInTerminal = options->runProcessesInTerminal;

            if (options->mountLibExec) {
                modifiedConfig.mounts.push_back(
                    Mount{
                        .type = MountType::Bind,
                        .source = Core::ICore::libexecPath().absoluteFilePath().path(),
                        .target = options->libExecMountPoint,
                    });
            }

            if (config->common.name)
                setDisplayName(*config->common.name);

            options->workspaceFolderMountPoint = std::visit(
                [](const auto &containerConfig) { return containerConfig.workspaceFolder; },
                *config->containerConfig);

            if (config->containerConfig) {
                if (std::holds_alternative<DockerfileContainer>(*config->containerConfig)) {
                    const auto &dockerfileContainer = std::get<DockerfileContainer>(
                        *config->containerConfig);

                    const FilePath configFileDir = instanceConfig.configFilePath.parentDir();
                    const FilePath dockerFile = configFileDir.resolvePath(
                        dockerfileContainer.dockerfile);

                    dockerFile.watch()
                        .and_then([this](std::unique_ptr<Utils::FilePathWatcher> dockerWatcher) {
                            connect(
                                dockerWatcher.get(),
                                &FilePathWatcher::pathChanged,
                                this,
                                &Device::onConfigChanged);
                            m_dockerFileWatcher = std::move(dockerWatcher);
                            return ResultOk;
                        })
                        .or_else(m_instanceConfig.logFunction);
                }
            }

            Result<std::unique_ptr<Instance>> instanceResult
                = DevContainer::Instance::fromConfig(*config, modifiedConfig);
            if (!instanceResult)
                return ResultError(instanceResult.error());

            *instance = std::move(*instanceResult);

            return ResultOk;
        }();

        if (!result) {
            instanceConfig.logFunction(
                Tr::tr("Cannot load the development container configuration: %1")
                    .arg(result.error()));
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    const auto setupProcessInterfaceCreator = [this, instance, runningInstance] {
        m_processInterfaceCreator = [inst = *instance, runningInstance] {
            return inst->createProcessInterface(runningInstance);
        };
        m_systemEnvironment = runningInstance->remoteEnvironment;
    };

    const auto setupCmdBridge =
        [this, instanceConfig, runningInstance, instance, options]() -> DoneResult {
        const auto result = [&]() -> Result<> {
            Result<FilePath> cmdBridgePath = CmdBridge::Client::getCmdBridgePath(
                runningInstance->osType, runningInstance->osArch, Core::ICore::libexecPath());

            if (!cmdBridgePath)
                return ResultError(cmdBridgePath.error());

            auto fileAccess = std::make_unique<FileAccess>(
                instanceConfig.workspaceFolder,
                FilePath::fromUserInput(options->workspaceFolderMountPoint));

            Result<> initResult = [&]() -> Result<> {
                if (options->copyCmdBridge) {
                    const CmdBridge::FileAccess::DeployResult res = fileAccess->deployAndInit(
                        Core::ICore::libexecPath(), rootPath(), runningInstance->remoteEnvironment);
                    if (!res)
                        return ResultError(res.error().message);
                    return ResultOk;
                } else {
                    const auto bridgeInContainerPath
                        = rootPath().withNewPath(options->libExecMountPoint)
                          / cmdBridgePath->relativeChildPath(Core::ICore::libexecPath()).path();

                    return fileAccess
                        ->init(bridgeInContainerPath, runningInstance->remoteEnvironment, false);
                }
            }();

            if (!initResult)
                return initResult;

            m_fileAccess = std::move(fileAccess);
            FSEngine::invalidateFileInfoCache();
            return ResultOk;
        }();

        if (!result) {
            instanceConfig.logFunction(Tr::tr("Cannot set up Command Bridge: %1").arg(result.error()));
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    const auto startDeviceTree = [instance, instanceConfig, runningInstance, progressStorage](
                                     QTaskTree &taskTree) -> SetupResult {
        const Result<Group> devcontainerRecipe = (*instance)->upRecipe(runningInstance);
        if (!devcontainerRecipe) {
            instanceConfig.logFunction(
                Tr::tr("Cannot create the development container recipe: %1")
                    .arg(devcontainerRecipe.error()));
            return SetupResult::StopWithError;
        }
        taskTree.setRecipe(std::move(*devcontainerRecipe));
        progressStorage->get()->addSource(taskTree);
        return SetupResult::Continue;
    };

    const auto onDeviceStarted = [this, instance](DoneWith doneWith) -> DoneResult {
        if (doneWith == DoneWith::Error)
            return DoneResult::Error;

        auto downRecipe = (*instance)->downRecipe(false);
        auto forceDownRecipe = (*instance)->downRecipe(true);
        if (!downRecipe || !forceDownRecipe) {
            qCWarning(devContainerDeviceLog)
                << "Cannot create down recipe for the development container instance:"
                << downRecipe.error();
            return DoneResult::Error;
        }

        DeviceManager::setDeviceState(id(), DeviceState::DeviceReadyToUse);

        m_downRecipe = std::move(*downRecipe);
        m_forceDownRecipe = std::move(*forceDownRecipe);
        return DoneResult::Success;
    };

    const auto setupManualKits = [this, instance, instanceConfig](QTaskTree &tree) {
        QJsonArray kits = customization((*instance)->config(), "qt-creator/kits").toArray();

        GroupItems steps;

        for (const QJsonValue &kitValue : kits) {
            if (!kitValue.isObject())
                continue;

            const QJsonObject kitObject = kitValue.toObject();

            Kit *kit = KitManager::registerKit([this](Kit *kit) {
                kit->setDetectionSource({DetectionSource::Temporary, id().toString()});
                kit->setUnexpandedDisplayName("%{Device:Name}");

                RunDeviceTypeKitAspect::setDeviceTypeId(kit, type());
                RunDeviceKitAspect::setDevice(kit, shared_from_this());
                BuildDeviceTypeKitAspect::setDeviceTypeId(kit, type());
                BuildDeviceKitAspect::setDevice(kit, shared_from_this());

                kit->setSticky(BuildDeviceKitAspect::id(), true);
                kit->setSticky(BuildDeviceTypeKitAspect::id(), true);
            });

            for (auto it = kitObject.constBegin(); it != kitObject.constEnd(); ++it) {
                if (it.key() == "name") {
                    kit->setUnexpandedDisplayName(it.value().toString("%{Device:Name}"));
                    continue;
                }

                const auto factory = findOrDefault(
                    KitAspectFactory::kitAspectFactories(),
                    [key = Id::fromString(it.key())](KitAspectFactory *factory) {
                        if (factory->id() == key || factory->jsonKeys().contains(key))
                            return true;
                        return false;
                    });

                if (!factory) {
                    instanceConfig.logFunction(
                        Tr::tr("Unknown kit aspect factory: %1").arg(it.key()));
                    continue;
                }

                auto executableItem = factory->createAspectFromJson(
                    {DetectionSource::Temporary, id().toString()},
                    rootPath(),
                    kit,
                    it.value(),
                    instanceConfig.logFunction);

                if (!executableItem) {
                    instanceConfig.logFunction(
                        Tr::tr("Cannot create kit aspect %1: %2")
                            .arg(it.key())
                            .arg(executableItem.error()));
                    continue;
                }

                steps.append(*executableItem);
            }
        }

        tree.setRecipe(steps);
    };

    const auto autoDetectKitsEnabled = [options] { return options->autoDetectKits; };

    const auto restoreVanishedTargets = [this]() {
        for (QMap<Utils::Key, QVariant> target : m_project->vanishedTargets()) {
            const QString name = target.value(Target::displayNameKey()).toString();
            auto kit = Utils::findOrDefault(KitManager::kits(), [this, &name](Kit *k) {
                if (BuildDeviceKitAspect::device(k) != shared_from_this())
                    return false;
                return k->displayName() == name;
            });

            if (kit) {
                if (m_project->copySteps(target, kit))
                    m_project->removeVanishedTarget(target);
            }
        }
    };

    // clang-format off
    return Group {
        instance, options,
        QSyncTask(init),
        QSyncTask(loadConfig),
        QTaskTreeTask(startDeviceTree, onDeviceStarted),
        QSyncTask(setupProcessInterfaceCreator),
        QSyncTask(setupCmdBridge),
        QTaskTreeTask(setupManualKits),
        autoDetectDeviceToolsRecipe(),
        If (autoDetectKitsEnabled) >> Then {
            kitDetectionRecipe(shared_from_this(), DetectionSource::Temporary, instanceConfig.logFunction)
        },
        QSyncTask(restoreVanishedTargets)
    };
    // clang-format on
}

Group Device::downRecipe(bool forceDown)
{
    if (!m_downRecipe)
        return Group{};

    // clang-format off
    return Group {
        QSyncTask([this](){
            m_processInterfaceCreator = nullptr;
            m_fileAccess.reset();
            m_systemEnvironment.reset();
            DeviceManager::setDeviceState(id(), DeviceState::DeviceStateUnknown);
            DeviceManager::removeDevice(id());
        }),
        removeDetectedKitsRecipe(shared_from_this(), m_instanceConfig.logFunction),
        forceDown ? *m_forceDownRecipe : *m_downRecipe
    };
    // clang-format on
}

void Device::up(InstanceConfig instanceConfig, std::function<void(Result<>)> callback)
{
    QTC_ASSERT(m_project, return);

    const auto onDone = [callback](DoneWith doneWith) {
        const Result<> result = (doneWith != DoneWith::Error)
                                    ? ResultOk
                                    : ResultError(
                                          Tr::tr(
                                              "Cannot start the development container. Check "
                                              "General Messages for details."));
        callback(result);
    };

    const Storage<ProgressPtr> progressStorage;

    Group recipe{
        progressStorage,
        upRecipe(instanceConfig, progressStorage),
    };

    m_taskTreeRunner.start(
        recipe,
        setupProgress(
            progressStorage, Tr::tr("Starting the development container"), "DevContainer.Startup"),
        onDone);
}

Result<> Device::down()
{
    if (!m_downRecipe)
        return ResultError(
            Tr::tr("The development container is not running or has not been started."));

    const Storage<ProgressPtr> progressStorage;

    Group recipe{progressStorage, downRecipe(false)};

    if (ExtensionSystem::PluginManager::isShuttingDown()) {
        QTaskTree taskTree;
        setupProgress(
            progressStorage, Tr::tr("Stopping the development container"), "DevContainer.Shutdown")(
            taskTree);
        taskTree.setRecipe(recipe);

        return taskTree.runBlocking() == DoneWith::Success
                   ? ResultOk
                   : ResultError(
                         Tr::tr(
                             "Cannot stop the development container. Check General Messages for "
                             "details."));
    }

    m_taskTreeRunner.start(
        recipe,
        setupProgress(
            progressStorage, Tr::tr("Stopping the development container"), "DevContainer.Shutdown"));
    return ResultOk;
}

void Device::restart(std::function<void(Result<>)> callback)
{
    const Storage<ProgressPtr> progressStorage;

    const auto onDone = [callback](DoneWith doneWith) {
        const Result<> result = (doneWith != DoneWith::Error)
                                    ? ResultOk
                                    : ResultError(
                                          Tr::tr(
                                              "Cannot start the development container. Check "
                                              "General Messages for details."));
        callback(result);
    };

    // clang-format off
    Group recipe {
        progressStorage,
        downRecipe(true),
        upRecipe(m_instanceConfig, progressStorage),
    };
    // clang-format on

    m_taskTreeRunner.start(
        recipe,
        setupProgress(
            progressStorage, Tr::tr("Restarting the development container"), "DevContainer.Restart"),
        onDone);
}

FilePath Device::rootPath() const
{
    static QStringView devContainerScheme = Constants::DEVCONTAINER_FS_SCHEME;
    static QStringView root = u"/";

    return FilePath::fromParts(devContainerScheme, m_instanceConfig.devContainerId(), root);
}

ProcessInterface *Device::createProcessInterface() const
{
    if (!m_processInterfaceCreator)
        return nullptr;

    return m_processInterfaceCreator();
}

Result<Environment> Device::systemEnvironmentWithError() const
{
    if (!m_systemEnvironment)
        return ResultError(Tr::tr("System environment is not available for this device."));
    return *m_systemEnvironment;
}

Result<> Device::ensureReachable(const FilePath &other) const
{
    if (other == m_instanceConfig.workspaceFolder)
        return ResultOk;
    if (other.isChildOf(m_instanceConfig.workspaceFolder))
        return ResultOk;
    if (other.isSameDevice(rootPath()))
        return ResultOk;

    // TODO: Check additional mounts!
    return ResultError(
        Tr::tr("Cannot reach \"%1\" from \"%2\".").arg(other.toUserOutput()).arg(displayName()));
}

Result<FilePath> Device::localSource(const FilePath &other) const
{
    std::shared_ptr<FileAccess> fileAccess = std::static_pointer_cast<FileAccess>(
        this->fileAccess());
    if (!fileAccess)
        return ResultError(Tr::tr("File access is not available for this device."));

    const FilePath workspaceFolderMountPoint = fileAccess->workspaceFolderMountPoint();
    const FilePath workspaceFolder = fileAccess->workspaceFolder();

    if (other.startsWith(workspaceFolderMountPoint.path()))
        return workspaceFolder / other.path().mid(workspaceFolderMountPoint.path().size());

    return ResultError(
        Tr::tr("No mapping available for %1 on %2.").arg(other.path(), displayName()));
}

bool Device::supportsQtTargetDeviceType(const QSet<Id> &targetDeviceTypes) const
{
    return targetDeviceTypes.contains(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
           || IDevice::supportsQtTargetDeviceType(targetDeviceTypes);
}

Result<> Device::supportsBuildingProject(const FilePath &projectDir) const
{
    if (projectDir == m_instanceConfig.workspaceFolder)
        return ResultOk;
    return ResultError(
        Tr::tr(
            "The project directory \"%1\" is not inside the development container workspace folder \"%2\".")
            .arg(projectDir.toUserOutput())
            .arg(m_instanceConfig.workspaceFolder.toUserOutput()));
}

void Device::toMap(Store &map) const
{
    Q_UNUSED(map);
}

} // namespace DevContainer
