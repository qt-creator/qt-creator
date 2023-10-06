// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dockersettings.h"

#include <coreplugin/documentmanager.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

#include <QMutex>

namespace Docker::Internal {

class DockerDeviceSettings : public ProjectExplorer::DeviceSettings
{
public:
    DockerDeviceSettings();

    void fromMap(const Utils::Store &map) override;

    QString repoAndTag() const;
    QString repoAndTagEncoded() const;
    Utils::FilePath rootPath() const;

    Utils::StringAspect imageId{this};
    Utils::StringAspect repo{this};
    Utils::StringAspect tag{this};
    Utils::BoolAspect useLocalUidGid{this};
    Utils::FilePathListAspect mounts{this};
    Utils::BoolAspect keepEntryPoint{this};
    Utils::BoolAspect enableLldbFlags{this};
    Utils::FilePathAspect clangdExecutable{this};
    Utils::StringSelectionAspect network{this};
    Utils::StringAspect extraArgs{this};

    Utils::TextDisplay containerStatus{this};
};

class DockerDevice : public ProjectExplorer::IDevice
{
public:
    using Ptr = QSharedPointer<DockerDevice>;
    using ConstPtr = QSharedPointer<const DockerDevice>;

    explicit DockerDevice(std::unique_ptr<DockerDeviceSettings> settings);
    ~DockerDevice();

    void shutdown();

    static Ptr create(std::unique_ptr<DockerDeviceSettings> settings)
    {
        return Ptr(new DockerDevice(std::move(settings)));
    }

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

    bool handlesFile(const Utils::FilePath &filePath) const override;
    bool ensureReachable(const Utils::FilePath &other) const override;
    Utils::expected_str<Utils::FilePath> localSource(const Utils::FilePath &other) const override;

    Utils::expected_str<Utils::Environment> systemEnvironmentWithError() const override;

    Utils::expected_str<void> updateContainerAccess() const;
    void setMounts(const QStringList &mounts) const;

    bool prepareForBuild(const ProjectExplorer::Target *target) override;
    std::optional<Utils::FilePath> clangdExecutable() const override;

protected:
    void fromMap(const Utils::Store &map) final;
    Utils::Store toMap() const final;

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
    std::vector<QWeakPointer<DockerDevice>> m_existingDevices;
};

} // namespace Docker::Internal
