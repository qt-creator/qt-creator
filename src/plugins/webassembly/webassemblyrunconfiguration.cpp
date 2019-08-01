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

#include "webassemblyrunconfigurationaspects.h"
#include "webassemblyrunconfiguration.h"
#include "webassemblyconstants.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace WebAssembly {
namespace Internal {

static CommandLine emrunCommand(Target *target, const QString &browser, const QString &port)
{
    BuildConfiguration *bc = target->activeBuildConfiguration();
    const QFileInfo emrunScript = bc->environment().searchInPath("emrun").toFileInfo();
    auto html = bc->buildDirectory().pathAppended(target->project()->displayName() + ".html");

    return CommandLine(bc->environment().searchInPath("python"), {
            emrunScript.absolutePath() + "/" + emrunScript.baseName() + ".py",
            "--browser", browser,
            "--port", port,
            "--no_emrun_detect",
            html.toString()
        });
}

// Runs a webassembly application via emscripten's "emrun" tool
// https://emscripten.org/docs/compiling/Running-html-files-with-emrun.html
class EmrunRunConfiguration : public ProjectExplorer::RunConfiguration
{
public:
    EmrunRunConfiguration(Target *target, Core::Id id)
            : RunConfiguration(target, id)
    {
        auto webBrowserAspect = addAspect<WebBrowserSelectionAspect>(target);

        auto effectiveEmrunCall = addAspect<BaseStringAspect>();
        effectiveEmrunCall->setLabelText(tr("Effective emrun call:"));
        effectiveEmrunCall->setDisplayStyle(BaseStringAspect::TextEditDisplay);
        effectiveEmrunCall->setReadOnly(true);

        auto updateConfiguration = [target, effectiveEmrunCall, webBrowserAspect] {
            effectiveEmrunCall->setValue(emrunCommand(target,
                                                      webBrowserAspect->currentBrowser(),
                                                      "<port>").toUserOutput());
        };

        updateConfiguration();

        connect(webBrowserAspect, &WebBrowserSelectionAspect::changed,
                this, updateConfiguration);
        connect(target->activeBuildConfiguration(), &BuildConfiguration::buildDirectoryChanged,
                this, updateConfiguration);
    }
};

class EmrunRunWorker : public SimpleTargetRunner
{
public:
    EmrunRunWorker(RunControl *runControl)
        : SimpleTargetRunner(runControl)
    {
        m_portsGatherer = new PortsGatherer(runControl);
        addStartDependency(m_portsGatherer);
    }

    void start() final
    {
        CommandLine cmd = emrunCommand(runControl()->target(),
                                       runControl()->aspect<WebBrowserSelectionAspect>()->currentBrowser(),
                                       m_portsGatherer->findPort().toString());
        Runnable r;
        r.setCommandLine(cmd);
        setRunnable(r);

        SimpleTargetRunner::start();
    }

    PortsGatherer *m_portsGatherer;
};


// Factories

EmrunRunConfigurationFactory::EmrunRunConfigurationFactory()
    : FixedRunConfigurationFactory(EmrunRunConfiguration::tr("Launch with emrun"))
{
    registerRunConfiguration<EmrunRunConfiguration>(Constants::WEBASSEMBLY_RUNCONFIGURATION_EMRUN);
    addSupportedTargetDeviceType(Constants::WEBASSEMBLY_DEVICE_TYPE);
}

EmrunRunWorkerFactory::EmrunRunWorkerFactory()
{
    setProducer([](RunControl *rc) { return new EmrunRunWorker(rc); });
    addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
    addSupportedRunConfiguration(Constants::WEBASSEMBLY_RUNCONFIGURATION_EMRUN);
}

} // namespace Internal
} // namespace Webassembly
