// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <vcsbase/vcsbaseclientsettings.h>

namespace Mercurial {
namespace Internal {

class MercurialSettings : public VcsBase::VcsBaseSettings
{
    Q_DECLARE_TR_FUNCTIONS(Mercurial::Internal::MercurialSettings)

public:
    Utils::StringAspect diffIgnoreWhiteSpace;
    Utils::StringAspect diffIgnoreBlankLines;

    MercurialSettings();
};

class MercurialSettingsPage final : public Core::IOptionsPage
{
public:
    explicit MercurialSettingsPage(MercurialSettings *settings);
};

} // namespace Internal
} // namespace Mercurial
