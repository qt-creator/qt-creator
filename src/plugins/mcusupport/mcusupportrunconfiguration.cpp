/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "mcusupportrunconfiguration.h"
#include "mcusupportconstants.h"

#include <projectexplorer/projectconfigurationaspects.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace McuSupport {
namespace Internal {

static CommandLine flashAndRunCommand(Target *target)
{
    BuildConfiguration *bc = target->activeBuildConfiguration();

    return CommandLine(bc->environment().searchInPath("cmake"), {});
}

class FlashAndRunConfiguration : public ProjectExplorer::RunConfiguration
{
public:
    FlashAndRunConfiguration(Target *target, Core::Id id)
        : RunConfiguration(target, id)
    {
        auto effectiveFlashAndRunCall = addAspect<BaseStringAspect>();
        effectiveFlashAndRunCall->setLabelText(tr("Effective flash and run call:"));
        effectiveFlashAndRunCall->setDisplayStyle(BaseStringAspect::TextEditDisplay);
        effectiveFlashAndRunCall->setReadOnly(true);
    }
};

class FlashAndRunWorker : public SimpleTargetRunner
{
public:
    FlashAndRunWorker(RunControl *runControl)
        : SimpleTargetRunner(runControl)
    {
        setStarter([this, runControl] {
            CommandLine cmd = flashAndRunCommand(runControl->target());
            Runnable r;
            r.setCommandLine(cmd);
            SimpleTargetRunner::doStart(r, {});
        });
    }
};

RunWorkerFactory::WorkerCreator makeFlashAndRunWorker()
{
    return RunWorkerFactory::make<FlashAndRunWorker>();
}

EmrunRunConfigurationFactory::EmrunRunConfigurationFactory()
    : FixedRunConfigurationFactory(FlashAndRunConfiguration::tr("Flash and run"))
{
    registerRunConfiguration<FlashAndRunConfiguration>(Constants::RUNCONFIGURATION);
    addSupportedTargetDeviceType(Constants::DEVICE_TYPE);
}

} // namespace Internal
} // namespace McuSupport
