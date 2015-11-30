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

#include "localqmlprofilerrunner.h"
#include "qmlprofilerplugin.h"
#include "qmlprofilerruncontrol.h"

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <analyzerbase/analyzerstartparameters.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/localapplicationrunconfiguration.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qmldebug/qmldebugcommandlinearguments.h>

#include <QTcpServer>
#include <QTemporaryFile>

using namespace QmlProfiler;
using namespace ProjectExplorer;

Analyzer::AnalyzerRunControl *LocalQmlProfilerRunner::createLocalRunControl(
        RunConfiguration *runConfiguration,
        const Analyzer::AnalyzerStartParameters &sp,
        QString *errorMessage)
{
    // only desktop device is supported
    const IDevice::ConstPtr device = DeviceKitInformation::device(
                runConfiguration->target()->kit());
    QTC_ASSERT(device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE, return 0);

    Analyzer::AnalyzerRunControl *rc = Analyzer::AnalyzerManager::createRunControl(
                sp, runConfiguration);
    QmlProfilerRunControl *engine = qobject_cast<QmlProfilerRunControl *>(rc);
    if (!engine) {
        delete rc;
        return 0;
    }

    Configuration conf;
    conf.executable = sp.debuggee;
    conf.executableArguments = sp.debuggeeArgs;
    conf.workingDirectory = sp.workingDirectory;
    conf.environment = sp.environment;
    conf.socket = sp.analyzerSocket;
    conf.port = sp.analyzerPort;

    if (conf.executable.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("No executable file to launch.");
        return 0;
    }

    LocalQmlProfilerRunner *runner = new LocalQmlProfilerRunner(conf, engine);

    QObject::connect(runner, &LocalQmlProfilerRunner::stopped,
                     engine, &QmlProfilerRunControl::notifyRemoteFinished);
    QObject::connect(runner, &LocalQmlProfilerRunner::appendMessage,
                     engine, &QmlProfilerRunControl::logApplicationMessage);
    QObject::connect(engine, &Analyzer::AnalyzerRunControl::starting,
                     runner, &LocalQmlProfilerRunner::start);
    QObject::connect(rc, &RunControl::finished, runner, &LocalQmlProfilerRunner::stop);
    return rc;
}

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
    m_configuration(configuration),
    m_engine(engine)
{
    connect(&m_launcher, &ApplicationLauncher::appendMessage,
            this, &LocalQmlProfilerRunner::appendMessage);
}

LocalQmlProfilerRunner::~LocalQmlProfilerRunner()
{
    disconnect();
}

void LocalQmlProfilerRunner::start()
{
    QString arguments = m_configuration.socket.isEmpty() ?
                QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlProfilerServices,
                                               m_configuration.port) :
                QmlDebug::qmlDebugLocalArguments(QmlDebug::QmlProfilerServices,
                                                 m_configuration.socket);


    if (!m_configuration.executableArguments.isEmpty())
        arguments += QLatin1Char(' ') + m_configuration.executableArguments;

    if (QmlProfilerPlugin::debugOutput) {
        qWarning("QmlProfiler: Launching %s:%s", qPrintable(m_configuration.executable),
                 qPrintable(m_configuration.socket.isEmpty() ?
                                QString::number(m_configuration.port) : m_configuration.socket));
    }

    m_launcher.setWorkingDirectory(m_configuration.workingDirectory);
    m_launcher.setEnvironment(m_configuration.environment);
    connect(&m_launcher, &ApplicationLauncher::processExited,
            this, &LocalQmlProfilerRunner::spontaneousStop);
    m_launcher.start(ApplicationLauncher::Gui, m_configuration.executable, arguments);

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
