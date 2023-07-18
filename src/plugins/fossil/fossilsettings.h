// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseclientsettings.h>

namespace Fossil::Internal {

class FossilSettings final : public VcsBase::VcsBaseSettings
{
public:
    FossilSettings();

    Utils::FilePathAspect defaultRepoPath{this};
    Utils::FilePathAspect sslIdentityFile{this};
    Utils::BoolAspect diffIgnoreAllWhiteSpace{this};
    Utils::BoolAspect diffStripTrailingCR{this};
    Utils::BoolAspect annotateShowCommitters{this};
    Utils::BoolAspect annotateListVersions{this};
    Utils::IntegerAspect timelineWidth{this};
    Utils::StringAspect timelineLineageFilter{this};
    Utils::BoolAspect timelineVerbose{this};
    Utils::StringAspect timelineItemType{this};
    Utils::BoolAspect disableAutosync{this};
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
