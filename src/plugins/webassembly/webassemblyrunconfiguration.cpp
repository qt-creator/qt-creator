/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

static FilePath pythonInterpreter(const Environment &env)
{
    const QString emsdkPythonEnvVarKey("EMSDK_PYTHON");
    if (env.hasKey(emsdkPythonEnvVarKey))
        return FilePath::fromUserInput(env.value(emsdkPythonEnvVarKey));

    // FIXME: Centralize addPythonsFromPath() from the Python plugin and use that
    for (const char *interpreterCandidate : {"python3", "python", "python2"}) {
        const FilePath interpereter = env.searchInPath(QLatin1String(interpreterCandidate));
        if (interpereter.isExecutableFile())
            return interpereter;
    }
    return {};
}

static CommandLine emrunCommand(Target *target, const QString &browser, const QString &port)
{
    if (BuildConfiguration *bc = target->activeBuildConfiguration()) {
        const Environment env = bc->environment();
        const FilePath emrun = env.searchInPath("emrun");
        const FilePath emrunPy = emrun.absolutePath().pathAppended(emrun.baseName() + ".py");
        const FilePath html =
                bc->buildDirectory().pathAppended(target->project()->displayName() + ".html");

        return CommandLine(pythonInterpreter(env), {
                emrunPy.path(),
                "--browser", browser,
                "--port", port,
                "--no_emrun_detect",
                "--serve_after_close",
                html.toString()
            });
    }
    return {};
}

// Runs a webassembly application via emscripten's "emrun" tool
// https://emscripten.org/docs/compiling/Running-html-files-with-emrun.html
class EmrunRunConfiguration : public ProjectExplorer::RunConfiguration
{
public:
    EmrunRunConfiguration(Target *target, Utils::Id id)
            : RunConfiguration(target, id)
    {
        auto webBrowserAspect = addAspect<WebBrowserSelectionAspect>(target);

        auto effectiveEmrunCall = addAspect<StringAspect>();
        effectiveEmrunCall->setLabelText(EmrunRunConfigurationFactory::tr("Effective emrun call:"));
        effectiveEmrunCall->setDisplayStyle(StringAspect::TextEditDisplay);
        effectiveEmrunCall->setReadOnly(true);

        setUpdater([target, effectiveEmrunCall, webBrowserAspect] {
            effectiveEmrunCall->setValue(emrunCommand(target,
                                                      webBrowserAspect->currentBrowser(),
                                                      "<port>").toUserOutput());
        });

        update(); // FIXME: Looks spurious

        // FIXME: A case for acquaintSiblings?
        connect(webBrowserAspect, &WebBrowserSelectionAspect::changed,
                this, &RunConfiguration::update);
        // FIXME: Is wrong after active build config changes, but probably
        // not needed anyway.
        connect(target->activeBuildConfiguration(), &BuildConfiguration::buildDirectoryChanged,
                this, &RunConfiguration::update);
        connect(target->project(), &Project::displayNameChanged,
                this, &RunConfiguration::update);
    }
};

class EmrunRunWorker : public SimpleTargetRunner
{
public:
    EmrunRunWorker(RunControl *runControl)
        : SimpleTargetRunner(runControl)
    {
        auto portsGatherer = new PortsGatherer(runControl);
        addStartDependency(portsGatherer);

        setStarter([this, runControl, portsGatherer] {
            Runnable r;
            r.command = emrunCommand(runControl->target(),
                                     runControl->aspect<WebBrowserSelectionAspect>()->currentBrowser(),
                                     QString::number(portsGatherer->findEndPoint().port()));
            SimpleTargetRunner::doStart(r, {});
        });
    }
};

RunWorkerFactory::WorkerCreator makeEmrunWorker()
{
    return RunWorkerFactory::make<EmrunRunWorker>();
}

// Factories

EmrunRunConfigurationFactory::EmrunRunConfigurationFactory()
    : FixedRunConfigurationFactory(EmrunRunConfigurationFactory::tr("Launch with emrun"))
{
    registerRunConfiguration<EmrunRunConfiguration>(Constants::WEBASSEMBLY_RUNCONFIGURATION_EMRUN);
    addSupportedTargetDeviceType(Constants::WEBASSEMBLY_DEVICE_TYPE);
}

} // namespace Internal
} // namespace Webassembly
