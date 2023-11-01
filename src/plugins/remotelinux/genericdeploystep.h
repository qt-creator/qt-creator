// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildstep.h>

namespace RemoteLinux::Internal {

class GenericDeployStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    GenericDeployStepFactory();
};

} // namespace RemoteLinux::Internal
