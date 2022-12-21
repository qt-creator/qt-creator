// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidqmltoolingsupport.h"
#include "androidrunner.h"

using namespace ProjectExplorer;

namespace Android {
namespace Internal {

AndroidQmlToolingSupport::AndroidQmlToolingSupport(RunControl *runControl,
                                                   const QString &intentName)
    : RunWorker(runControl)
{
    setId("AndroidQmlToolingSupport");

    auto runner = new AndroidRunner(runControl, intentName);
    addStartDependency(runner);

    auto worker = runControl->createWorker(QmlDebug::runnerIdForRunMode(runControl->runMode()));
    worker->addStartDependency(this);

    connect(runner, &AndroidRunner::qmlServerReady, this, [this, worker](const QUrl &server) {
        worker->recordData("QmlServerUrl", server);
        reportStarted();
    });
}

void AndroidQmlToolingSupport::start()
{
}

void AndroidQmlToolingSupport::stop()
{
    reportStopped();
}

} // namespace Internal
} // namespace Android
