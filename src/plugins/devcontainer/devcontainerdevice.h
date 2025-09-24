// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <projectexplorer/devicesupport/idevice.h>

#include <devcontainer/devcontainer.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/qtcprocess.h>

namespace CmdBridge {
class FileAccess;
}

namespace ProjectExplorer {
class Project;
}

namespace DevContainer {

class ProgressPromise;
using ProgressPtr = std::unique_ptr<ProgressPromise>;

class Device : public ProjectExplorer::IDevice
{
public:
    Device(ProjectExplorer::Project *project);
    ~Device();

    ProjectExplorer::IDeviceWidget *createWidget() override;

    void up(InstanceConfig instanceConfig, std::function<void(Utils::Result<>)> callback);
    Utils::Result<> down();

    void restart(std::function<void(Utils::Result<>)> callback);

    Utils::ProcessInterface *createProcessInterface() const override;

    Utils::Result<Utils::Environment> systemEnvironmentWithError() const override;

    bool ensureReachable(const Utils::FilePath &other) const override;
    Utils::Result<Utils::FilePath> localSource(const Utils::FilePath &other) const override;

    bool supportsQtTargetDeviceType(const QSet<Utils::Id> &targetDeviceTypes) const override;

    void toMap(Utils::Store &map) const override;

    bool supportsBuildingProject(const ProjectExplorer::Project *project) const override;

public: // FilePath stuff
    bool handlesFile(const Utils::FilePath &filePath) const override;
    Utils::FilePath rootPath() const override;

private:
    void onConfigChanged();
    Tasking::Group upRecipe(
        InstanceConfig instanceConfig, Tasking::Storage<ProgressPtr> progressStorage);
    Tasking::Group downRecipe();

private:
    Utils::Process::ProcessInterfaceCreator m_processInterfaceCreator;
    InstanceConfig m_instanceConfig;
    std::unique_ptr<CmdBridge::FileAccess> m_fileAccess;
    std::optional<Utils::Environment> m_systemEnvironment;
    std::optional<Tasking::ExecutableItem> m_downRecipe;
    Tasking::ParallelTaskTreeRunner m_taskTreeRunner;

    std::unique_ptr<Utils::FilePathWatcher> m_devContainerJsonWatcher;
    std::unique_ptr<Utils::FilePathWatcher> m_dockerFileWatcher;

    QPointer<ProjectExplorer::Project> m_project;
};

} // namespace DevContainer
