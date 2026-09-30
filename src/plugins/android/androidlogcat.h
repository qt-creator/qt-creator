// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "androiddevice.h"

#include <functional>

namespace ProjectExplorer {
class RunControl;
}

namespace Android::Internal {

void showLogcatTab(const AndroidDevice::ConstPtr &device);

void bindRunningAppToLogcat(ProjectExplorer::RunControl *runControl, qint64 pid,
                            const QString &packageName);

using LogcatLineHandler = std::function<void(qint64 pid, const QString &line)>;

void monitorLogcat(ProjectExplorer::RunControl *runControl, const LogcatLineHandler &onLine);

} // namespace Android::Internal
