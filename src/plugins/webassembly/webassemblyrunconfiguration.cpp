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
#include <projectexplorer/buildsystem.h>
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

static CommandLine emrunCommand(const RunConfiguration *rc, const QString &browser,
                                const QString &port)
{
    if (BuildConfiguration *bc = rc->target()->activeBuildConfiguration()) {
        const Environment env = bc->environment();
        const FilePath emrun = env.searchInPath("emrun");
        const FilePath emrunPy = emrun.absolutePath().pathAppended(emrun.baseName() + ".py");
        const FilePath target = rc->buildTargetInfo().targetFilePath;
        const FilePath html = target.absolutePath() / target.baseName() + ".html";

        QStringList args(emrunPy.path());
        if (!browser.isEmpty()) {
            args.append("--browser");
            args.append(browser);
        }
        args.append("--port");
        args.append(port);
        args.append("--no_emrun_detect");
        args.append("--serve_after_close");
        args.append(html.toString());

        return CommandLine(pythonInterpreter(env), args);
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

        setUpdater([this, effectiveEmrunCall, webBrowserAspect] {
            effectiveEmrunCall->setValue(emrunCommand(this,
                                                      webBrowserAspect->currentBrowser(),
                                                      "<port>").toUserOutput());
        });

        connect(webBrowserAspect, &BaseAspect::changed, this, &RunConfiguration::update);
        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
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
            const QString browserId =
                    runControl->aspect<WebBrowserSelectionAspect>()->currentBrowser;
            r.command = emrunCommand(runControl->runConfiguration(),
                                     browserId,
                                     QString::number(portsGatherer->findEndPoint().port()));
            SimpleTargetRunner::doStart(r);
        });
    }
};

RunWorkerFactory::WorkerCreator makeEmrunWorker()
{
    return RunWorkerFactory::make<EmrunRunWorker>();
}

// Factories

EmrunRunConfigurationFactory::EmrunRunConfigurationFactory()
{
    registerRunConfiguration<EmrunRunConfiguration>(Constants::WEBASSEMBLY_RUNCONFIGURATION_EMRUN);
    addSupportedTargetDeviceType(Constants::WEBASSEMBLY_DEVICE_TYPE);
}

} // namespace Internal
} // namespace Webassembly
