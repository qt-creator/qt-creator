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

#include "localqmlprofilerrunner.h"
#include "qmlprofilerplugin.h"

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qmldebug/qmldebugcommandlinearguments.h>
#include <debugger/analyzer/analyzerruncontrol.h>

#include <utils/temporaryfile.h>

#include <QTcpServer>
#include <QTemporaryFile>

using namespace ProjectExplorer;

namespace QmlProfiler {

QString LocalQmlProfilerRunner::findFreeSocket()
{
    Utils::TemporaryFile file("qmlprofiler-freesocket");
    if (file.open()) {
        return file.fileName();
    } else {
        qWarning() << "Could not open a temporary file to find a debug socket.";
        return QString();
    }
}

Utils::Port LocalQmlProfilerRunner::findFreePort(QString &host)
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost)
            && !server.listen(QHostAddress::LocalHostIPv6)) {
        qWarning() << "Cannot open port on host for QML profiling.";
        return Utils::Port();
    }
    host = server.serverAddress().toString();
    return Utils::Port(server.serverPort());
}

LocalQmlProfilerRunner::LocalQmlProfilerRunner(const Configuration &configuration,
                                               Debugger::AnalyzerRunControl *runControl) :
    QObject(runControl),
    m_configuration(configuration)
{
    connect(&m_launcher, &ApplicationLauncher::appendMessage,
            this, &LocalQmlProfilerRunner::appendMessage);
    connect(this, &LocalQmlProfilerRunner::stopped,
            runControl, &Debugger::AnalyzerRunControl::notifyRemoteFinished);
    connect(this, &LocalQmlProfilerRunner::appendMessage,
            runControl, &Debugger::AnalyzerRunControl::appendMessage);
    connect(runControl, &Debugger::AnalyzerRunControl::starting,
            this, &LocalQmlProfilerRunner::start);
    connect(runControl, &RunControl::finished,
            this, &LocalQmlProfilerRunner::stop);

    m_outputParser.setNoOutputText(ApplicationLauncher::msgWinCannotRetrieveDebuggingOutput());

    connect(runControl, &Debugger::AnalyzerRunControl::appendMessageRequested,
            this, [this](RunControl *runControl, const QString &msg, Utils::OutputFormat format) {
        Q_UNUSED(runControl);
        Q_UNUSED(format);
        m_outputParser.processOutput(msg);
    });

    connect(&m_outputParser, &QmlDebug::QmlOutputParser::waitingForConnectionOnPort,
            runControl, [this, runControl](Utils::Port port) {
        runControl->notifyRemoteSetupDone(port);
    });

    connect(&m_outputParser, &QmlDebug::QmlOutputParser::noOutputMessage,
            runControl, [this, runControl]() {
        runControl->notifyRemoteSetupDone(Utils::Port());
    });

    connect(&m_outputParser, &QmlDebug::QmlOutputParser::connectingToSocketMessage,
            runControl, [this, runControl]() {
        runControl->notifyRemoteSetupDone(Utils::Port());
    });

    connect(&m_outputParser, &QmlDebug::QmlOutputParser::errorMessage,
            runControl, [this, runControl](const QString &message) {
        runControl->notifyRemoteSetupFailed(message);
    });
}

void LocalQmlProfilerRunner::start()
{
    QTC_ASSERT(!m_configuration.socket.isEmpty() || m_configuration.port.isValid(), return);

    StandardRunnable runnable = m_configuration.debuggee;
    QString arguments = m_configuration.socket.isEmpty() ?
                QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlProfilerServices,
                                               m_configuration.port) :
                QmlDebug::qmlDebugLocalArguments(QmlDebug::QmlProfilerServices,
                                                 m_configuration.socket);


    if (!m_configuration.debuggee.commandLineArguments.isEmpty())
        arguments += QLatin1Char(' ') + m_configuration.debuggee.commandLineArguments;

    runnable.commandLineArguments = arguments;
    runnable.runMode = ApplicationLauncher::Gui;

    // queue this, as the process can already die in the call to start().
    // We want the started() signal to be emitted before the stopped() signal.
    connect(&m_launcher, &ApplicationLauncher::processExited,
            this, &LocalQmlProfilerRunner::spontaneousStop,
            Qt::QueuedConnection);
    m_launcher.start(runnable);

    emit started();
}

void LocalQmlProfilerRunner::spontaneousStop(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(status);
    disconnect(&m_launcher, &ApplicationLauncher::processExited,
               this, &LocalQmlProfilerRunner::spontaneousStop);

    emit stopped();
}

void LocalQmlProfilerRunner::stop()
{
    if (m_launcher.isRunning())
        m_launcher.stop();
}

} // namespace QmlProfiler
