// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildstep.h>

namespace CMakeProjectManager::Internal {

class CMakeInstallStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    CMakeInstallStepFactory();
};

} // CMakeProjectManager::Internal
