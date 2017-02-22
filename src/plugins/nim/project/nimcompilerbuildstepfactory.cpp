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

#include "nimbuildconfiguration.h"
#include "nimcompilerbuildstep.h"
#include "nimcompilerbuildstepfactory.h"

#include "../nimconstants.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

#include <memory>

using namespace ProjectExplorer;

namespace Nim {

NimCompilerBuildStepFactory::NimCompilerBuildStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{}

QList<BuildStepInfo> NimCompilerBuildStepFactory::availableSteps(BuildStepList *parent) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        return {};

    auto bc = qobject_cast<NimBuildConfiguration *>(parent->parent());
    if (!bc || bc->hasNimCompilerBuildStep())
        return {};

    return {{Constants::C_NIMCOMPILERBUILDSTEP_ID, tr("Nim Compiler Build Step")}};
}

BuildStep *NimCompilerBuildStepFactory::create(BuildStepList *parent, Core::Id)
{
    return new NimCompilerBuildStep(parent);
}

BuildStep *NimCompilerBuildStepFactory::clone(BuildStepList *parent, BuildStep *buildStep)
{
    QTC_ASSERT(parent, return nullptr);
    QTC_ASSERT(buildStep, return nullptr);
    std::unique_ptr<NimCompilerBuildStep> result(new NimCompilerBuildStep(parent));
    return result->fromMap(buildStep->toMap()) ? result.release() : nullptr;
}

}

