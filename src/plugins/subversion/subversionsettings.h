// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>
#include <vcsbase/vcsbaseclientsettings.h>

namespace Subversion {
namespace Internal {

class SubversionSettings : public VcsBase::VcsBaseSettings
{
    Q_DECLARE_TR_FUNCTIONS(Subversion::Internal::SubversionSettings)

public:
    SubversionSettings();

    bool hasAuthentication() const;

    Utils::BoolAspect useAuthentication;
    Utils::StringAspect password;
    Utils::BoolAspect spaceIgnorantAnnotation;
    Utils::BoolAspect diffIgnoreWhiteSpace;
    Utils::BoolAspect logVerbose;
};

class SubversionSettingsPage final : public Core::IOptionsPage
{
public:
    explicit SubversionSettingsPage(SubversionSettings *settings);
};

} // namespace Internal
} // namespace Subversion
