// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <vcsbase/vcsbaseclientsettings.h>

namespace Cvs {
namespace Internal {

class CvsSettings : public VcsBase::VcsBaseSettings
{
    Q_DECLARE_TR_FUNCTIONS(Cvs::Internal::SettingsPage)

public:
    Utils::StringAspect cvsRoot;
    Utils::StringAspect diffOptions;
    Utils::BoolAspect diffIgnoreWhiteSpace;
    Utils::BoolAspect diffIgnoreBlankLines;
    Utils::BoolAspect describeByCommitId;

    CvsSettings();

    QStringList addOptions(const QStringList &args) const;
};

class CvsSettingsPage final : public Core::IOptionsPage
{
public:
    explicit CvsSettingsPage(CvsSettings *settings);
};

} // namespace Internal
} // namespace Cvs
