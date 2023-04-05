// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"

#include "idevice.h"

#include <QApplication>

#include <memory>

namespace ProjectExplorer {
class ProjectExplorerPlugin;
class DesktopDevicePrivate;

namespace Internal { class DesktopDeviceFactory; }

class PROJECTEXPLORER_EXPORT DesktopDevice : public IDevice
{
public:
    ~DesktopDevice() override;

    IDevice::DeviceInfo deviceInformation() const override;

    IDeviceWidget *createWidget() override;
    bool canCreateProcessModel() const override;
    DeviceProcessList *createProcessListModel(QObject *parent) const override;
    DeviceProcessSignalOperation::Ptr signalOperation() const override;
    QUrl toolControlChannel(const ControlChannelHint &) const override;
    bool usableAsBuildDevice() const override;

    bool handlesFile(const Utils::FilePath &filePath) const override;
    Utils::Environment systemEnvironment() const override;

    Utils::FilePath rootPath() const override;
    Utils::FilePath filePath(const QString &pathOnDevice) const override;

protected:
    DesktopDevice();

    friend class ProjectExplorerPlugin;
    friend class Internal::DesktopDeviceFactory;

    std::unique_ptr<DesktopDevicePrivate> d;
};

} // namespace ProjectExplorer
