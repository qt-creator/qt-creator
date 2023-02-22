// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "buildstep.h"

namespace ProjectExplorer::Internal {

class CopyFileStepFactory final : public BuildStepFactory
{
public:
    CopyFileStepFactory();
};

class CopyDirectoryStepFactory final : public BuildStepFactory
{
public:
    CopyDirectoryStepFactory();
};

} // ProjectExplorer::Internal
