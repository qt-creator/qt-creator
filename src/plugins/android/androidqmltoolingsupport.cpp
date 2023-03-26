// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidqmltoolingsupport.h"

#include "androidconstants.h"
#include "androidrunner.h"

using namespace ProjectExplorer;

namespace Android::Internal {

class AndroidQmlToolingSupport final : public RunWorker
{
public:
    explicit AndroidQmlToolingSupport(RunControl *runControl) : RunWorker(runControl)
    {
        setId("AndroidQmlToolingSupport");

        auto runner = new AndroidRunner(runControl, {});
        addStartDependency(runner);

        auto worker = runControl->createWorker(QmlDebug::runnerIdForRunMode(runControl->runMode()));
        worker->addStartDependency(this);

        connect(runner, &AndroidRunner::qmlServerReady, this, [this, worker](const QUrl &server) {
            worker->recordData("QmlServerUrl", server);
            reportStarted();
        });
    }

private:
    void start() override {}
    void stop() override { reportStopped(); }
};


AndroidQmlToolingSupportFactory::AndroidQmlToolingSupportFactory()
{
    setProduct<AndroidQmlToolingSupport>();
    addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    addSupportedRunConfig(Constants::ANDROID_RUNCONFIG_ID);
}

} // Android::Internal
