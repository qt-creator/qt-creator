// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "makestep.h"
#include "autotoolsprojectconstants.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/makestep.h>
#include <projectexplorer/projectexplorerconstants.h>

using namespace ProjectExplorer;

namespace AutotoolsProjectManager::Internal {

// MakeStep

class AutotoolsMakeStep final : public MakeStep
{
public:
    AutotoolsMakeStep(BuildStepList *bsl, Utils::Id id)
        : MakeStep(bsl, id)
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

class MakeStepFactory final : public BuildStepFactory
{
public:
    MakeStepFactory()
    {
        registerStep<AutotoolsMakeStep>(Constants::MAKE_STEP_ID);
        setDisplayName(ProjectExplorer::MakeStep::defaultDisplayName());
        setSupportedProjectType(Constants::AUTOTOOLS_PROJECT_ID);
    }
};

void setupAutotoolsMakeStep()
{
    static MakeStepFactory theAutotoolsMakestepFactory;
}

} // AutotoolsProjectManager::Internal
