// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "deviceprocesslist.h"
#include "idevice.h"

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT ProcessList : public DeviceProcessList
{
    Q_OBJECT

public:
    explicit ProcessList(const IDeviceConstPtr &device, QObject *parent = nullptr);

private:
    void doUpdate() override;
    void doKillProcess(const Utils::ProcessInfo &process) override;

private:
    void handleUpdate();
    void reportDelayedKillStatus(const QString &errorMessage);

private:
    DeviceProcessSignalOperation::Ptr m_signalOperation;
};

} // namespace ProjectExplorer
