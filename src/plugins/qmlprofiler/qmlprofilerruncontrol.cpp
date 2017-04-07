/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmlprofilerruncontrol.h"
#include "localqmlprofilerrunner.h"
#include "qmlprofilertool.h"

#include <debugger/analyzer/analyzermanager.h>
#include <debugger/analyzer/analyzerstartparameters.h>

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/localapplicationruncontrol.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtsupportconstants.h>
#include <qmldebug/qmloutputparser.h>

#include <utils/qtcassert.h>

#include <QMainWindow>
#include <QMessageBox>
#include <QTimer>
#include <QTcpServer>
#include <QApplication>
#include <QMessageBox>
#include <QPushButton>

using namespace Debugger;
using namespace Core;
using namespace ProjectExplorer;

namespace QmlProfiler {

//
// QmlProfilerRunControlPrivate
//

class QmlProfilerRunControl::QmlProfilerRunControlPrivate
{
public:
    Internal::QmlProfilerTool *m_tool = 0;
    QmlProfilerStateManager *m_profilerState = 0;
    QTimer m_noDebugOutputTimer;
};

//
// QmlProfilerRunControl
//

QmlProfilerRunControl::QmlProfilerRunControl(RunConfiguration *runConfiguration,
                                             Internal::QmlProfilerTool *tool)
    : RunControl(runConfiguration, ProjectExplorer::Constants::QML_PROFILER_RUN_MODE)
    , d(new QmlProfilerRunControlPrivate)
{
    setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR);
    setSupportsReRunning(false);

    d->m_tool = tool;
    // Only wait 4 seconds for the 'Waiting for connection' on application output, then just try to connect
    // (application output might be redirected / blocked)
    d->m_noDebugOutputTimer.setSingleShot(true);
    d->m_noDebugOutputTimer.setInterval(4000);
    connect(&d->m_noDebugOutputTimer, &QTimer::timeout, this, [this]() {
        notifyRemoteSetupDone(Utils::Port());
    });
}

QmlProfilerRunControl::~QmlProfilerRunControl()
{
    if (isRunning() && d->m_profilerState)
        stop();
    delete d;
}

void QmlProfilerRunControl::start()
{
    reportApplicationStart();
    d->m_tool->finalizeRunControl(this);
    QTC_ASSERT(d->m_profilerState, reportApplicationStop(); return);

    QTC_ASSERT(connection().is<AnalyzerConnection>(), reportApplicationStop(); return);
    auto conn = connection().as<AnalyzerConnection>();

    if (conn.analyzerPort.isValid())
        emit processRunning(conn.analyzerPort);
    else if (conn.analyzerSocket.isEmpty())
        d->m_noDebugOutputTimer.start();

    d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppRunning);
    emit starting();
}

void QmlProfilerRunControl::stop()
{
    QTC_ASSERT(d->m_profilerState, return);

    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppRunning:
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppStopRequested);
        break;
    case QmlProfilerStateManager::AppStopRequested:
        // Pressed "stop" a second time. Kill the application without collecting data
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::Idle);
        break;
    case QmlProfilerStateManager::Idle:
    case QmlProfilerStateManager::AppDying:
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
    case QmlProfilerStateManager::AppRunning:
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppDying);
        reportApplicationStop();
        break;
    case QmlProfilerStateManager::Idle:
        break;
    default:
        const QString message = QString::fromLatin1("Process died unexpectedly from state %1 in %2:%3")
            .arg(d->m_profilerState->currentStateAsString(), QString::fromLatin1(__FILE__), QString::number(__LINE__));
        qWarning("%s", qPrintable(message));
        break;
    }
}

void QmlProfilerRunControl::cancelProcess()
{
    QTC_ASSERT(d->m_profilerState, return);

    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::Idle:
        break;
    case QmlProfilerStateManager::AppRunning:
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppDying);
        break;
    default: {
        const QString message = QString::fromLatin1("Unexpected process termination requested with state %1 in %2:%3")
            .arg(d->m_profilerState->currentStateAsString(), QString::fromLatin1(__FILE__), QString::number(__LINE__));
        qWarning("%s", qPrintable(message));
        return;
    }
    }
    reportApplicationStop();
}

void QmlProfilerRunControl::notifyRemoteSetupFailed(const QString &errorMessage)
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

    connect(infoBox, &QDialog::finished,
            this, &QmlProfilerRunControl::wrongSetupMessageBoxFinished);

    infoBox->show();

    // KILL
    d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppDying);
    d->m_noDebugOutputTimer.stop();
    reportApplicationStop();
}

void QmlProfilerRunControl::wrongSetupMessageBoxFinished(int button)
{
    if (button == QMessageBox::Help) {
        HelpManager::handleHelpRequest(QLatin1String("qthelp://org.qt-project.qtcreator/doc/creator-debugging-qml.html"
                               "#setting-up-qml-debugging"));
    }
}

void QmlProfilerRunControl::notifyRemoteSetupDone(Utils::Port port)
{
    d->m_noDebugOutputTimer.stop();

    if (!port.isValid()) {
        QTC_ASSERT(connection().is<AnalyzerConnection>(), return);
        port = connection().as<AnalyzerConnection>().analyzerPort;
    }
    if (port.isValid())
        emit processRunning(port);
}

void QmlProfilerRunControl::registerProfilerStateManager( QmlProfilerStateManager *profilerState )
{
    // disconnect old
    if (d->m_profilerState)
        disconnect(d->m_profilerState, &QmlProfilerStateManager::stateChanged,
                   this, &QmlProfilerRunControl::profilerStateChanged);

    d->m_profilerState = profilerState;

    // connect
    if (d->m_profilerState)
        connect(d->m_profilerState, &QmlProfilerStateManager::stateChanged,
                this, &QmlProfilerRunControl::profilerStateChanged);
}

void QmlProfilerRunControl::profilerStateChanged()
{
    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::Idle:
        reportApplicationStop();
        d->m_noDebugOutputTimer.stop();
        break;
    default:
        break;
    }
}

} // namespace QmlProfiler
