// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseclientsettings.h>

namespace Git::Internal {

enum CommitType
{
    SimpleCommit,
    AmendCommit,
    FixupCommit
};

// Todo: Add user name and password?
class GitSettings final : public VcsBase::VcsBaseSettings
{
public:
    GitSettings();

    Utils::BoolAspect pullRebase{this};
    Utils::BoolAspect showTags{this};
    Utils::BoolAspect omitAnnotationDate{this};
    Utils::BoolAspect ignoreSpaceChangesInDiff{this};
    Utils::BoolAspect ignoreSpaceChangesInBlame{this};
    Utils::IntegerAspect blameMoveDetection{this};
    Utils::BoolAspect diffPatience{this};
    Utils::BoolAspect winSetHomeEnvironment{this};
    Utils::StringAspect gitkOptions{this};
    Utils::BoolAspect logDiff{this};
    Utils::FilePathAspect repositoryBrowserCmd{this};
    Utils::BoolAspect graphLog{this};
    Utils::BoolAspect colorLog{this};
    Utils::BoolAspect firstParent{this};
    Utils::BoolAspect followRenames{this};
    Utils::IntegerAspect lastResetIndex{this};
    Utils::BoolAspect refLogShowDate{this};
    Utils::BoolAspect instantBlame{this};
    Utils::BoolAspect instantBlameIgnoreSpaceChanges{this};
    Utils::BoolAspect instantBlameIgnoreLineMoves{this};

    mutable Utils::FilePath resolvedBinPath;
    mutable bool tryResolve = true;

    Utils::FilePath gitExecutable(bool *ok = nullptr, QString *errorMessage = nullptr) const;

    static QString trIgnoreWhitespaceChanges();
    static QString trIgnoreLineMoves();
};

GitSettings &settings();

} // Git::Internal
