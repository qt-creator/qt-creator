// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/documentmanager.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

#include <QMutex>

namespace Docker::Internal {

class DockerDevice : public ProjectExplorer::IDevice
{
public:
    using Ptr = std::shared_ptr<DockerDevice>;
    using ConstPtr = std::shared_ptr<const DockerDevice>;

    DockerDevice();
    ~DockerDevice();

    void shutdown();

    static Ptr create() { return Ptr(new DockerDevice); }

    Utils::CommandLine createCommandLine() const;

    ProjectExplorer::IDeviceWidget *createWidget() override;
    QList<ProjectExplorer::Task> validate() const override;

    Utils::ProcessInterface *createProcessInterface() const override;

    bool canCreateProcessModel() const override { return true; }
    bool hasDeviceTester() const override { return false; }
    ProjectExplorer::DeviceTester *createDeviceTester() const override;
    bool usableAsBuildDevice() const override;

    Utils::FilePath rootPath() const override;
    Utils::FilePath filePath(const QString &pathOnDevice) const override;

    bool canMount(const Utils::FilePath &filePath) const override
    {
        return !filePath.needsDevice() || filePath.isSameDevice(rootPath());
    }

    bool handlesFile(const Utils::FilePath &filePath) const override;
    bool ensureReachable(const Utils::FilePath &other) const override;
    Utils::expected_str<Utils::FilePath> localSource(const Utils::FilePath &other) const override;

    Utils::expected_str<Utils::Environment> systemEnvironmentWithError() const override;

    Utils::expected_str<void> updateContainerAccess() const;
    void setMounts(const QStringList &mounts) const;

    bool prepareForBuild(const ProjectExplorer::Target *target) override;
    std::optional<Utils::FilePath> clangdExecutable() const override;

    QString repoAndTag() const;
    QString repoAndTagEncoded() const;

    Utils::StringAspect imageId{this};
    Utils::StringAspect repo{this};
    Utils::StringAspect tag{this};
    Utils::BoolAspect useLocalUidGid{this};
    Utils::FilePathListAspect mounts{this};
    Utils::BoolAspect keepEntryPoint{this};
    Utils::BoolAspect enableLldbFlags{this};
    Utils::FilePathAspect clangdExecutableAspect{this};
    Utils::StringSelectionAspect network{this};
    Utils::StringAspect extraArgs{this};

    Utils::TextDisplay containerStatus{this};

protected:
    void fromMap(const Utils::Store &map) final;
    void toMap(Utils::Store &map) const final;

private:
    void aboutToBeRemoved() const final;

    class DockerDevicePrivate *d = nullptr;

    friend class DockerDeviceSetupWizard;
    friend class DockerDeviceWidget;
};

class DockerDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    DockerDeviceFactory();

    void shutdownExistingDevices();

private:
    QMutex m_deviceListMutex;
    std::vector<std::weak_ptr<DockerDevice>> m_existingDevices;
};

} // namespace Docker::Internal
