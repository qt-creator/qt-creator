// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <vcsbase/vcsbaseclientsettings.h>

namespace Cvs::Internal {

class CvsSettings : public VcsBase::VcsBaseSettings
{
public:
    CvsSettings();

    Utils::StringAspect cvsRoot;
    Utils::StringAspect diffOptions;
    Utils::BoolAspect diffIgnoreWhiteSpace;
    Utils::BoolAspect diffIgnoreBlankLines;
    Utils::BoolAspect describeByCommitId;

    QStringList addOptions(const QStringList &args) const;
};

CvsSettings &settings();

class CvsSettingsPage final : public Core::IOptionsPage
{
public:
    CvsSettingsPage();
};

} // Cvs::Internal
