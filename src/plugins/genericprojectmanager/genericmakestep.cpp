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

#include "genericmakestep.h"
#include "genericprojectconstants.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;

namespace GenericProjectManager {
namespace Internal {

const char GENERIC_MS_ID[] = "GenericProjectManager.GenericMakeStep";
const char GENERIC_MS_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("GenericProjectManager::Internal::GenericMakeStep",
                                                     "Make");

GenericMakeStep::GenericMakeStep(BuildStepList *parent, const QString &buildTarget)
    : MakeStep(parent, GENERIC_MS_ID, buildTarget, {"all", "clean"})
{
    setDefaultDisplayName(QCoreApplication::translate("GenericProjectManager::Internal::GenericMakeStep",
                                                      GENERIC_MS_DISPLAY_NAME));
}

bool GenericMakeStep::init(QList<const BuildStep *> &earlierSteps)
{
    BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        bc = target()->activeBuildConfiguration();
    if (!bc)
        emit addTask(Task::buildConfigurationMissingTask());

    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    if (!tc)
        emit addTask(Task::compilerMissingTask());

    if (!bc || !tc) {
        emitFaultyConfigurationMessage();
        return false;
    }

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    Utils::Environment env = bc->environment();
    Utils::Environment::setupEnglishOutput(&env);
    pp->setEnvironment(env);
    pp->setCommand(effectiveMakeCommand());
    pp->setArguments(allArguments());
    pp->resolveAll();

    // If we are cleaning, then make can fail with an error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on an already clean project
    setIgnoreReturnValue(isClean());

    setOutputParser(new GnuMakeParser());
    IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        appendOutputParser(parser);
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init(earlierSteps);
}

//
// GenericMakeAllStepFactory
//

GenericMakeAllStepFactory::GenericMakeAllStepFactory()
{
    struct Step : GenericMakeStep
    {
        Step(BuildStepList *bsl) : GenericMakeStep(bsl, QString("all")) { }
    };

    registerStep<Step>(GENERIC_MS_ID);
    setDisplayName(QCoreApplication::translate(
        "GenericProjectManager::Internal::GenericMakeStep", GENERIC_MS_DISPLAY_NAME));
    setSupportedProjectType(Constants::GENERICPROJECT_ID);
    setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_BUILD,
                           ProjectExplorer::Constants::BUILDSTEPS_DEPLOY});
}

//
// GenericMakeCleanStepFactory
//

GenericMakeCleanStepFactory::GenericMakeCleanStepFactory()
{
    struct Step : GenericMakeStep
    {
        Step(BuildStepList *bsl) : GenericMakeStep(bsl)
        {
            setBuildTarget("clean", true);
            setClean(true);
        }
    };

    registerStep<Step>(GENERIC_MS_ID);
    setDisplayName(QCoreApplication::translate(
        "GenericProjectManager::Internal::GenericMakeStep", GENERIC_MS_DISPLAY_NAME));
    setSupportedProjectType(Constants::GENERICPROJECT_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
}

} // namespace Internal
} // namespace GenericProjectManager
