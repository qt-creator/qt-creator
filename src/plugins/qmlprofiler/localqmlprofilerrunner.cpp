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
#include "qmlprofilerruncontrol.h"

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qmldebug/qmldebugcommandlinearguments.h>

#include <QTcpServer>
#include <QTemporaryFile>

using namespace QmlProfiler;
using namespace ProjectExplorer;

QString LocalQmlProfilerRunner::findFreeSocket()
{
    QTemporaryFile file;
    if (file.open()) {
        return file.fileName();
    } else {
        qWarning() << "Could not open a temporary file to find a debug socket.";
        return QString();
    }
}

quint16 LocalQmlProfilerRunner::findFreePort(QString &host)
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost)
            && !server.listen(QHostAddress::LocalHostIPv6)) {
        qWarning() << "Cannot open port on host for QML profiling.";
        return 0;
    }
    host = server.serverAddress().toString();
    return server.serverPort();
}

LocalQmlProfilerRunner::LocalQmlProfilerRunner(const Configuration &configuration,
                                               QmlProfilerRunControl *engine) :
    QObject(engine),
    m_configuration(configuration)
{
    connect(&m_launcher, &ApplicationLauncher::appendMessage,
            this, &LocalQmlProfilerRunner::appendMessage);
    connect(this, &LocalQmlProfilerRunner::stopped,
            engine, &QmlProfilerRunControl::notifyRemoteFinished);
    connect(this, &LocalQmlProfilerRunner::appendMessage,
            engine, &QmlProfilerRunControl::logApplicationMessage);
    connect(engine, &Debugger::AnalyzerRunControl::starting,
            this, &LocalQmlProfilerRunner::start);
    connect(engine, &RunControl::finished,
            this, &LocalQmlProfilerRunner::stop);
}

LocalQmlProfilerRunner::~LocalQmlProfilerRunner()
{
    disconnect();
}

void LocalQmlProfilerRunner::start()
{
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

    if (QmlProfilerPlugin::debugOutput) {
        qWarning("QmlProfiler: Launching %s:%s", qPrintable(m_configuration.debuggee.executable),
                 qPrintable(m_configuration.socket.isEmpty() ?
                                QString::number(m_configuration.port) : m_configuration.socket));
    }

    connect(&m_launcher, &ApplicationLauncher::processExited,
            this, &LocalQmlProfilerRunner::spontaneousStop);
    m_launcher.start(runnable);

    emit started();
}

void LocalQmlProfilerRunner::spontaneousStop(int exitCode, QProcess::ExitStatus status)
{
    if (QmlProfilerPlugin::debugOutput) {
        if (status == QProcess::CrashExit)
            qWarning("QmlProfiler: Application crashed.");
        else
            qWarning("QmlProfiler: Application exited (exit code %d).", exitCode);
    }

    disconnect(&m_launcher, &ApplicationLauncher::processExited,
               this, &LocalQmlProfilerRunner::spontaneousStop);

    emit stopped();
}

void LocalQmlProfilerRunner::stop()
{
    if (QmlProfilerPlugin::debugOutput)
        qWarning("QmlProfiler: Stopping application ...");

    if (m_launcher.isRunning())
        m_launcher.stop();
}
