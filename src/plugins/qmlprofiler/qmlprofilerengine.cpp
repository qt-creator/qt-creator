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
#include <projectexplorer/applicationlauncher.h>

#include <qmljsdebugclient/qdeclarativedebugclient_p.h>

#include "timelineview.h"
#include "tracewindow.h"

#include <QDebug>

#include "canvas/qdeclarativecanvas_p.h"
#include "canvas/qdeclarativecontext2d_p.h"
#include "canvas/qdeclarativetiledcanvas_p.h"



using namespace QmlProfiler::Internal;

class QmlProfilerEngine::QmlProfilerEnginePrivate
{
public:
    QmlProfilerEnginePrivate(QmlProfilerEngine *qq) : q(qq) {}
    ~QmlProfilerEnginePrivate() {}

    void launchperfmonitor();
    bool attach(const QString &address, uint port);

    QmlProfilerEngine *q;

    Analyzer::AnalyzerStartParameters m_params;
    ProjectExplorer::ApplicationLauncher m_launcher;
    bool m_running;
    bool m_fetchingData;
    bool m_delayedDelete;
};

QmlProfilerEngine::QmlProfilerEngine(const Analyzer::AnalyzerStartParameters &sp, ProjectExplorer::RunConfiguration *runConfiguration)
    : IAnalyzerEngine(sp, runConfiguration)
    , d(new QmlProfilerEnginePrivate(this))
{
    d->m_params = sp;

    d->m_running = false;
    d->m_fetchingData = false;
    d->m_delayedDelete = false;

    connect(&d->m_launcher, SIGNAL(appendMessage(QString,ProjectExplorer::OutputFormat)),
            this, SLOT(logApplicationMessage(QString,ProjectExplorer::OutputFormat)));
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

void QmlProfilerEngine::spontaneousStop(int exitCode)
{
    if (QmlProfilerPlugin::debugOutput)
        qWarning() << "QmlProfiler: Application exited (exit code " << exitCode << ").";
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
        disconnect(&d->m_launcher, SIGNAL(processExited(int)), this, SLOT(spontaneousStop(int)));
        if (d->m_launcher.isRunning()) {
            d->m_launcher.stop();
        }

        emit finished();
    }
}

void QmlProfilerEngine::QmlProfilerEnginePrivate::launchperfmonitor()
{
    QString arguments = QLatin1String("-qmljsdebugger=port:") + QString::number(m_params.connParams.port)
            + QLatin1String(",block");
    if (!m_params.debuggeeArgs.isEmpty())
        arguments += QChar(' ') + m_params.debuggeeArgs;

    if (QmlProfilerPlugin::debugOutput)
        qWarning("QmlProfiler: Launching %s:%d", qPrintable(m_params.displayName), m_params.connParams.port);

    m_launcher.setWorkingDirectory(m_params.workingDirectory);
    m_launcher.setEnvironment(m_params.environment);
    connect(&m_launcher, SIGNAL(processExited(int)), q, SLOT(spontaneousStop(int)));
    m_launcher.start(ProjectExplorer::ApplicationLauncher::Gui, m_params.debuggee, arguments);
}

void QmlProfilerEngine::logApplicationMessage(const QString &msg, Utils::OutputFormat /*format*/)
{
    qDebug() << "app: " << msg;
}
