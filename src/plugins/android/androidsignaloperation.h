// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevice.h>

#include <solutions/tasking/tasktreerunner.h>

namespace Android::Internal {

class AndroidSignalOperation : public ProjectExplorer::DeviceProcessSignalOperation
{
public:
    void killProcess(qint64 pid) override;
    void killProcess(const QString &filePath) override;
    void interruptProcess(qint64 pid) override;

protected:
    explicit AndroidSignalOperation();

private:
    void signalOperationViaADB(qint64 pid, int signal);

    Tasking::TaskTreeRunner m_taskTreeRunner;

    friend class AndroidDevice;
};

} // namespace Android::Internal
