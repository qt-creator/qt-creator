// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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
               && keepEntryPoint == other.keepEntryPoint;
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

    QString imageId;
    QString repo;
    QString tag;
    QString size;
    bool useLocalUidGid = true;
    QStringList mounts = {Core::DocumentManager::projectsDirectory().toString()};
    bool keepEntryPoint = false;
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
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const override;
    ProjectExplorer::DeviceEnvironmentFetcher::Ptr environmentFetcher() const override;
    bool usableAsBuildDevice() const override;

    Utils::FilePath mapToGlobalPath(const Utils::FilePath &pathOnDevice) const override;
    QString mapToDevicePath(const Utils::FilePath &globalPath) const override;

    Utils::FilePath rootPath() const override;

    bool handlesFile(const Utils::FilePath &filePath) const override;
    bool isExecutableFile(const Utils::FilePath &filePath) const override;
    bool isReadableFile(const Utils::FilePath &filePath) const override;
    bool isWritableFile(const Utils::FilePath &filePath) const override;
    bool isReadableDirectory(const Utils::FilePath &filePath) const override;
    bool isWritableDirectory(const Utils::FilePath &filePath) const override;
    bool isFile(const Utils::FilePath &filePath) const override;
    bool isDirectory(const Utils::FilePath &filePath) const override;
    bool createDirectory(const Utils::FilePath &filePath) const override;
    bool exists(const Utils::FilePath &filePath) const override;
    bool ensureExistingFile(const Utils::FilePath &filePath) const override;
    bool removeFile(const Utils::FilePath &filePath) const override;
    bool removeRecursively(const Utils::FilePath &filePath) const override;
    bool copyFile(const Utils::FilePath &filePath, const Utils::FilePath &target) const override;
    bool renameFile(const Utils::FilePath &filePath, const Utils::FilePath &target) const override;
    Utils::FilePath symLinkTarget(const Utils::FilePath &filePath) const override;
    void iterateDirectory(const Utils::FilePath &filePath,
                          const std::function<bool(const Utils::FilePath &)> &callBack,
                          const Utils::FileFilter &filter) const override;
    std::optional<QByteArray> fileContents(const Utils::FilePath &filePath,
                                           qint64 limit,
                                           qint64 offset) const override;
    bool writeFileContents(const Utils::FilePath &filePath, const QByteArray &data) const override;
    QDateTime lastModified(const Utils::FilePath &filePath) const override;
    qint64 fileSize(const Utils::FilePath &filePath) const override;
    QFileDevice::Permissions permissions(const Utils::FilePath &filePath) const override;
    bool setPermissions(const Utils::FilePath &filePath,
                        QFileDevice::Permissions permissions) const override;
    bool ensureReachable(const Utils::FilePath &other) const override;

    Utils::Environment systemEnvironment() const override;

    const DockerDeviceData data() const;
    DockerDeviceData data();

    void setData(const DockerDeviceData &data);

    void updateContainerAccess() const;
    void setMounts(const QStringList &mounts) const;

    bool prepareForBuild(const ProjectExplorer::Target *target) override;

protected:
    void fromMap(const QVariantMap &map) final;
    QVariantMap toMap() const final;

private:
    void iterateWithFind(const Utils::FilePath &filePath,
                         const std::function<bool(const Utils::FilePath &)> &callBack,
                         const Utils::FileFilter &filter) const;

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
