// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>
#include <vcsbase/vcsbaseclientsettings.h>

namespace Git::Internal {

enum CommitType
{
    SimpleCommit,
    AmendCommit,
    FixupCommit
};

// Todo: Add user name and password?
class GitSettings : public VcsBase::VcsBaseSettings
{
public:
    GitSettings();

    Utils::BoolAspect pullRebase;
    Utils::BoolAspect showTags;
    Utils::BoolAspect omitAnnotationDate;
    Utils::BoolAspect ignoreSpaceChangesInDiff;
    Utils::BoolAspect ignoreSpaceChangesInBlame;
    Utils::IntegerAspect blameMoveDetection;
    Utils::BoolAspect diffPatience;
    Utils::BoolAspect winSetHomeEnvironment;
    Utils::StringAspect gitkOptions;
    Utils::BoolAspect logDiff;
    Utils::StringAspect repositoryBrowserCmd;
    Utils::BoolAspect graphLog;
    Utils::BoolAspect colorLog;
    Utils::BoolAspect firstParent;
    Utils::BoolAspect followRenames;
    Utils::IntegerAspect lastResetIndex;
    Utils::BoolAspect refLogShowDate;
    Utils::BoolAspect instantBlame;

    mutable Utils::FilePath resolvedBinPath;
    mutable bool tryResolve = true;

    Utils::FilePath gitExecutable(bool *ok = nullptr, QString *errorMessage = nullptr) const;
};

GitSettings &settings();

class GitSettingsPage final : public Core::IOptionsPage
{
public:
    GitSettingsPage();
};

} // Git::Internal
