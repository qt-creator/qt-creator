// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dockerdeviceenvironmentaspect.h"

#include <coreplugin/documentmanager.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

#include <utils/synchronizedvalue.h>

namespace Docker {
namespace Internal {

class DockerDevicePrivate;
class DockerDeviceSetupWizard;
class DockerDeviceWidget;

class PortMappings final : public Utils::AspectList
{
public:
    explicit PortMappings(Utils::AspectContainer *container);

    QStringList createArguments() const;
    QSet<int> usedContainerPorts() const;
};

} // namespace Internal

class DOCKER_EXPORT DockerDevice : public ProjectExplorer::IDevice
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
    ProjectExplorer::DeviceTester *createDeviceTester() override;

    Utils::FilePath rootPath() const override;

    bool canMount(const Utils::FilePath &filePath) const override
    {
        return filePath.isLocal() || filePath.isSameDevice(rootPath());
    }

    bool supportsQtTargetDeviceType(const QSet<Utils::Id> &targetDeviceTypes) const override;

    Utils::Result<> supportsBuildingProject(const Utils::FilePath &projectDir) const override;
    Utils::Result<> handlesFile(const Utils::FilePath &filePath) const override;
    Utils::Result<> ensureReachable(const Utils::FilePath &other) const override;
    Utils::Result<Utils::FilePath> localSource(const Utils::FilePath &other) const override;

    Utils::Result<Utils::Environment> systemEnvironmentWithError() const override;

    Utils::Result<> updateContainerAccess() const;

    bool prepareForBuild(const ProjectExplorer::Target *target) override;

    QString repoAndTag() const;
    QString repoAndTagEncoded() const;

    QString deviceStateToString() const override;
    QPixmap deviceStateIcon() const override;

    QUrl toolControlChannel(const ControlChannelHint &) const override;

    Utils::StringAspect imageId{this};
    Utils::StringAspect repo{this};
    Utils::StringAspect tag{this};
    Utils::BoolAspect useLocalUidGid{this};
    Utils::FilePathListAspect mounts{this};
    Utils::BoolAspect keepEntryPoint{this};
    Utils::BoolAspect enableLldbFlags{this};
    Utils::StringSelectionAspect network{this};
    Utils::StringAspect extraArgs{this};
    DockerDeviceEnvironmentAspect environment{this};
    Internal::PortMappings portMappings{this};
    Utils::BoolAspect mountCmdBridge{this};

protected:
    void fromMap(const Utils::Store &map) final;
    void toMap(Utils::Store &map) const final;

private:
    void aboutToBeRemoved() const final;

    Internal::DockerDevicePrivate *d = nullptr;

    friend class Internal::DockerDeviceSetupWizard;
    friend class Internal::DockerDeviceWidget;
};

namespace Internal {

class DockerDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    DockerDeviceFactory();

    void shutdownExistingDevices();

private:
    Utils::SynchronizedValue<std::vector<std::weak_ptr<DockerDevice>>> m_existingDevices;
};

} // namespace Internal
} // namespace Docker
