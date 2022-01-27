/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

namespace RemoteLinux {

class REMOTELINUX_EXPORT LinuxDevice : public ProjectExplorer::IDevice
{
    Q_DECLARE_TR_FUNCTIONS(RemoteLinux::Internal::LinuxDevice)

public:
    using Ptr = QSharedPointer<LinuxDevice>;
    using ConstPtr = QSharedPointer<const LinuxDevice>;

    ~LinuxDevice();

    static Ptr create() { return Ptr(new LinuxDevice); }

    ProjectExplorer::IDeviceWidget *createWidget() override;

    bool canCreateProcess() const override { return true; }
    ProjectExplorer::DeviceProcess *createProcess(QObject *parent) const override;
    bool canAutoDetectPorts() const override;
    ProjectExplorer::PortsGatheringMethod::Ptr portsGatheringMethod() const override;
    bool canCreateProcessModel() const override { return true; }
    ProjectExplorer::DeviceProcessList *createProcessListModel(QObject *parent) const override;
    bool hasDeviceTester() const override { return true; }
    ProjectExplorer::DeviceTester *createDeviceTester() const override;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const override;
    ProjectExplorer::DeviceEnvironmentFetcher::Ptr environmentFetcher() const override;

    QString userAtHost() const;
    Utils::FilePath mapToGlobalPath(const Utils::FilePath &pathOnDevice) const override;

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
    QByteArray fileContents(const Utils::FilePath &filePath, qint64 limit, qint64 offset) const override;
    bool writeFileContents(const Utils::FilePath &filePath, const QByteArray &data) const override;
    QDateTime lastModified(const Utils::FilePath &filePath) const override;
    void runProcess(Utils::QtcProcess &process) const override;
    qint64 fileSize(const Utils::FilePath &filePath) const override;
    qint64 bytesAvailable(const Utils::FilePath &filePath) const override;
    QFileDevice::Permissions permissions(const Utils::FilePath &filePath) const override;
    bool setPermissions(const Utils::FilePath &filePath, QFileDevice::Permissions permissions) const override;

protected:
    LinuxDevice();

    class LinuxDevicePrivate *d;
};

namespace Internal {

class LinuxDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    LinuxDeviceFactory();
};

} // namespace Internal
} // namespace RemoteLinux
