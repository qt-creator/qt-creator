// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dockersettings.h"

#include <coreplugin/documentmanager.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

#include <QMutex>

namespace Docker::Internal {

class DockerDeviceData
{
public:
    bool operator==(const DockerDeviceData &other) const
    {
        return imageId == other.imageId && repo == other.repo && tag == other.tag
               && useLocalUidGid == other.useLocalUidGid && mounts == other.mounts
               && keepEntryPoint == other.keepEntryPoint && enableLldbFlags == other.enableLldbFlags
               && clangdExecutable == other.clangdExecutable;
    }

    bool operator!=(const DockerDeviceData &other) const { return !(*this == other); }

    // Used for "docker run"
    QString repoAndTag() const
    {
        if (repo == "<none>")
            return imageId;

        if (tag == "<none>")
            return repo;

        return repo + ':' + tag;
    }

    QString repoAndTagEncoded() const { return repoAndTag().replace(':', '.'); }

    QString imageId;
    QString repo;
    QString tag;
    QString size;
    bool useLocalUidGid = true;
    QStringList mounts = {Core::DocumentManager::projectsDirectory().toString()};
    bool keepEntryPoint = false;
    bool enableLldbFlags = false;
    Utils::FilePath clangdExecutable;
};

class DockerDevice : public ProjectExplorer::IDevice
{
public:
    using Ptr = QSharedPointer<DockerDevice>;
    using ConstPtr = QSharedPointer<const DockerDevice>;

    explicit DockerDevice(DockerSettings *settings, const DockerDeviceData &data);
    ~DockerDevice();

    void shutdown();

    static Ptr create(DockerSettings *settings, const DockerDeviceData &data)
    {
        return Ptr(new DockerDevice(settings, data));
    }

    ProjectExplorer::IDeviceWidget *createWidget() override;
    QList<ProjectExplorer::Task> validate() const override;

    Utils::ProcessInterface *createProcessInterface() const override;

    bool canAutoDetectPorts() const override;
    ProjectExplorer::PortsGatheringMethod portsGatheringMethod() const override;
    bool canCreateProcessModel() const override { return false; }
    ProjectExplorer::DeviceProcessList *createProcessListModel(QObject *parent) const override;
    bool hasDeviceTester() const override { return false; }
    ProjectExplorer::DeviceTester *createDeviceTester() const override;
    bool usableAsBuildDevice() const override;

    Utils::FilePath rootPath() const override;
    Utils::FilePath filePath(const QString &pathOnDevice) const override;

    bool handlesFile(const Utils::FilePath &filePath) const override;
    bool ensureReachable(const Utils::FilePath &other) const override;
    Utils::expected_str<Utils::FilePath> localSource(const Utils::FilePath &other) const override;

    Utils::Environment systemEnvironment() const override;

    const DockerDeviceData data() const;
    DockerDeviceData data();

    void setData(const DockerDeviceData &data);

    void updateContainerAccess() const;
    void setMounts(const QStringList &mounts) const;

    bool prepareForBuild(const ProjectExplorer::Target *target) override;
    std::optional<Utils::FilePath> clangdExecutable() const override;

protected:
    void fromMap(const QVariantMap &map) final;
    QVariantMap toMap() const final;

private:
    void aboutToBeRemoved() const final;

    class DockerDevicePrivate *d = nullptr;

    friend class DockerDeviceSetupWizard;
    friend class DockerDeviceWidget;
};

class DockerDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    DockerDeviceFactory(DockerSettings *settings);

    void shutdownExistingDevices();

private:
    QMutex m_deviceListMutex;
    std::vector<QWeakPointer<DockerDevice>> m_existingDevices;
};

} // namespace Docker::Internal

Q_DECLARE_METATYPE(Docker::Internal::DockerDeviceData)
