// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devcontainerdevice.h"

#include "devcontainerplugin_constants.h"
#include "devcontainerplugintr.h"

#include <client/bridgedfileaccess.h>
#include <client/cmdbridgeclient.h>

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <devcontainer/devcontainerconfig.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/kitaspect.h>
#include <projectexplorer/kitmanager.h>

#include <tasking/conditional.h>

#include <utils/algorithm.h>

#include <tasking/conditional.h>

#include <QLoggingCategory>
#include <QProgressDialog>

Q_LOGGING_CATEGORY(devContainerDeviceLog, "qtc.devcontainer.device", QtWarningMsg)

using namespace Utils;
using namespace ProjectExplorer;

namespace DevContainer {

Device::Device()
{
    setDisplayType(Tr::tr("Development Container"));
    setOsType(OsTypeLinux);
    setupId(IDevice::AutoDetected, Id::generate());
    setType(Constants::DEVCONTAINER_DEVICE_TYPE);
    setMachineType(IDevice::Hardware);
    setFileAccessFactory([this]() { return m_fileAccess.get(); });
}

Device::~Device() {} // Necessary for forward declared unique_ptr

IDeviceWidget *Device::createWidget()
{
    return nullptr;
}

bool Device::handlesFile(const FilePath &filePath) const
{
    const FilePath root = rootPath();
    if (filePath.scheme() == root.scheme() && filePath.host() == root.host())
        return true;
    return false;
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

    ProgressPromise(Tasking::TaskTree &tree, const QString title, Utils::Id id)
    {
        Core::FutureProgress *futureProgress = Core::ProgressManager::addTask(future(), title, id);
        QObject::connect(
            futureProgress, &Core::FutureProgress::canceled, &tree, &Tasking::TaskTree::cancel);
        QObject::connect(&tree, &Tasking::TaskTree::destroyed, &tree, [this]() { delete this; });

        addSource(tree);
        start();
    }

    void addSource(Tasking::TaskTree &taskTree)
    {
        sources = [tt = QPointer<Tasking::TaskTree>(&taskTree),
                   max = taskTree.progressMaximum(),
                   lastSource = sources]() -> Progress {
            Progress last = lastSource();
            if (tt)
                return last + Progress{tt->progressValue(), max};
            return last + Progress{max, max};
        };
        update();
        QObject::connect(&taskTree, &Tasking::TaskTree::progressValueChanged, &taskTree, [this]() {
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
            return (m_workspaceFolderMountPoint / hostPath.mid(m_workspaceFolder.path().length()))
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

Result<> Device::up(
    const FilePath &path, InstanceConfig instanceConfig, std::function<void(Result<>)> callback)
{
    m_instanceConfig = instanceConfig;
    m_processInterfaceCreator = nullptr;
    m_fileAccess.reset();
    m_systemEnvironment.reset();
    m_downRecipe.reset();

    using namespace Tasking;

    TaskTree *tree = new TaskTree(this);
    ProgressPromise *progress
        = new ProgressPromise(*tree, Tr::tr("Starting DevContainer"), "DevContainer.Startup");

    connect(tree, &TaskTree::done, this, [progress, callback](DoneWith doneWith) {
        progress->finish();
        if (doneWith == DoneWith::Error) {
            callback(ResultError(
                Tr::tr("Failed to start DevContainer, check General Messages for details")));
        } else {
            callback(ResultOk);
        }
    });

    connect(tree, &TaskTree::done, tree, &TaskTree::deleteLater);

    struct Options
    {
        bool mountLibExec = true;
        bool copyCmdBridge = false;
        QString libExecMountPoint = "/devcontainer/libexec";
        QString workspaceFolderMountPoint;
        bool runProcessesInTerminal = false;
        bool autoDetectKits = true;
    };

    Storage<std::shared_ptr<Instance>> instance;
    Storage<Options> options;

    auto runningInstance = std::make_shared<DevContainer::RunningInstanceData>();

    const auto loadConfig = [&path, &instanceConfig, instance, options, this]() -> DoneResult {
        const auto result = [&]() -> Result<> {
            Result<Config> config = Instance::configFromFile(path, instanceConfig);
            if (!config)
                return ResultError(config.error());

            if (!config->containerConfig) {
                return ResultError(
                    Tr::tr("DevContainer config does not contain a container configuration."));
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

            instanceConfig.runProcessesInTerminal = options->runProcessesInTerminal;

            if (options->mountLibExec) {
                instanceConfig.mounts.push_back(Mount{
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

            Result<std::unique_ptr<Instance>> instanceResult
                = DevContainer::Instance::fromConfig(*config, instanceConfig);
            if (!instanceResult)
                return ResultError(instanceResult.error());

            *instance = std::move(*instanceResult);

            return ResultOk;
        }();

        if (!result) {
            instanceConfig.logFunction(
                Tr::tr("Failed to load DevContainer config: %1").arg(result.error()));
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    const auto setupProcessInterfaceCreator = [this, instance, runningInstance]() {
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

            Result<> initResult = [&] {
                if (options->copyCmdBridge) {
                    return fileAccess->deployAndInit(
                        Core::ICore::libexecPath(), rootPath(), runningInstance->remoteEnvironment);
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
            return ResultOk;
        }();

        if (!result) {
            instanceConfig.logFunction(Tr::tr("Failed to setup CmdBridge: %1").arg(result.error()));
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    const auto startDeviceTree =
        [instance, instanceConfig, runningInstance, progress](TaskTree &taskTree) -> SetupResult {
        const Result<Tasking::Group> devcontainerRecipe = (*instance)->upRecipe(runningInstance);
        if (!devcontainerRecipe) {
            instanceConfig.logFunction(
                Tr::tr("Failed to create DevContainer recipe: %1").arg(devcontainerRecipe.error()));
            return SetupResult::StopWithError;
        }
        taskTree.setRecipe(std::move(*devcontainerRecipe));
        progress->addSource(taskTree);
        return SetupResult::Continue;
    };

    const auto onDeviceStarted = [this, instance]() -> DoneResult {
        auto downRecipe = (*instance)->downRecipe();
        if (!downRecipe) {
            qCWarning(devContainerDeviceLog)
                << "Failed to create down recipe for DevContainer instance:" << downRecipe.error();
            return DoneResult::Error;
        }

        m_downRecipe = std::move(*downRecipe);
        return DoneResult::Success;
    };

    const auto setupManualKits = [this, instance, instanceConfig](TaskTree &tree) {
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

                const auto factory = Utils::findOrDefault(
                    KitAspectFactory::kitAspectFactories(),
                    Utils::equal(&KitAspectFactory::id, Utils::Id::fromString(it.key())));

                if (!factory) {
                    instanceConfig.logFunction(
                        Tr::tr("Unknown kit aspect factory: %1").arg(it.key()));
                    continue;
                }

                auto executableItem = factory->createAspectFromJson(
                    id().toString(), rootPath(), kit, it.value(), instanceConfig.logFunction);
                if (!executableItem) {
                    instanceConfig.logFunction(
                        Tr::tr("Failed to create kit aspect %1: %2")
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

    // clang-format off
    Group recipe {
        instance, options,
        Sync(loadConfig),
        TaskTreeTask(startDeviceTree, onDeviceStarted),
        Sync(setupProcessInterfaceCreator),
        Sync(setupCmdBridge),
        TaskTreeTask(setupManualKits),
        If (autoDetectKitsEnabled) >> Then {
            kitDetectionRecipe(shared_from_this(), instanceConfig.logFunction)
        },
    };
    // clang-format on

    tree->setRecipe(recipe);
    tree->start();
    return ResultOk;
}

Utils::Result<> Device::down()
{
    if (!m_downRecipe)
        return ResultError(Tr::tr("DevContainer is not running or has not been started."));

    m_processInterfaceCreator = nullptr;
    m_fileAccess.reset();
    m_systemEnvironment.reset();

    using namespace Tasking;

    TaskTree *tree = new TaskTree(this);
    ProgressPromise *progress
        = new ProgressPromise(*tree, Tr::tr("Stopping DevContainer"), "DevContainer.Shutdown");

    connect(tree, &TaskTree::done, this, [progress](DoneWith /*doneWith*/) {
        progress->finish();
    });

    connect(tree, &TaskTree::done, tree, &TaskTree::deleteLater);

    // clang-format off
    Group recipe {
        removeDetectedKitsRecipe(shared_from_this(), m_instanceConfig.logFunction),
        *m_downRecipe
    };
    // clang-format on

    tree->setRecipe(recipe);
    tree->start();

    return ResultOk;
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
};

bool Device::ensureReachable(const FilePath &other) const
{
    if (other == m_instanceConfig.workspaceFolder)
        return true;
    if (other.isChildOf(m_instanceConfig.workspaceFolder))
        return true;
    if (other.isSameDevice(rootPath()))
        return true;

    return false;
};

Result<FilePath> Device::localSource(const FilePath &other) const
{
    auto fileAccess = static_cast<FileAccess *>(this->fileAccess());
    if (!fileAccess)
        return ResultError(Tr::tr("File access is not available for this device."));

    const FilePath workspaceFolderMountPoint = fileAccess->workspaceFolderMountPoint();
    const FilePath workspaceFolder = fileAccess->workspaceFolder();

    if (other.startsWith(workspaceFolderMountPoint.path()))
        return workspaceFolder / other.path().mid(workspaceFolderMountPoint.path().length());

    return ResultError(
        Tr::tr("No mapping available for %1 on %2.").arg(other.path(), displayName()));
}

} // namespace DevContainer
