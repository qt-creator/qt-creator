// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/kit.h>
#include <remotelinux/linuxdevice.h>

namespace Qdb::Internal {

class QdbDevice final : public RemoteLinux::LinuxDevice
{
public:
    typedef std::shared_ptr<QdbDevice> Ptr;
    typedef std::shared_ptr<const QdbDevice> ConstPtr;

    static Ptr create() { return Ptr(new QdbDevice); }

    ProjectExplorer::IDeviceWidget *createWidget() final;

    Utils::ProcessInterface *createProcessInterface() const override;

    void setupDefaultNetworkSettings(const QString &host);

private:
    QdbDevice();
};

void setupQdbLinuxDevice();

} // Qdb::Internal
