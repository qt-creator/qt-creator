// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxanalyzesupport.h"

#include "qnxconstants.h"
#include "qnxtr.h"
#include "slog2inforunner.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>

#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qnx::Internal {

class QnxQmlProfilerWorkerFactory final : public RunWorkerFactory
{
public:
    QnxQmlProfilerWorkerFactory()
    {
        setProducer([](RunControl *runControl) {
            runControl->requestQmlChannel();

            const auto modifier = [runControl](Process &process) {
                CommandLine cmd = runControl->commandLine();
                cmd.addArg(qmlDebugTcpArguments(QmlProfilerServices, runControl->qmlChannel()));
                process.setCommand(cmd);
            };

            auto worker = createProcessWorker(runControl, modifier);
            runControl->postMessage(Tr::tr("Preparing remote side..."), LogMessageFormat);

            auto slog2InfoRunner = new RunWorker(runControl, slog2InfoRecipe(runControl));

            auto profiler = runControl->createWorker(ProjectExplorer::Constants::QML_PROFILER_RUNNER);
            profiler->addStartDependency(worker);
            worker->addStartDependency(slog2InfoRunner);
            worker->addStopDependency(profiler);
            return worker;
        });
        // FIXME: Shouldn't this use the run mode id somehow?
        addSupportedRunConfig(Constants::QNX_RUNCONFIG_ID);
    }
};

void setupQnxQmlProfiler()
{
    static QnxQmlProfilerWorkerFactory theQnxQmlProfilerWorkerFactory;
}

} // Qnx::Internal
