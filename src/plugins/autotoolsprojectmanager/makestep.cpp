// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "makestep.h"
#include "autotoolsprojectconstants.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>

namespace AutotoolsProjectManager::Internal {

// MakeStep

class MakeStep : public ProjectExplorer::MakeStep
{
public:
    MakeStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id)
        : ProjectExplorer::MakeStep(bsl, id)
    {
        setAvailableBuildTargets({"all", "clean"});
        if (bsl->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
            setSelectedBuildTarget("clean");
            setIgnoreReturnValue(true);
        } else {
            setSelectedBuildTarget("all");
        }
    }
};

// MakeStepFactory

MakeStepFactory::MakeStepFactory()
{
    registerStep<MakeStep>(Constants::MAKE_STEP_ID);
    setDisplayName(ProjectExplorer::MakeStep::defaultDisplayName());
    setSupportedProjectType(Constants::AUTOTOOLS_PROJECT_ID);
}

} // AutotoolsProjectManager::Internal
