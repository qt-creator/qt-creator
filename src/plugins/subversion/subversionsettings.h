// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseclientsettings.h>

namespace Subversion::Internal {

class SubversionSettings final : public VcsBase::VcsBaseSettings
{
public:
    SubversionSettings();

    bool hasAuthentication() const;

    Utils::BoolAspect useAuthentication{this};
    Utils::StringAspect password{this};
    Utils::BoolAspect spaceIgnorantAnnotation{this};
    Utils::BoolAspect diffIgnoreWhiteSpace{this};
    Utils::BoolAspect logVerbose{this};
};

SubversionSettings &settings();

} // Subversion::Internal
