// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericmakestep.h"

#include "genericprojectconstants.h"

#include <projectexplorer/makestep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace GenericProjectManager::Internal {

class GenericMakeStep final : public MakeStep
{
public:
    GenericMakeStep(BuildStepList *parent, Id id)
        : MakeStep(parent, id)
    {
        setAvailableBuildTargets({"all", "clean"});
        if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_BUILD) {
            setSelectedBuildTarget("all");
        } else if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
            setSelectedBuildTarget("clean");
            setIgnoreReturnValue(true);
        }
    }
};

class GenericMakeStepFactory final : public BuildStepFactory
{
public:
    GenericMakeStepFactory()
    {
        registerStep<GenericMakeStep>(Constants::GENERIC_MS_ID);
        setDisplayName(MakeStep::defaultDisplayName());
        setSupportedProjectType(Constants::GENERICPROJECT_ID);
    }
};

void setupGenericMakeStep()
{
    static GenericMakeStepFactory theGenericMakeStepFactory;
}

} // GenericProjectManager::Internal
