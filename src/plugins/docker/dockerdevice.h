// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dockerdeviceenvironmentaspect.h"

#include <coreplugin/documentmanager.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

#include <utils/aspectlist.h>
#include <utils/synchronizedvalue.h>

#include <utility>

namespace Docker {
namespace Internal {

class ContainerToolSettings;
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

struct MountPair
{
    Utils::FilePath path;
    Utils::FilePath containerPath;
};

// Splits "<host>" or "<host>:<container>" without interpreting either side as a
// path, which is what makes it checkable on any host OS.
std::pair<QString, QString> splitMountEntry(const QString &entry);

MountPair parseMount(const QString &entry);
QList<MountPair> parseMounts(const QStringList &entries);

// Where a host path shows up inside the container, for a mount that names its
// container path. Empty when no such mount covers it, leaving the caller with
// the plain path. The way back is DockerDevicePrivate::localSource().
Utils::FilePath mapToContainerPath(const QList<MountPair> &mounts, const Utils::FilePath &hostPath);

// Inverts the drive-letter mapping a Windows host applies on the way in (see
// mapToDevicePath): /c/dev/src -> C:/dev/src. Empty when the path is not shaped
// like that, or when the host does not map that way at all. Takes the host OS
// rather than asking HostOsInfo, so the rule can be checked from any host.
QString invertedDriveLetterPath(Utils::OsType hostOs, const Utils::FilePath &devicePath);

// Where a path inside the container comes from on the host, the way back from
// mapToContainerPath(). The mounts are tried before the inversion above, so a
// mount naming "/w" wins over reading it as drive W.
Utils::Result<Utils::FilePath> hostPathFor(
    const QList<MountPair> &mounts, Utils::OsType hostOs, const Utils::FilePath &devicePath);

// The whole host-to-container translation: the mounts above, and failing those
// the drive letter a Windows host gets. Pure, so it needs no running container.
QString containerPathFor(const QList<MountPair> &mounts, const QString &hostPath);

} // namespace Internal

class DOCKER_EXPORT DockerDevice : public ProjectExplorer::IDevice
{
public:
    using Ptr = std::shared_ptr<DockerDevice>;
    using ConstPtr = std::shared_ptr<const DockerDevice>;

    explicit DockerDevice(Internal::ContainerToolSettings *settings);
    ~DockerDevice();

    void shutdown();

    static Ptr create(Internal::ContainerToolSettings *settings)
    {
        return Ptr(new DockerDevice(settings));
    }

    Utils::CommandLine createCommandLine() const;
    Utils::CommandLine createCommandLineForDisplay() const;

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
    Utils::FilePath configuredDevicePath(const Utils::FilePath &localPath) const override;

    Utils::Result<Utils::Environment> systemEnvironmentWithError() const override;
    Utils::Result<Utils::Environment> systemEnvironmentIfKnown() const override;

    Utils::Result<> updateContainerAccess() const;

    bool prepareForBuild(const ProjectExplorer::Target *target) override;

    QString repoAndTag() const;
    QString repoAndTagEncoded() const;

    QString deviceStateToString() const override;
    QPixmap deviceStateIcon() const override;

    QUrl toolControlChannel(const ControlChannelHint &) const override;
    QString qmlDebugServerBindHost() const override;

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
    Utils::BoolAspect enableX11Forwarding{this};
    Utils::StringAspect x11Display{this};

protected:
    void fromMap(const Utils::Store &map) final;
    void toMap(Utils::Store &map) const final;

private:
    void aboutToBeRemoved() const final;
    QtTaskTree::ExecutableItem signalOperationRecipeImpl(
        const ProjectExplorer::SignalOperationData &data,
        const QtTaskTree::Storage<Utils::Result<>> &resultStorage) const final;

    Internal::DockerDevicePrivate *d = nullptr;

    friend class Internal::DockerDeviceSetupWizard;
    friend class Internal::DockerDeviceWidget;
};

namespace Internal {

class DockerDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    explicit DockerDeviceFactory(ContainerToolSettings *settings);

    void shutdownExistingDevices();

private:
    ContainerToolSettings *m_settings = nullptr;
    Utils::SynchronizedValue<std::vector<std::weak_ptr<DockerDevice>>> m_existingDevices;
};

} // namespace Internal
} // namespace Docker
