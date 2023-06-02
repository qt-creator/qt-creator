// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/buildstep.h>

namespace RemoteLinux {

class REMOTELINUX_EXPORT RsyncDeployStepFactory
    : public ProjectExplorer::BuildStepFactory
{
public:
    RsyncDeployStepFactory();
};

} // namespace RemoteLinux
