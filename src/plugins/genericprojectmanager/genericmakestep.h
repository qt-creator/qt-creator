// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/makestep.h>

namespace GenericProjectManager {
namespace Internal {

class GenericMakeStepFactory final : public ProjectExplorer::BuildStepFactory
{
public:
    GenericMakeStepFactory();
};

} // namespace Internal
} // namespace GenericProjectManager
