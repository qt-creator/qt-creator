// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildstep.h>

namespace McuSupport::Internal {

class MCUBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    MCUBuildStepFactory();

    static ProjectExplorer::Kit *findMostRecentQulKit();
    static void updateDeployStep(ProjectExplorer::Target *target, bool enabled);
};

} // namespace McuSupport::Internal
