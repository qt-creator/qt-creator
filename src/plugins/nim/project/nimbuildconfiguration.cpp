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
#include "nimbuildconfigurationwidget.h"
#include "nimcompilerbuildstep.h"
#include "nimproject.h"

#include "../nimconstants.h"

#include <projectexplorer/namedwidget.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildsteplist.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimBuildConfiguration::NimBuildConfiguration(Target *target)
    : BuildConfiguration(target, Constants::C_NIMBUILDCONFIGURATION_ID)
{}

NamedWidget *NimBuildConfiguration::createConfigWidget()
{
    return new NimBuildConfigurationWidget(this);
}

BuildConfiguration::BuildType NimBuildConfiguration::buildType() const
{
    return BuildConfiguration::Unknown;
}

bool NimBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    if (!canRestore(map))
        return false;

    const QString displayName = map[Constants::C_NIMBUILDCONFIGURATION_DISPLAY_KEY].toString();
    const QString buildDirectory = map[Constants::C_NIMBUILDCONFIGURATION_BUILDDIRECTORY_KEY].toString();

    setDisplayName(displayName);
    setBuildDirectory(FileName::fromString(buildDirectory));

    return true;
}

QVariantMap NimBuildConfiguration::toMap() const
{
    QVariantMap result = BuildConfiguration::toMap();
    result[Constants::C_NIMBUILDCONFIGURATION_DISPLAY_KEY] = displayName();
    result[Constants::C_NIMBUILDCONFIGURATION_BUILDDIRECTORY_KEY] = buildDirectory().toString();
    return result;
}

FileName NimBuildConfiguration::cacheDirectory() const
{
    return buildDirectory().appendPath(QStringLiteral("nimcache"));
}

FileName NimBuildConfiguration::outFilePath() const
{
    const NimCompilerBuildStep *step = nimCompilerBuildStep();
    QTC_ASSERT(step, return FileName());
    return step->outFilePath();
}

bool NimBuildConfiguration::canRestore(const QVariantMap &map)
{
    return idFromMap(map) == Constants::C_NIMBUILDCONFIGURATION_ID;
}

bool NimBuildConfiguration::hasNimCompilerBuildStep() const
{
    BuildStepList *steps = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    return steps ? steps->contains(Constants::C_NIMCOMPILERBUILDSTEP_ID) : false;
}

bool NimBuildConfiguration::hasNimCompilerCleanStep() const
{
    BuildStepList *steps = stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    return steps ? steps->contains(Constants::C_NIMCOMPILERCLEANSTEP_ID) : false;
}

const NimCompilerBuildStep *NimBuildConfiguration::nimCompilerBuildStep() const
{
    BuildStepList *steps = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    QTC_ASSERT(steps, return nullptr);
    foreach (BuildStep *step, steps->steps())
        if (step->id() == Constants::C_NIMCOMPILERBUILDSTEP_ID)
            return qobject_cast<NimCompilerBuildStep *>(step);
    return nullptr;
}

}

