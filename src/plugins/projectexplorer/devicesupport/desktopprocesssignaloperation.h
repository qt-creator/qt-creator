// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "idevice.h"

#include <projectexplorer/projectexplorer_export.h>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT DesktopProcessSignalOperation : public DeviceProcessSignalOperation
{
    Q_OBJECT
public:
    void killProcess(qint64 pid) override;
    void killProcess(const QString &filePath) override;
    void interruptProcess(qint64 pid) override;
    void interruptProcess(const QString &filePath) override;

private:
    void killProcessSilently(qint64 pid);
    void interruptProcessSilently(qint64 pid);

    void appendMsgCannotKill(qint64 pid, const QString &why);
    void appendMsgCannotInterrupt(qint64 pid, const QString &why);

protected:
    DesktopProcessSignalOperation() = default;

    friend class DesktopDevice;
};

} // namespace ProjectExplorer
