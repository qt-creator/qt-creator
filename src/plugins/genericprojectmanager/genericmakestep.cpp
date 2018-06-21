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

#include <projectexplorer/projectexplorerconstants.h>

using namespace ProjectExplorer;

namespace GenericProjectManager {
namespace Internal {

const char GENERIC_MS_ID[] = "GenericProjectManager.GenericMakeStep";

GenericMakeStep::GenericMakeStep(BuildStepList *parent, const QString &buildTarget)
    : MakeStep(parent, GENERIC_MS_ID, buildTarget, {"all", "clean"})
{
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
    setDisplayName(MakeStep::defaultDisplayName());
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
    setDisplayName(MakeStep::defaultDisplayName());
    setSupportedProjectType(Constants::GENERICPROJECT_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
}

} // namespace Internal
} // namespace GenericProjectManager
