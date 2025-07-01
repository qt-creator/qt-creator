// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devcontainerdevice.h"

#include "devcontainerplugintr.h"

#include <client/bridgedfileaccess.h>
#include <client/cmdbridgeclient.h>

#include <coreplugin/icore.h>

#include <devcontainer/devcontainerconfig.h>

#include <QLoggingCategory>
#include <QProgressDialog>

Q_LOGGING_CATEGORY(devContainerDeviceLog, "qtc.devcontainer.device", QtWarningMsg)

using namespace Utils;

namespace DevContainer {

Device::Device() {}

Device::~Device() {} // Necessary for forward declared unique_ptr

ProjectExplorer::IDeviceWidget *Device::createWidget()
{
    return nullptr;
}

bool Device::handlesFile(const FilePath &filePath) const
{
    const Utils::FilePath root = rootPath();
    if (filePath.scheme() == root.scheme() && filePath.host() == root.host())
        return true;
    return false;
}

class ProgressDialog : public QProgressDialog
{
public:
    using Source = std::function<std::pair<int, int>()>;
    Source sources = [] { return std::make_pair(0, 0); };

    ProgressDialog()
    {
        show();
        setWindowTitle(Tr::tr("Starting DevContainer"));
        setValue(1);
        setLabelText(Tr::tr("Loading DevContainer..."));
    }

    void addSource(Tasking::TaskTree &taskTree)
    {
        sources = [&taskTree, lastSource = sources]() {
            auto last = lastSource();
            return std::make_pair(
                last.first + taskTree.progressValue(), last.second + taskTree.progressMaximum());
        };
        update();
        connect(&taskTree, &Tasking::TaskTree::progressValueChanged, this, &ProgressDialog::update);
    }

    void update()
    {
        std::pair<int, int> total = sources();
        if (maximum() != total.second)
            setMaximum(total.second);
        if (value() != total.first)
            setValue(total.first);
    }
};

Result<> Device::up(const FilePath &path, InstanceConfig instanceConfig)
{
    m_instanceConfig = instanceConfig;
    m_processInterfaceCreator = nullptr;
    m_fileAccess.reset();

    ProgressDialog progress;

    using namespace Tasking;

    struct Options
    {
        bool mountLibExec = true;
        bool copyCmdBridge = false;
        QString libExecMountPoint = "/devcontainer/libexec";
    };

    QString error;
    Storage<std::shared_ptr<Instance>> instance;
    Storage<Options> options;
    auto runningInstance = std::make_shared<DevContainer::RunningInstanceData>();

    const auto loadConfig = [&error, &path, &instanceConfig, instance, options]() -> DoneResult {
        const auto result = [&]() -> Result<> {
            Result<Config> config = Instance::configFromFile(path, instanceConfig);
            if (!config)
                return ResultError(config.error());

            options->mountLibExec
                = DevContainer::customization(*config, "qt-creator/device/mount-libexec")
                      .toBool(true);

            options->libExecMountPoint
                = DevContainer::customization(*config, "qt-creator/device/libexec-mount-point")
                      .toString("/devcontainer/libexec");

            options->copyCmdBridge
                = DevContainer::customization(*config, "qt-creator/device/copy-cmd-bridge")
                      .toBool(false);

            if (options->mountLibExec) {
                instanceConfig.mounts.push_back(Mount{
                    .type = MountType::Bind,
                    .source = Core::ICore::libexecPath().absoluteFilePath().path(),
                    .target = options->libExecMountPoint,
                });
            }

            Result<std::unique_ptr<Instance>> instanceResult
                = DevContainer::Instance::fromConfig(*config, instanceConfig);
            if (!instanceResult)
                return ResultError(instanceResult.error());

            *instance = std::move(*instanceResult);

            return ResultOk;
        }();

        if (!result) {
            error = result.error();
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    const auto setupProcessInterfaceCreator = [this, instance, runningInstance]() {
        m_processInterfaceCreator = [inst = *instance, runningInstance] {
            return inst->createProcessInterface(runningInstance);
        };
    };

    const auto setupCmdBridge = [this, runningInstance, instance, &error, options]() -> DoneResult {
        const auto result = [&]() -> Result<> {
            Utils::Result<Utils::FilePath> cmdBridgePath = CmdBridge::Client::getCmdBridgePath(
                runningInstance->osType, runningInstance->osArch, Core::ICore::libexecPath());

            if (!cmdBridgePath)
                return ResultError(cmdBridgePath.error());

            auto fileAccess = std::make_unique<CmdBridge::FileAccess>();

            Utils::Result<> initResult = [&] {
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

            setFileAccess(fileAccess.get());
            m_fileAccess = std::move(fileAccess);
            return ResultOk;
        }();

        if (!result) {
            error = result.error();
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    const auto subTree =
        [instance, runningInstance, &error, &progress](TaskTree &taskTree) -> SetupResult {
        const Result<Tasking::Group> devcontainerRecipe = (*instance)->upRecipe(runningInstance);
        if (!devcontainerRecipe) {
            error = devcontainerRecipe.error();
            return SetupResult::StopWithError;
        }
        taskTree.setRecipe(std::move(*devcontainerRecipe));
        progress.addSource(taskTree);
        return SetupResult::Continue;
    };

    // clang-format off
    Group recipe {
        instance, options,
        Sync(loadConfig),
        TaskTreeTask(subTree),
        Sync(setupProcessInterfaceCreator),
        Sync(setupCmdBridge)
    };
    // clang-format on

    Tasking::TaskTree tree;
    tree.setRecipe(
        recipe.withCancel([&] { return std::make_pair(&progress, &QProgressDialog::canceled); }));
    progress.addSource(tree);

    const auto doneWith = tree.runBlocking();
    if (doneWith != Tasking::DoneWith::Success)
        return ResultError(Tr::tr("Failed to start DevContainer:") + error);

    return ResultOk;
}

Utils::FilePath Device::rootPath() const
{
    static QStringView devContainerScheme = u"devcontainer";
    static QStringView root = u"/";

    return FilePath::fromParts(devContainerScheme, m_instanceConfig.devContainerId(), root);
}

Utils::ProcessInterface *Device::createProcessInterface() const
{
    if (!m_processInterfaceCreator)
        return nullptr;

    return m_processInterfaceCreator();
}

} // namespace DevContainer
