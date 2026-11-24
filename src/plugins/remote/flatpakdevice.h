// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevice.h>

#include <utils/aspects.h>

namespace Remote {

namespace Internal {
class FlatpakDevicePrivate;
} // namespace Internal

class FlatpakDevice final : public ProjectExplorer::IDevice
{
public:
    using Ptr = std::shared_ptr<FlatpakDevice>;

    ~FlatpakDevice() final;

    static Ptr create() { return Ptr(new FlatpakDevice); }

    void shutdown();

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

    Utils::FilePathAspect launcher{this};

private:
    FlatpakDevice();

    QtTaskTree::ExecutableItem signalOperationRecipeImpl(
        const ProjectExplorer::SignalOperationData &data,
        const QtTaskTree::Storage<Utils::Result<>> &resultStorage) const final;

    Internal::FlatpakDevicePrivate *d = nullptr;

    friend class Internal::FlatpakDevicePrivate;
};

namespace Internal {

void setupFlatpakDevice();

void addMissingFlatpakDevice();

} // namespace Internal
} // namespace Remote
