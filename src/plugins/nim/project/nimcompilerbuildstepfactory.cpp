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

#include "project/nimcompilerbuildstepfactory.h"

#include "nimconstants.h"
#include "project/nimbuildconfiguration.h"
#include "project/nimcompilerbuildstep.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

#include <memory>

using namespace ProjectExplorer;

namespace Nim {

NimCompilerBuildStepFactory::NimCompilerBuildStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{}

bool NimCompilerBuildStepFactory::canCreate(BuildStepList *parent, Core::Id id) const
{
    QTC_ASSERT(parent, return false);
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        return false;
    if (!qobject_cast<NimBuildConfiguration *>(parent->parent()))
        return false;
    return id == Constants::C_NIMCOMPILERBUILDSTEP_ID;
}

BuildStep *NimCompilerBuildStepFactory::create(BuildStepList *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return nullptr;
    return new NimCompilerBuildStep(parent);
}

bool NimCompilerBuildStepFactory::canClone(BuildStepList *parent, BuildStep *buildStep) const
{
    QTC_ASSERT(parent, return false);
    QTC_ASSERT(buildStep, return false);
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        return false;
    if (!qobject_cast<NimBuildConfiguration *>(parent->parent()))
        return false;
    return buildStep->id() == Constants::C_NIMCOMPILERBUILDSTEP_ID;
}

BuildStep *NimCompilerBuildStepFactory::clone(BuildStepList *parent, BuildStep *buildStep)
{
    QTC_ASSERT(parent, return nullptr);
    QTC_ASSERT(buildStep, return nullptr);
    std::unique_ptr<NimCompilerBuildStep> result(new NimCompilerBuildStep(parent));
    return result->fromMap(buildStep->toMap()) ? result.release() : nullptr;
}

bool NimCompilerBuildStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *NimCompilerBuildStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return nullptr;
    std::unique_ptr<NimCompilerBuildStep> buildStep(new NimCompilerBuildStep(parent));
    if (buildStep->fromMap(map))
        return buildStep.release();
    return nullptr;
}

QList<Core::Id> NimCompilerBuildStepFactory::availableCreationIds(BuildStepList *parent) const
{
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        if (auto bc = qobject_cast<NimBuildConfiguration *>(parent->parent()))
            if (!bc->hasNimCompilerBuildStep())
                return { Constants::C_NIMCOMPILERBUILDSTEP_ID };
    return {};
}

QString NimCompilerBuildStepFactory::displayNameForId(Core::Id id) const
{
    return id == Constants::C_NIMCOMPILERBUILDSTEP_ID ? tr("Nim Compiler Build Step") : QString();
}

}

