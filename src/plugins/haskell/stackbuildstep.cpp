// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stackbuildstep.h"

#include "haskellconstants.h"
#include "haskellsettings.h"
#include "haskelltr.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>

using namespace ProjectExplorer;

namespace Haskell {
namespace Internal {

StackBuildStep::StackBuildStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id)
    : AbstractProcessStep(bsl, id)
{
    setDefaultDisplayName(trDisplayName());
}

QWidget *StackBuildStep::createConfigWidget()
{
    return new QWidget;
}

QString StackBuildStep::trDisplayName()
{
    return Tr::tr("Stack Build");
}

bool StackBuildStep::init()
{
    if (AbstractProcessStep::init()) {
        const auto projectDir = QDir(project()->projectDirectory().toString());
        processParameters()->setCommandLine(
            {settings().stackPath(),
             {"build", "--work-dir", projectDir.relativeFilePath(buildDirectory().toString())}});
        processParameters()->setEnvironment(buildEnvironment());
    }
    return true;
}

StackBuildStepFactory::StackBuildStepFactory()
{
    registerStep<StackBuildStep>(Constants::C_STACK_BUILD_STEP_ID);
    setDisplayName(StackBuildStep::StackBuildStep::trDisplayName());
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
}

} // namespace Internal
} // namespace Haskell
