/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "project/nimcompilercleanstepfactory.h"

#include "nimconstants.h"
#include "project/nimbuildconfiguration.h"
#include "project/nimcompilercleanstep.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <memory>

using namespace ProjectExplorer;

namespace Nim {

NimCompilerCleanStepFactory::NimCompilerCleanStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{}

bool NimCompilerCleanStepFactory::canCreate(BuildStepList *parent, Core::Id id) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_CLEAN)
        return false;
    if (!qobject_cast<NimBuildConfiguration *>(parent->parent()))
        return false;
    return id == Constants::C_NIMCOMPILERCLEANSTEP_ID;
}

BuildStep *NimCompilerCleanStepFactory::create(BuildStepList *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return nullptr;
    return new NimCompilerCleanStep(parent);
}

bool NimCompilerCleanStepFactory::canClone(BuildStepList *, BuildStep *) const
{
    return false;
}

BuildStep *NimCompilerCleanStepFactory::clone(BuildStepList *, BuildStep *)
{
    return nullptr;
}

bool NimCompilerCleanStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *NimCompilerCleanStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return nullptr;
    std::unique_ptr<NimCompilerCleanStep> cleanStep(new NimCompilerCleanStep(parent));
    if (cleanStep->fromMap(map))
        return cleanStep.release();
    return nullptr;
}

QList<Core::Id> NimCompilerCleanStepFactory::availableCreationIds(BuildStepList *parent) const
{
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN)
        if (auto bc = qobject_cast<NimBuildConfiguration *>(parent->parent()))
            if (!bc->hasNimCompilerCleanStep())
                return { Constants::C_NIMCOMPILERCLEANSTEP_ID };
    return {};
}

QString NimCompilerCleanStepFactory::displayNameForId(Core::Id id) const
{
    return id == Constants::C_NIMCOMPILERCLEANSTEP_ID ? tr(Nim::Constants::C_NIMCOMPILERCLEANSTEP_DISPLAY) : QString();
}

}
