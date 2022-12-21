// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runcontrol.h>

namespace RemoteLinux {
namespace Internal {

class RemoteLinuxQmlToolingSupport : public ProjectExplorer::SimpleTargetRunner
{
public:
    explicit RemoteLinuxQmlToolingSupport(ProjectExplorer::RunControl *runControl);
};

} // namespace Internal
} // namespace RemoteLinux
