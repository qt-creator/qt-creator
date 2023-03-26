// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildstep.h>

namespace AutotoolsProjectManager::Internal {

class ConfigureStepFactory final : public ProjectExplorer::BuildStepFactory
{
public:
    ConfigureStepFactory();
};

} // AutotoolsProjectManager::Internal
