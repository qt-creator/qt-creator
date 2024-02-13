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

    void setSerialNumber(const QString &serial);
    QString serialNumber() const;

    void setupDefaultNetworkSettings(const QString &host);

protected:
    void fromMap(const Utils::Store &map) final;
    Utils::Store toMap() const final;

private:
    QdbDevice();

    QString m_serialNumber;
};

void setupQdbLinuxDevice();

} // Qdb::Internal
