// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "deviceprocesslist.h"

namespace ProjectExplorer {
namespace Internal {

class LocalProcessList : public DeviceProcessList
{
    Q_OBJECT

public:
    explicit LocalProcessList(const IDeviceConstPtr &device, QObject *parent = nullptr);

private:
    void doUpdate() override;
    void doKillProcess(const Utils::ProcessInfo &process) override;

private:
    void handleUpdate();
    void reportDelayedKillStatus(const QString &errorMessage);
};

} // namespace Internal
} // namespace ProjectExplorer
