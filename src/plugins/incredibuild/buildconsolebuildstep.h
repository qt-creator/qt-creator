// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildstep.h>

namespace IncrediBuild::Internal {

class BuildConsoleStepFactory final : public ProjectExplorer::BuildStepFactory
{
public:
    BuildConsoleStepFactory();
};

} // IncrediBuild::Internal
