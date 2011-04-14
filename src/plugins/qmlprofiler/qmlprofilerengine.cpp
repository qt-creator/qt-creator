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

#include <qmljsdebugclient/qdeclarativedebugclient_p.h>

#include "timelineview.h"
#include "tracewindow.h"

#include <QDebug>

#include "canvas/qdeclarativecanvas_p.h"
#include "canvas/qdeclarativecontext2d_p.h"
#include "canvas/qdeclarativetiledcanvas_p.h"

#include <QProcess>


using namespace QmlProfiler::Internal;

class QmlProfilerEngine::QmlProfilerEnginePrivate
{
public:
    QmlProfilerEnginePrivate(QmlProfilerEngine *qq) : q(qq) {}
    ~QmlProfilerEnginePrivate() {}

    bool launchperfmonitor();
    bool attach(const QString &address, uint port);

    QmlProfilerEngine *q;

    Analyzer::AnalyzerStartParameters m_params;
    QProcess *m_process;
    bool m_running;
    bool m_fetchingData;
    bool m_delayedDelete;
};

QmlProfilerEngine::QmlProfilerEngine(const Analyzer::AnalyzerStartParameters &sp, ProjectExplorer::RunConfiguration *runConfiguration)
    : IAnalyzerEngine(sp, runConfiguration)
    , d(new QmlProfilerEnginePrivate(this))
{
    d->m_params = sp;
    d->m_process = 0;
    d->m_running = false;
    d->m_fetchingData = false;
    d->m_delayedDelete = false;
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
    d->m_delayedDelete = false;

    emit processRunning();
}

void QmlProfilerEngine::stop()
{
    if (d->m_fetchingData) {
        if (d->m_running)
            d->m_delayedDelete = true;
        emit stopRecording();
    }
    else
        finishProcess();
}

void QmlProfilerEngine::spontaneousStop()
{
    d->m_running = false;
    Analyzer::AnalyzerManager::instance()->stopTool();
    emit finished();
}

void QmlProfilerEngine::setFetchingData(bool b)
{
    d->m_fetchingData = b;
}

void QmlProfilerEngine::dataReceived() {
    if (d->m_delayedDelete)
        finishProcess();
    d->m_delayedDelete = false;
}

void QmlProfilerEngine::finishProcess()
{
    if (d->m_running) {
        d->m_running = false;
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

        emit finished();
    }
}

bool QmlProfilerEngine::QmlProfilerEnginePrivate::launchperfmonitor()
{
    bool qtquick1 = false;

    m_process = new QProcess();

    QStringList arguments("-qmljsdebugger=port:" + QString::number(m_params.connParams.port) + ",block");
    arguments.append(m_params.debuggeeArgs.split(" "));

    if (QmlProfilerPlugin::debugOutput)
        qWarning("QmlProfiler: Launching %s", qPrintable(m_params.displayName));

    if (qtquick1) {
        QProcessEnvironment env;
        env.insert("QMLSCENE_IMPORT_NAME", "quick1");
        m_process->setProcessEnvironment(env);
    }

    m_process->setProcessChannelMode(QProcess::ForwardedChannels);
    m_process->setWorkingDirectory(m_params.workingDirectory);
    connect(m_process,SIGNAL(finished(int)),q,SLOT(spontaneousStop()));
    m_process->start(m_params.debuggee, arguments);

    if (!m_process->waitForStarted()) {
        if (QmlProfilerPlugin::debugOutput)
            qWarning("QmlProfiler: %s failed to start", qPrintable(m_params.displayName));
        return false;
    }

    if (QmlProfilerPlugin::debugOutput)
        qWarning("QmlProfiler: Connecting to %s:%d", qPrintable(m_params.connParams.host), m_params.connParams.port);

    return true;
}


















