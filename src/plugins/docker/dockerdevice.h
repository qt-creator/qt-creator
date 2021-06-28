/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>
#include <projectexplorer/devicesupport/sshdeviceprocess.h>

#include <utils/aspects.h>

namespace Docker {
namespace Internal {

class DockerDeviceData
{
public:
    QString id() const { return "docker:" + imageId; }

    QString imageId;
    QString repo;
    QString tag;
    QString size;
};

class DockerDevice : public ProjectExplorer::IDevice
{
    Q_DECLARE_TR_FUNCTIONS(Docker::Internal::DockerDevice)

public:
    using Ptr = QSharedPointer<DockerDevice>;
    using ConstPtr = QSharedPointer<const DockerDevice>;

    explicit DockerDevice(const DockerDeviceData &data);
    ~DockerDevice();

    static Ptr create(const DockerDeviceData &data) { return Ptr(new DockerDevice(data)); }

    ProjectExplorer::IDeviceWidget *createWidget() override;

    bool canCreateProcess() const override { return true; }
    ProjectExplorer::DeviceProcess *createProcess(QObject *parent) const override;
    bool canAutoDetectPorts() const override;
    ProjectExplorer::PortsGatheringMethod::Ptr portsGatheringMethod() const override;
    bool canCreateProcessModel() const override { return false; }
    ProjectExplorer::DeviceProcessList *createProcessListModel(QObject *parent) const override;
    bool hasDeviceTester() const override { return false; }
    ProjectExplorer::DeviceTester *createDeviceTester() const override;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const override;
    ProjectExplorer::DeviceEnvironmentFetcher::Ptr environmentFetcher() const override;

    Utils::FilePath mapToGlobalPath(const Utils::FilePath &pathOnDevice) const override;

    bool handlesFile(const Utils::FilePath &filePath) const override;
    bool isExecutableFile(const Utils::FilePath &filePath) const override;
    bool isReadableFile(const Utils::FilePath &filePath) const override;
    bool isWritableFile(const Utils::FilePath &filePath) const override;
    bool isReadableDirectory(const Utils::FilePath &filePath) const override;
    bool isWritableDirectory(const Utils::FilePath &filePath) const override;
    bool createDirectory(const Utils::FilePath &filePath) const override;
    bool exists(const Utils::FilePath &filePath) const override;
    bool removeFile(const Utils::FilePath &filePath) const override;
    bool copyFile(const Utils::FilePath &filePath, const Utils::FilePath &target) const override;
    bool renameFile(const Utils::FilePath &filePath, const Utils::FilePath &target) const override;
    Utils::FilePath searchInPath(const Utils::FilePath &filePath) const override;
    QList<Utils::FilePath> directoryEntries(const Utils::FilePath &filePath,
                                            const QStringList &nameFilters,
                                            QDir::Filters filters) const override;
    QByteArray fileContents(const Utils::FilePath &filePath, int limit) const override;
    bool writeFileContents(const Utils::FilePath &filePath, const QByteArray &data) const override;
    QDateTime lastModified(const Utils::FilePath &filePath) const override;
    void runProcess(Utils::QtcProcess &process) const override;

    Utils::Environment systemEnvironment() const override;

    const DockerDeviceData &data() const;

    void tryCreateLocalFileAccess() const;
    bool hasLocalFileAccess() const;

    Utils::FilePath mapToLocalAccess(const Utils::FilePath &filePath) const;
    Utils::FilePath mapFromLocalAccess(const Utils::FilePath &filePath) const;
    Utils::FilePath mapFromLocalAccess(const QString &filePath) const;

private:
    void fromMap(const QVariantMap &map) final;
    QVariantMap toMap() const final;

    void aboutToBeRemoved() const final;

    class DockerDevicePrivate *d = nullptr;
    friend class DockerDeviceSetupWizard;
    friend class DockerDeviceWidget;
};

class DockerDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    DockerDeviceFactory();

    ProjectExplorer::IDevice::Ptr create() const override;
};

} // Internal
} // Docker

Q_DECLARE_METATYPE(Docker::Internal::DockerDeviceData)
