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

#include "baremetalcustomrunconfiguration.h"

#include "baremetalconstants.h"

#include <projectexplorer/target.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <qtsupport/qtoutputformatter.h>

using namespace Utils;
using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

BareMetalCustomRunConfiguration::BareMetalCustomRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    auto exeAspect = addAspect<ExecutableAspect>();
    exeAspect->setSettingsKey("BareMetal.CustomRunConfig.Executable");
    exeAspect->setPlaceHolderText(tr("Unknown"));
    exeAspect->setDisplayStyle(BaseStringAspect::PathChooserDisplay);
    exeAspect->setHistoryCompleter("BareMetal.CustomRunConfig.History");
    exeAspect->setExpectedKind(PathChooser::Any);

    addAspect<ArgumentsAspect>();
    addAspect<WorkingDirectoryAspect>(nullptr);

    setDefaultDisplayName(RunConfigurationFactory::decoratedTargetName(tr("Custom Executable"), target));
}

const char *BareMetalCustomRunConfiguration::Id = "BareMetal";

bool BareMetalCustomRunConfiguration::isConfigured() const
{
    return !aspect<ExecutableAspect>()->executable().isEmpty();
}

RunConfiguration::ConfigurationState
BareMetalCustomRunConfiguration::ensureConfigured(QString *errorMessage)
{
    if (!isConfigured()) {
        if (errorMessage) {
            *errorMessage = tr("The remote executable must be set "
                               "in order to run a custom remote run configuration.");
        }
        return UnConfigured;
    }
    return Configured;
}

// BareMetalCustomRunConfigurationFactory

BareMetalCustomRunConfigurationFactory::BareMetalCustomRunConfigurationFactory()
    : FixedRunConfigurationFactory(BareMetalCustomRunConfiguration::tr("Custom Executable"), true)
{
    registerRunConfiguration<BareMetalCustomRunConfiguration>(BareMetalCustomRunConfiguration::Id);
    addSupportedTargetDeviceType(BareMetal::Constants::BareMetalOsType);
}

} // namespace Internal
} // namespace BareMetal
