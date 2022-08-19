// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runconfiguration.h>

namespace BareMetal {
namespace Internal {

class BareMetalRunConfigurationFactory final
        : public ProjectExplorer::RunConfigurationFactory
{
public:
    BareMetalRunConfigurationFactory();
};

class BareMetalCustomRunConfigurationFactory final
        : public ProjectExplorer::FixedRunConfigurationFactory
{
public:
    BareMetalCustomRunConfigurationFactory();
};

} // namespace Internal
} // namespace BareMetal
