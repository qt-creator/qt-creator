// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <remotelinux/remotelinuxsignaloperation.h>

namespace Qnx::Internal {

class QnxDeviceProcessSignalOperation : public RemoteLinux::RemoteLinuxSignalOperation
{
protected:
    explicit QnxDeviceProcessSignalOperation(const ProjectExplorer::IDeviceConstPtr &device);

private:
    QString killProcessByNameCommandLine(const QString &filePath) const override;
    QString interruptProcessByNameCommandLine(const QString &filePath) const override;

    friend class QnxDevice;
};

} // Qnx::Internal
