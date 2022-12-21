// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericmakestep.h"
#include "genericprojectconstants.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>

using namespace ProjectExplorer;

namespace GenericProjectManager {
namespace Internal {

class GenericMakeStep : public ProjectExplorer::MakeStep
{
public:
    explicit GenericMakeStep(BuildStepList *parent, Utils::Id id);
};

GenericMakeStep::GenericMakeStep(BuildStepList *parent, Utils::Id id)
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

GenericMakeStepFactory::GenericMakeStepFactory()
{
    registerStep<GenericMakeStep>(Constants::GENERIC_MS_ID);
    setDisplayName(MakeStep::defaultDisplayName());
    setSupportedProjectType(Constants::GENERICPROJECT_ID);
}

} // namespace Internal
} // namespace GenericProjectManager
