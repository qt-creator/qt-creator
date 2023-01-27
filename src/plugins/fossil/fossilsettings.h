// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>
#include <vcsbase/vcsbaseclientsettings.h>

namespace Fossil {
namespace Internal {

class FossilSettings : public VcsBase::VcsBaseSettings
{
public:
    Utils::StringAspect defaultRepoPath;
    Utils::StringAspect sslIdentityFile;
    Utils::BoolAspect diffIgnoreAllWhiteSpace;
    Utils::BoolAspect diffStripTrailingCR;
    Utils::BoolAspect annotateShowCommitters;
    Utils::BoolAspect annotateListVersions;
    Utils::IntegerAspect timelineWidth;
    Utils::StringAspect timelineLineageFilter;
    Utils::BoolAspect timelineVerbose;
    Utils::StringAspect timelineItemType;
    Utils::BoolAspect disableAutosync;

    FossilSettings();
};

struct RepositorySettings
{
    enum AutosyncMode {AutosyncOff, AutosyncOn, AutosyncPullOnly};

    QString user;
    QString sslIdentityFile;
    AutosyncMode autosync = AutosyncOn;
};

inline bool operator==(const RepositorySettings &lh, const RepositorySettings &rh)
{
    return (lh.user == rh.user &&
            lh.sslIdentityFile == rh.sslIdentityFile &&
            lh.autosync == rh.autosync);
}

class OptionsPage : public Core::IOptionsPage
{
public:
    OptionsPage(const std::function<void()> &onApply, FossilSettings *settings);
};

} // namespace Internal
} // namespace Fossil
