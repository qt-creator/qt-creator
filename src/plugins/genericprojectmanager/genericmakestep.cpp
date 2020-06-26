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
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_BUILD) {
        setBuildTarget("all");
    } else if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        setBuildTarget("clean");
        setClean(true);
    }
    setAvailableBuildTargets({"all", "clean"});
}

GenericMakeStepFactory::GenericMakeStepFactory()
{
    registerStep<GenericMakeStep>(Constants::GENERIC_MS_ID);
    setDisplayName(MakeStep::defaultDisplayName());
    setSupportedProjectType(Constants::GENERICPROJECT_ID);
}

} // namespace Internal
} // namespace GenericProjectManager
