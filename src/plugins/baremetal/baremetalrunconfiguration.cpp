/****************************************************************************
**
** Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
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

#include "baremetalrunconfiguration.h"

#include "baremetalcustomrunconfiguration.h"
#include "baremetalrunconfigurationwidget.h"

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtoutputformatter.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal {
namespace Internal {

BareMetalRunConfiguration::BareMetalRunConfiguration(Target *target)
    : RunConfiguration(target, IdPrefix)
{
    addExtraAspect(new ArgumentsAspect(this, "Qt4ProjectManager.MaemoRunConfiguration.Arguments"));
    addExtraAspect(new WorkingDirectoryAspect(this, "BareMetal.RunConfig.WorkingDirectory"));

    connect(target, &Target::deploymentDataChanged,
            this, &BareMetalRunConfiguration::handleBuildSystemDataUpdated);
    connect(target, &Target::applicationTargetsChanged,
            this, &BareMetalRunConfiguration::handleBuildSystemDataUpdated);
    connect(target, &Target::kitChanged,
            this, &BareMetalRunConfiguration::handleBuildSystemDataUpdated); // Handles device changes, etc.
}

QString BareMetalRunConfiguration::extraId() const
{
    return m_buildKey;
}

void BareMetalRunConfiguration::doAdditionalSetup(const RunConfigurationCreationInfo &info)
{
    m_buildKey = info.buildKey;
    setDefaultDisplayName(info.displayName);
}

QWidget *BareMetalRunConfiguration::createConfigurationWidget()
{
    return new BareMetalRunConfigurationWidget(this);
}

OutputFormatter *BareMetalRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

QVariantMap BareMetalRunConfiguration::toMap() const
{
    return RunConfiguration::toMap();
}

bool BareMetalRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    m_buildKey = ProjectExplorer::idFromMap(map).suffixAfter(id());

    return true;
}

QString BareMetalRunConfiguration::localExecutableFilePath() const
{
    const BuildTargetInfo bti = target()->applicationTargets().buildTargetInfo(m_buildKey);
    return bti.targetFilePath.toString();
}

QString BareMetalRunConfiguration::buildSystemTarget() const
{
    const BuildTargetInfo bti = target()->applicationTargets().buildTargetInfo(m_buildKey);
    return bti.targetName;
}

void BareMetalRunConfiguration::handleBuildSystemDataUpdated()
{
    emit targetInformationChanged();
    emit enabledChanged();
}

const char *BareMetalRunConfiguration::IdPrefix = "BareMetal";

} // namespace Internal
} // namespace BareMetal

