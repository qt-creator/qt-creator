/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "stackbuildstep.h"

#include "haskellconstants.h"
#include "haskellmanager.h"

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
    return tr("Stack Build");
}

bool StackBuildStep::init()
{
    if (AbstractProcessStep::init()) {
        const auto projectDir = QDir(project()->projectDirectory().toString());
        processParameters()->setCommandLine(
            {HaskellManager::stackExecutable(),
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
