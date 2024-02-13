// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stackbuildstep.h"

#include "haskellconstants.h"
#include "haskellsettings.h"
#include "haskelltr.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>

using namespace ProjectExplorer;

namespace Haskell::Internal {

static QString trDisplayName()
{
    return Tr::tr("Stack Build");
}

class StackBuildStep final : public AbstractProcessStep
{
public:
    StackBuildStep(BuildStepList *bsl, Utils::Id id)
        : AbstractProcessStep(bsl, id)
    {
        setDefaultDisplayName(trDisplayName());
    }

    QWidget *createConfigWidget() final
    {
        return new QWidget;
    }

    bool init() final
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
};

class StackBuildStepFactory final : public BuildStepFactory
{
public:
    StackBuildStepFactory()
    {
        registerStep<StackBuildStep>(Constants::C_STACK_BUILD_STEP_ID);
        setDisplayName(trDisplayName());
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    }
};

void setupHaskellStackBuildStep()
{
    static StackBuildStepFactory theStackBuildStepFactory;
}

} // Haskell::Internal
