/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlprofilerruncontrol.h"

#include "localqmlprofilerrunner.h"

#include <analyzerbase/analyzermanager.h>
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <coreplugin/helpmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/localapplicationruncontrol.h>
#include <projectexplorer/localapplicationrunconfiguration.h>
#include <qtsupport/qtsupportconstants.h>
#include <qmldebug/qmloutputparser.h>

#include <QMainWindow>
#include <QMessageBox>
#include <QTimer>
#include <QTcpServer>
#include <QApplication>
#include <QMessageBox>
#include <QPushButton>

using namespace Analyzer;
using namespace Core;
using namespace ProjectExplorer;

namespace QmlProfiler {

//
// QmlProfilerRunControlPrivate
//

class QmlProfilerRunControl::QmlProfilerRunControlPrivate
{
public:
    QmlProfilerRunControlPrivate() : m_running(false) {}

    QmlProfilerStateManager *m_profilerState;
    QTimer m_noDebugOutputTimer;
    QmlDebug::QmlOutputParser m_outputParser;
    bool m_running;
};

//
// QmlProfilerRunControl
//

QmlProfilerRunControl::QmlProfilerRunControl(const AnalyzerStartParameters &sp,
                                             RunConfiguration *runConfiguration) :
    AnalyzerRunControl(sp, runConfiguration), d(new QmlProfilerRunControlPrivate)
{
    d->m_profilerState = 0;

    // Only wait 4 seconds for the 'Waiting for connection' on application output, then just try to connect
    // (application output might be redirected / blocked)
    d->m_noDebugOutputTimer.setSingleShot(true);
    d->m_noDebugOutputTimer.setInterval(4000);
    connect(&d->m_noDebugOutputTimer, SIGNAL(timeout()), this, SLOT(processIsRunning()));

    d->m_outputParser.setNoOutputText(ApplicationLauncher::msgWinCannotRetrieveDebuggingOutput());
    connect(&d->m_outputParser, SIGNAL(waitingForConnectionOnPort(quint16)),
            this, SLOT(processIsRunning(quint16)));
    connect(&d->m_outputParser, SIGNAL(noOutputMessage()),
            this, SLOT(processIsRunning()));
    connect(&d->m_outputParser, SIGNAL(errorMessage(QString)),
            this, SLOT(wrongSetupMessageBox(QString)));
}

QmlProfilerRunControl::~QmlProfilerRunControl()
{
    if (d->m_profilerState && d->m_profilerState->currentState() == QmlProfilerStateManager::AppRunning)
        stopEngine();
    delete d;
}

bool QmlProfilerRunControl::startEngine()
{
    QTC_ASSERT(d->m_profilerState, return false);

    d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppStarting);

    if (startParameters().analyzerPort != 0)
        emit processRunning(startParameters().analyzerPort);
    else
        d->m_noDebugOutputTimer.start();

    d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppRunning);
    engineStarted();
    return true;
}

void QmlProfilerRunControl::stopEngine()
{
    QTC_ASSERT(d->m_profilerState, return);

    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppRunning : {
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppStopRequested);
        break;
    }
    case QmlProfilerStateManager::AppReadyToStop : {
        cancelProcess();
        break;
    }
    case QmlProfilerStateManager::AppDying :
        // valid, but no further action is needed
        break;
    default: {
        const QString message = QString::fromLatin1("Unexpected engine stop from state %1 in %2:%3")
            .arg(d->m_profilerState->currentStateAsString(), QString::fromLatin1(__FILE__), QString::number(__LINE__));
        qWarning("%s", qPrintable(message));
    }
        break;
    }
}

void QmlProfilerRunControl::notifyRemoteFinished()
{
    QTC_ASSERT(d->m_profilerState, return);

    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppRunning : {
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppDying);
        AnalyzerManager::stopTool();

        runControlFinished();
        break;
    }
    case QmlProfilerStateManager::AppStopped :
    case QmlProfilerStateManager::AppKilled :
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::Idle);
        break;
    default: {
        const QString message = QString::fromLatin1("Process died unexpectedly from state %1 in %2:%3")
            .arg(d->m_profilerState->currentStateAsString(), QString::fromLatin1(__FILE__), QString::number(__LINE__));
        qWarning("%s", qPrintable(message));
}
        break;
    }
}

void QmlProfilerRunControl::cancelProcess()
{
    QTC_ASSERT(d->m_profilerState, return);

    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppReadyToStop : {
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppStopped);
        break;
    }
    case QmlProfilerStateManager::AppRunning : {
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppDying);
        break;
    }
    default: {
        const QString message = QString::fromLatin1("Unexpected process termination requested with state %1 in %2:%3")
            .arg(d->m_profilerState->currentStateAsString(), QString::fromLatin1(__FILE__), QString::number(__LINE__));
        qWarning("%s", qPrintable(message));
        return;
    }
    }
    runControlFinished();
}

void QmlProfilerRunControl::logApplicationMessage(const QString &msg, Utils::OutputFormat format)
{
    appendMessage(msg, format);
    d->m_outputParser.processOutput(msg);
}

void QmlProfilerRunControl::wrongSetupMessageBox(const QString &errorMessage)
{
    QMessageBox *infoBox = new QMessageBox(ICore::mainWindow());
    infoBox->setIcon(QMessageBox::Critical);
    infoBox->setWindowTitle(tr("Qt Creator"));
    //: %1 is detailed error message
    infoBox->setText(tr("Could not connect to the in-process QML debugger:\n%1")
                     .arg(errorMessage));
    infoBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Help);
    infoBox->setDefaultButton(QMessageBox::Ok);
    infoBox->setModal(true);

    connect(infoBox, SIGNAL(finished(int)),
            this, SLOT(wrongSetupMessageBoxFinished(int)));

    infoBox->show();

    // KILL
    d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppDying);
    AnalyzerManager::stopTool();
    runControlFinished();
}

void QmlProfilerRunControl::wrongSetupMessageBoxFinished(int button)
{
    if (button == QMessageBox::Help) {
        HelpManager::handleHelpRequest(QLatin1String("qthelp://org.qt-project.qtcreator/doc/creator-debugging-qml.html"
                               "#setting-up-qml-debugging"));
    }
}

void QmlProfilerRunControl::showNonmodalWarning(const QString &warningMsg)
{
    QMessageBox *noExecWarning = new QMessageBox(ICore::mainWindow());
    noExecWarning->setIcon(QMessageBox::Warning);
    noExecWarning->setWindowTitle(tr("QML Profiler"));
    noExecWarning->setText(warningMsg);
    noExecWarning->setStandardButtons(QMessageBox::Ok);
    noExecWarning->setDefaultButton(QMessageBox::Ok);
    noExecWarning->setModal(false);
    noExecWarning->show();
}

void QmlProfilerRunControl::notifyRemoteSetupDone(quint16 port)
{
    d->m_noDebugOutputTimer.stop();
    emit processRunning(port);
}

void QmlProfilerRunControl::processIsRunning(quint16 port)
{
    d->m_noDebugOutputTimer.stop();

    if (port == 0)
        port = startParameters().analyzerPort;
    if (port != 0)
        emit processRunning(port);
}

void QmlProfilerRunControl::engineStarted()
{
    d->m_running = true;
    emit starting(this);
}

void QmlProfilerRunControl::runControlFinished()
{
    d->m_running = false;
    emit finished();
}

void QmlProfilerRunControl::registerProfilerStateManager( QmlProfilerStateManager *profilerState )
{
    // disconnect old
    if (d->m_profilerState)
        disconnect(d->m_profilerState, SIGNAL(stateChanged()), this, SLOT(profilerStateChanged()));

    d->m_profilerState = profilerState;

    // connect
    if (d->m_profilerState)
        connect(d->m_profilerState, SIGNAL(stateChanged()), this, SLOT(profilerStateChanged()));
}

void QmlProfilerRunControl::profilerStateChanged()
{
    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppReadyToStop : {
        if (d->m_running)
            cancelProcess();
        break;
    }
    case QmlProfilerStateManager::Idle : {
        d->m_noDebugOutputTimer.stop();
        break;
    }
    default:
        break;
    }
}

} // namespace QmlProfiler
