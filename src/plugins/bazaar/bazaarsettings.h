// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <vcsbase/vcsbaseclientsettings.h>

namespace Bazaar::Internal {

class BazaarSettings final : public VcsBase::VcsBaseSettings
{
public:
    BazaarSettings();

    Utils::BoolAspect diffIgnoreWhiteSpace;
    Utils::BoolAspect diffIgnoreBlankLines;
    Utils::BoolAspect logVerbose;
    Utils::BoolAspect logForward;
    Utils::BoolAspect logIncludeMerges;
    Utils::StringAspect logFormat;
};

class BazaarSettingsPage final : public Core::IOptionsPage
{
public:
    explicit BazaarSettingsPage(BazaarSettings *settings);
};

} // Bazaar::Internal
