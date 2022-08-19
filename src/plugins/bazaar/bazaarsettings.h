// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <vcsbase/vcsbaseclientsettings.h>

namespace Bazaar {
namespace Internal {

class BazaarSettings final : public VcsBase::VcsBaseSettings
{
    Q_DECLARE_TR_FUNCTIONS(Bazaar::Internal::BazaarSettings)

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

} // namespace Internal
} // namespace Bazaar
