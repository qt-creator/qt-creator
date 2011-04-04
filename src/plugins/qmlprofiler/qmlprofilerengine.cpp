/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmlprofilerengine.h"

#include "qmlprofilerplugin.h"
#include "qmlprofilertool.h"

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerconstants.h>

#include <projectexplorer/applicationrunconfiguration.h>

#include <private/qdeclarativedebugclient_p.h>

#include "timelineview.h"
#include "tracewindow.h"

#include <QDebug>

#include "canvas/qdeclarativecanvas_p.h"
#include "canvas/qdeclarativecontext2d_p.h"
#include "canvas/qdeclarativetiledcanvas_p.h"

#include <utils/environment.h>
#include <QProcess>
#include "tracewindow.h"

#ifdef Q_OS_UNIX
#include <unistd.h> // sleep
#endif

using namespace QmlProfiler::Internal;

class QmlProfilerEngine::QmlProfilerEnginePrivate
{
public:
    QmlProfilerEnginePrivate(QmlProfilerEngine *qq) : q(qq) {}
    ~QmlProfilerEnginePrivate() {}

    bool launchperfmonitor();
    bool attach(const QString &address, uint port);

    QmlProfilerEngine *q;

    QString m_workingDirectory;
    QString m_executable;
    QString m_commandLineArguments;
    Utils::Environment m_environment;

    QProcess *m_process;
    bool m_running;
};

QmlProfilerEngine::QmlProfilerEngine(const Analyzer::AnalyzerStartParameters &sp, ProjectExplorer::RunConfiguration *runConfiguration)
    : IAnalyzerEngine(sp, runConfiguration)
    , d(new QmlProfilerEnginePrivate(this))
{
    ProjectExplorer::LocalApplicationRunConfiguration *localAppConfig =
            qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration *>(runConfiguration);

    if (!localAppConfig)
        return;

    d->m_workingDirectory = localAppConfig->workingDirectory();
    d->m_executable = localAppConfig->executable();
    d->m_commandLineArguments = localAppConfig->commandLineArguments();
    d->m_environment = localAppConfig->environment();
    d->m_process = 0;
    d->m_running = false;
}

QmlProfilerEngine::~QmlProfilerEngine()
{
    if (d->m_running)
        stop();
    delete d;
}

void QmlProfilerEngine::start()
{
    d->launchperfmonitor();
    d->m_running = true;

    emit processRunning();
}

void QmlProfilerEngine::stop()
{
    d->m_running = false;
    emit stopRecording();
}

void QmlProfilerEngine::spontaneousStop()
{
    Analyzer::AnalyzerManager::instance()->stopTool();
}

void QmlProfilerEngine::viewUpdated()
{
    if (d->m_process) {
        disconnect(d->m_process,SIGNAL(finished(int)),this,SLOT(spontaneousStop()));
        if (d->m_process->state() == QProcess::Running) {
            d->m_process->terminate();
            if (!d->m_process->waitForFinished(1000)) {
                d->m_process->kill();
                d->m_process->waitForFinished();
            }
        }
        delete d->m_process;
        d->m_process = 0;
    }

    emit processTerminated();
}

bool QmlProfilerEngine::QmlProfilerEnginePrivate::launchperfmonitor()
{
    bool qtquick1 = false;

    m_process = new QProcess();

    QStringList arguments("-qmljsdebugger=port:" + QString::number(QmlProfilerTool::port) + ",block");
    if (QmlProfilerPlugin::debugOutput)
        qWarning("QmlProfiler: Launching %s", qPrintable(m_executable));

    if (qtquick1) {
        QProcessEnvironment env;
        env.insert("QMLSCENE_IMPORT_NAME", "quick1");
        m_process->setProcessEnvironment(env);
    }

    m_process->setProcessChannelMode(QProcess::ForwardedChannels);
    m_process->setWorkingDirectory(m_workingDirectory);
    connect(m_process,SIGNAL(finished(int)),q,SLOT(spontaneousStop()));
    m_process->start(m_executable, arguments);

    // give the process time to start
    sleep(1);

    if (!m_process->waitForStarted()) {
        if (QmlProfilerPlugin::debugOutput)
            qWarning("QmlProfiler: %s failed to start", qPrintable(m_executable));
        return false;
    }

    if (QmlProfilerPlugin::debugOutput)
        qWarning("QmlProfiler: Connecting to %s:%d", qPrintable(QmlProfilerTool::host), QmlProfilerTool::port);

    return true;
}


















