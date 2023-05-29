// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>

namespace Tasking { class GroupItem; }

namespace RemoteLinux {

namespace Internal { class GenericLinuxDeviceTesterPrivate; }

class REMOTELINUX_EXPORT GenericLinuxDeviceTester : public ProjectExplorer::DeviceTester
{
    Q_OBJECT

public:
    explicit GenericLinuxDeviceTester(QObject *parent = nullptr);
    ~GenericLinuxDeviceTester() override;

    void setExtraCommandsToTest(const QStringList &extraCommands);
    void setExtraTests(const QList<Tasking::GroupItem> &extraTests);
    void testDevice(const ProjectExplorer::IDevice::Ptr &deviceConfiguration) override;
    void stopTest() override;

private:
    std::unique_ptr<Internal::GenericLinuxDeviceTesterPrivate> d;
};

} // namespace RemoteLinux
