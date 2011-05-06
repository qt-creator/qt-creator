/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmlprofilerengine.h"

#include "qmlprofilerplugin.h"
#include "qmlprofilertool.h"
#include "localqmlprofilerrunner.h"

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerconstants.h>
#include <projectexplorer/applicationlauncher.h>

#include <qmljsdebugclient/qdeclarativedebugclient_p.h>

#include <utils/qtcassert.h>

#include "timelineview.h"
#include "tracewindow.h"

#include <QDebug>

#include "canvas/qdeclarativecanvas_p.h"
#include "canvas/qdeclarativecontext2d_p.h"
#include "canvas/qdeclarativetiledcanvas_p.h"



using namespace QmlProfiler::Internal;


//
// QmlProfilerEnginePrivate
//

class QmlProfilerEngine::QmlProfilerEnginePrivate
{
public:
    QmlProfilerEnginePrivate(QmlProfilerEngine *qq) : q(qq), m_runner(0) {}
    ~QmlProfilerEnginePrivate() {}

    bool attach(const QString &address, uint port);
    static AbstractQmlProfilerRunner *createRunner(const Analyzer::AnalyzerStartParameters &m_params, QObject *parent);

    QmlProfilerEngine *q;

    Analyzer::AnalyzerStartParameters m_params;
    AbstractQmlProfilerRunner *m_runner;
    bool m_running;
    bool m_fetchingData;
    bool m_delayedDelete;
};

AbstractQmlProfilerRunner *
QmlProfilerEngine::QmlProfilerEnginePrivate::createRunner(const Analyzer::AnalyzerStartParameters &m_params, QObject *parent)
{
    AbstractQmlProfilerRunner *runner = 0;
    if (m_params.startMode == Analyzer::StartLocal) {
        LocalQmlProfilerRunner::Configuration configuration;
        configuration.executable = m_params.debuggee;
        configuration.executableArguments = m_params.debuggeeArgs;
        configuration.workingDirectory = m_params.workingDirectory;
        configuration.environment = m_params.environment;
        configuration.port = m_params.connParams.port;

        runner = new LocalQmlProfilerRunner(configuration, parent);

    } else if (m_params.startMode == Analyzer::StartRemote) {

    }
    return runner;
}

//
// QmlProfilerEngine
//

QmlProfilerEngine::QmlProfilerEngine(const Analyzer::AnalyzerStartParameters &sp, ProjectExplorer::RunConfiguration *runConfiguration)
    : IAnalyzerEngine(sp, runConfiguration)
    , d(new QmlProfilerEnginePrivate(this))
{
    d->m_params = sp;

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
    QTC_ASSERT(!d->m_runner, return);
    d->m_runner = QmlProfilerEnginePrivate::createRunner(d->m_params, this);
    QTC_ASSERT(d->m_runner, return);

    connect(d->m_runner, SIGNAL(started()), this, SIGNAL(processRunning()));
    connect(d->m_runner, SIGNAL(stopped()), this, SLOT(stopped()));
    connect(d->m_runner, SIGNAL(appendMessage(QString,Utils::OutputFormat)),
            this, SLOT(logApplicationMessage(QString,Utils::OutputFormat)));

    d->m_runner->start();

    d->m_running = true;
    d->m_delayedDelete = false;
}

void QmlProfilerEngine::stop()
{
    if (d->m_fetchingData) {
        if (d->m_running)
            d->m_delayedDelete = true;
        emit stopRecording();
    } else {
        finishProcess();
    }
}

void QmlProfilerEngine::stopped()
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
    // user stop?
    if (d->m_running) {
        d->m_running = false;
        d->m_runner->stop();

        emit finished();
    }
}

void QmlProfilerEngine::logApplicationMessage(const QString &msg, Utils::OutputFormat /*format*/)
{
    qDebug() << "app: " << msg;
}
