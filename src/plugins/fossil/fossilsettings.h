// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseclientsettings.h>

namespace Fossil::Internal {

class FossilSettings : public VcsBase::VcsBaseSettings
{
public:
    FossilSettings();

    Utils::FilePathAspect defaultRepoPath;
    Utils::FilePathAspect sslIdentityFile;
    Utils::BoolAspect diffIgnoreAllWhiteSpace;
    Utils::BoolAspect diffStripTrailingCR;
    Utils::BoolAspect annotateShowCommitters;
    Utils::BoolAspect annotateListVersions;
    Utils::IntegerAspect timelineWidth;
    Utils::StringAspect timelineLineageFilter;
    Utils::BoolAspect timelineVerbose;
    Utils::StringAspect timelineItemType;
    Utils::BoolAspect disableAutosync;
};

FossilSettings &settings();

struct RepositorySettings
{
    enum AutosyncMode {AutosyncOff, AutosyncOn, AutosyncPullOnly};

    QString user;
    QString sslIdentityFile;
    AutosyncMode autosync = AutosyncOn;

    friend bool operator==(const RepositorySettings &lh, const RepositorySettings &rh)
    {
        return lh.user == rh.user
            && lh.sslIdentityFile == rh.sslIdentityFile
            && lh.autosync == rh.autosync;
    }
};

} // Fossil::Internal
