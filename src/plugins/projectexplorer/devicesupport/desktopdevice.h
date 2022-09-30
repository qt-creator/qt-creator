// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"

#include "idevice.h"
#include "idevicefactory.h"

#include <QApplication>

namespace ProjectExplorer {
class ProjectExplorerPlugin;

namespace Internal { class DesktopDeviceFactory; }

class PROJECTEXPLORER_EXPORT DesktopDevice : public IDevice
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::DesktopDevice)

public:
    IDevice::DeviceInfo deviceInformation() const override;

    IDeviceWidget *createWidget() override;
    bool canAutoDetectPorts() const override;
    bool canCreateProcessModel() const override;
    DeviceProcessList *createProcessListModel(QObject *parent) const override;
    ProjectExplorer::PortsGatheringMethod portsGatheringMethod() const override;
    DeviceProcessSignalOperation::Ptr signalOperation() const override;
    DeviceEnvironmentFetcher::Ptr environmentFetcher() const override;
    QUrl toolControlChannel(const ControlChannelHint &) const override;
    bool usableAsBuildDevice() const override;

    bool handlesFile(const Utils::FilePath &filePath) const override;
    Utils::Environment systemEnvironment() const override;
    bool isExecutableFile(const Utils::FilePath &filePath) const override;
    bool isReadableFile(const Utils::FilePath &filePath) const override;
    bool isWritableFile(const Utils::FilePath &filePath) const override;
    bool isReadableDirectory(const Utils::FilePath &filePath) const override;
    bool isWritableDirectory(const Utils::FilePath &filePath) const override;
    bool isFile(const Utils::FilePath &filePath) const override;
    bool isDirectory(const Utils::FilePath &filePath) const override;
    bool ensureExistingFile(const Utils::FilePath &filePath) const override;
    bool createDirectory(const Utils::FilePath &filePath) const override;
    bool exists(const Utils::FilePath &filePath) const override;
    bool removeFile(const Utils::FilePath &filePath) const override;
    bool removeRecursively(const Utils::FilePath &filePath) const override;
    bool copyFile(const Utils::FilePath &filePath, const Utils::FilePath &target) const override;
    bool renameFile(const Utils::FilePath &filePath, const Utils::FilePath &target) const override;
    QDateTime lastModified(const Utils::FilePath &filePath) const override;
    Utils::FilePath symLinkTarget(const Utils::FilePath &filePath) const override;
    void iterateDirectory(const Utils::FilePath &filePath,
                          const std::function<bool(const Utils::FilePath &)> &callBack,
                          const Utils::FileFilter &filter) const override;
    std::optional<QByteArray> fileContents(const Utils::FilePath &filePath,
                                           qint64 limit,
                                           qint64 offset) const override;
    bool writeFileContents(const Utils::FilePath &filePath, const QByteArray &data) const override;
    qint64 fileSize(const Utils::FilePath &filePath) const override;
    QFile::Permissions permissions(const Utils::FilePath &filePath) const override;
    bool setPermissions(const Utils::FilePath &filePath, QFile::Permissions) const override;
    Utils::FilePath mapToGlobalPath(const Utils::FilePath &pathOnDevice) const override;

protected:
    DesktopDevice();

    friend class ProjectExplorerPlugin;
    friend class Internal::DesktopDeviceFactory;
};

} // namespace ProjectExplorer
