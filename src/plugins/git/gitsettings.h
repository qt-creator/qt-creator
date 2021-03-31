/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>
#include <vcsbase/vcsbaseclientsettings.h>

namespace Git {
namespace Internal {

enum CommitType
{
    SimpleCommit,
    AmendCommit,
    FixupCommit
};

// Todo: Add user name and password?
class GitSettings : public VcsBase::VcsBaseSettings
{
    Q_DECLARE_TR_FUNCTIONS(Git::Internal::GitSettings)

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

    Utils::FilePath gitExecutable(bool *ok = nullptr, QString *errorMessage = nullptr) const;
};

class GitSettingsPage final : public Core::IOptionsPage
{
public:
    explicit GitSettingsPage(GitSettings *settings);
};

} // namespace Internal
} // namespace Git
