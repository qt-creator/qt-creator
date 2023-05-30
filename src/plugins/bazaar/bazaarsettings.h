// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseclientsettings.h>

namespace Bazaar::Internal {

class BazaarSettings final : public VcsBase::VcsBaseSettings
{
public:
    BazaarSettings();

    Utils::BoolAspect diffIgnoreWhiteSpace{this};
    Utils::BoolAspect diffIgnoreBlankLines{this};
    Utils::BoolAspect logVerbose{this};
    Utils::BoolAspect logForward{this};
    Utils::BoolAspect logIncludeMerges{this};
    Utils::StringAspect logFormat{this};
};

BazaarSettings &settings();

} // Bazaar::Internal
