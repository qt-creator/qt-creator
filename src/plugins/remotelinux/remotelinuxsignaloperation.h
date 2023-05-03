// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>

namespace RemoteLinux {

class REMOTELINUX_EXPORT RemoteLinuxSignalOperation
        : public ProjectExplorer::DeviceProcessSignalOperation
{
    Q_OBJECT
public:
    ~RemoteLinuxSignalOperation() override;

    void killProcess(qint64 pid) override;
    void killProcess(const QString &filePath) override;
    void interruptProcess(qint64 pid) override;
    void interruptProcess(const QString &filePath) override;

protected:
    RemoteLinuxSignalOperation(const ProjectExplorer::IDeviceConstPtr &device);

private:
    virtual QString killProcessByNameCommandLine(const QString &filePath) const;
    virtual QString interruptProcessByNameCommandLine(const QString &filePath) const;

    void runnerDone();
    void run(const QString &command);

    const ProjectExplorer::IDeviceConstPtr m_device;
    std::unique_ptr<Utils::Process> m_process;

    friend class LinuxDevice;
};

}
