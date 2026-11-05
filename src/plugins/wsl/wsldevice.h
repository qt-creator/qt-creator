// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevice.h>

#include <utils/aspects.h>

namespace Wsl {

namespace Internal {
class WslDevicePrivate;
class WslDeviceWidget;
} // namespace Internal

class WslDevice final : public ProjectExplorer::IDevice
{
public:
    using Ptr = std::shared_ptr<WslDevice>;
    using ConstPtr = std::shared_ptr<const WslDevice>;

    ~WslDevice() final;

    static Ptr create() { return Ptr(new WslDevice); }

    void shutdown();

    Utils::CommandLine createCommandLineForDisplay() const;

    ProjectExplorer::IDeviceWidget *createWidget() final;

    bool canCreateProcessModel() const final { return true; }
    bool hasDeviceTester() const final { return false; }

    Utils::ProcessInterface *createProcessInterface() const final;

    Utils::FilePath rootPath() const final;

    bool supportsQtTargetDeviceType(const QSet<Utils::Id> &targetDeviceTypes) const final;
    Utils::Result<> supportsBuildingProject(const Utils::FilePath &projectDir) const final;
    Utils::Result<> handlesFile(const Utils::FilePath &filePath) const final;
    Utils::Result<> ensureReachable(const Utils::FilePath &other) const final;
    Utils::Result<Utils::FilePath> localSource(const Utils::FilePath &other) const final;
    Utils::FilePath configuredDevicePath(const Utils::FilePath &localPath) const final;

    QUrl toolControlChannel(const ControlChannelHint &hint) const final;

    QString deviceStateToString() const final;

    Utils::StringAspect distribution{this};
    Utils::StringAspect userName{this};
    Utils::StringAspect mountRoot{this};
    Utils::BoolAspect appendWindowsPath{this};

protected:
    void fromMap(const Utils::Store &map) final;

private:
    WslDevice();

    QtTaskTree::ExecutableItem signalOperationRecipeImpl(
        const ProjectExplorer::SignalOperationData &data,
        const QtTaskTree::Storage<Utils::Result<>> &resultStorage) const final;

    Internal::WslDevicePrivate *d = nullptr;

    friend class Internal::WslDevicePrivate;
    friend class Internal::WslDeviceWidget;
};

namespace Internal {

void setupWslDevice();

void configureWslDevice(const WslDevice::Ptr &device, const QString &distribution);

QStringList detectWslDistributions();

void addMissingWslDevices(const QStringList &distributions);

} // namespace Internal
} // namespace Wsl
