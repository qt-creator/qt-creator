// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseclientsettings.h>

namespace Mercurial::Internal {

class MercurialSettings : public VcsBase::VcsBaseSettings
{
public:
    MercurialSettings();

    Utils::StringAspect diffIgnoreWhiteSpace;
    Utils::StringAspect diffIgnoreBlankLines;
};

MercurialSettings &settings();

} // Mercurial::Internal
