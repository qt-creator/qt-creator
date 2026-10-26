// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

#include <utils/aspects.h>
#include <utils/synchronizedvalue.h>

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

    // Drops the command bridge, so that the next access starts a fresh one.
    // A distribution that was terminated from the outside leaves one behind
    // that answers nothing.
    void shutdown();

    // What a command sent to this distribution turns into, for the widget to
    // show. The marker and the command itself are left off.
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

class WslDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    WslDeviceFactory();

    void shutdownExistingDevices();

private:
    Utils::SynchronizedValue<std::vector<std::weak_ptr<WslDevice>>> m_existingDevices;
};

// Points a freshly constructed device at a distribution. Asks the
// distribution nothing, so that adding a device does not start it.
void configureWslDevice(const WslDevice::Ptr &device, const QString &distribution);

// The installed distributions, for addMissingWslDevices() to work from. Talks
// to the WSL service, so it belongs off the main thread.
QStringList detectWslDistributions();

// Adds a device for every distribution that has none yet. WSL keeps no stable
// identifier for a distribution beyond its name, so the name is what an
// existing device is recognized by. Touches the device manager, so main
// thread only.
void addMissingWslDevices(const QStringList &distributions);

} // namespace Internal
} // namespace Wsl
