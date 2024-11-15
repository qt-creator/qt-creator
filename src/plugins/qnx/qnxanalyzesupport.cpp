// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxanalyzesupport.h"

#include "qnxconstants.h"
#include "qnxtr.h"
#include "slog2inforunner.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qnx::Internal {

class QnxQmlProfilerWorkerFactory final : public RunWorkerFactory
{
public:
    QnxQmlProfilerWorkerFactory()
    {
        setProducer([](RunControl *runControl) {
            auto worker = new SimpleTargetRunner(runControl);
            worker->setId("QnxQmlProfilerSupport");
            worker->appendMessage(Tr::tr("Preparing remote side..."), LogMessageFormat);

            runControl->requestQmlChannel();

            auto slog2InfoRunner = new Slog2InfoRunner(runControl);
            worker->addStartDependency(slog2InfoRunner);

            auto profiler = runControl->createWorker(ProjectExplorer::Constants::QML_PROFILER_RUNNER);
            profiler->addStartDependency(worker);
            worker->addStopDependency(profiler);

            worker->setStartModifier([worker] {
                CommandLine cmd = worker->commandLine();
                cmd.addArg(qmlDebugTcpArguments(QmlProfilerServices, worker->qmlChannel()));
                worker->setCommandLine(cmd);
            });
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
