// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildstep.h>

namespace RemoteLinux::Internal {

class KillAppStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    KillAppStepFactory();
};

} // RemoteLinux::Internal
