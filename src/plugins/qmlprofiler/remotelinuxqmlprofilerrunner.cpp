/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "remotelinuxqmlprofilerrunner.h"

#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <remotelinux/remotelinuxrunconfiguration.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace QmlProfiler::Internal;
using namespace RemoteLinux;

RemoteLinuxQmlProfilerRunner::RemoteLinuxQmlProfilerRunner(
    RemoteLinuxRunConfiguration *runConfiguration, QObject *parent)
    : AbstractQmlProfilerRunner(parent)
    , m_portsGatherer(new DeviceUsedPortsGatherer(this))
    , m_runner(new DeviceApplicationRunner(this))
    , m_device(DeviceKitInformation::device(runConfiguration->target()->kit()))
    , m_remoteExecutable(runConfiguration->remoteExecutableFilePath())
    , m_arguments(runConfiguration->arguments())
    , m_commandPrefix(runConfiguration->commandPrefix())
    , m_port(0)
{
    connect(m_runner, SIGNAL(reportError(QString)), this, SLOT(handleError(QString)));
    connect(m_runner, SIGNAL(remoteStderr(QByteArray)), this, SLOT(handleStdErr(QByteArray)));
    connect(m_runner, SIGNAL(remoteStdout(QByteArray)), this, SLOT(handleStdOut(QByteArray)));
    connect(m_runner, SIGNAL(finished(bool)), SLOT(handleRemoteProcessFinished(bool)));
    connect(m_runner, SIGNAL(reportProgress(QString)), this, SLOT(handleProgressReport(QString)));
    connect(m_portsGatherer, SIGNAL(error(QString)), SLOT(handlePortsGathererError(QString)));
    connect(m_portsGatherer, SIGNAL(portListReady()), SLOT(handlePortListReady()));
}

RemoteLinuxQmlProfilerRunner::~RemoteLinuxQmlProfilerRunner()
{
    stop();
}

void RemoteLinuxQmlProfilerRunner::start()
{
    QTC_ASSERT(m_port == 0, return);

    m_portsGatherer->start(m_device);
    emit started();
}

void RemoteLinuxQmlProfilerRunner::stop()
{
    if (m_port == 0)
        m_portsGatherer->stop();
    else
        m_runner->stop(m_device->processSupport()
                ->killProcessByNameCommandLine(m_remoteExecutable).toUtf8());
    m_port = 0;
}

quint16 RemoteLinuxQmlProfilerRunner::debugPort() const
{
    return m_port;
}

void RemoteLinuxQmlProfilerRunner::handlePortsGathererError(const QString &message)
{
    emit appendMessage(tr("Gathering ports failed: %1").arg(message), Utils::ErrorMessageFormat);
    m_port = 0;
    emit stopped();
}

void RemoteLinuxQmlProfilerRunner::handlePortListReady()
{
    getPorts();
}

void RemoteLinuxQmlProfilerRunner::getPorts()
{
    Utils::PortList portList = m_device->freePorts();
    m_port = m_portsGatherer->getNextFreePort(&portList);

    if (m_port == -1) {
        emit appendMessage(tr("Not enough free ports on device for analyzing.\n"),
                           Utils::ErrorMessageFormat);
        m_port = 0;
        emit stopped();
    } else {
        emit appendMessage(tr("Starting remote process...\n"), Utils::NormalMessageFormat);
        QString arguments = m_arguments;
        if (!arguments.isEmpty())
            arguments.append(QLatin1Char(' '));
        arguments.append(QString::fromLatin1("-qmljsdebugger=port:%1,block").arg(m_port));
        const QString commandLine = QString::fromLatin1("%1 %2 %3")
                .arg(m_commandPrefix, m_remoteExecutable, arguments);
        m_runner->start(m_device, commandLine.toUtf8());
    }
}

void RemoteLinuxQmlProfilerRunner::handleError(const QString &msg)
{
    emit appendMessage(msg + QLatin1Char('\n'), Utils::ErrorMessageFormat);
}

void RemoteLinuxQmlProfilerRunner::handleStdErr(const QByteArray &msg)
{
    emit appendMessage(QString::fromUtf8(msg), Utils::StdErrFormat);
}

void RemoteLinuxQmlProfilerRunner::handleStdOut(const QByteArray &msg)
{
    emit appendMessage(QString::fromUtf8(msg), Utils::StdOutFormat);
}

void RemoteLinuxQmlProfilerRunner::handleRemoteProcessFinished(bool success)
{
    if (!success)
        appendMessage(tr("Failure running remote process."), Utils::NormalMessageFormat);
    m_port = 0;
    emit stopped();
}

void RemoteLinuxQmlProfilerRunner::handleProgressReport(const QString &progressString)
{
    appendMessage(progressString + QLatin1Char('\n'), Utils::NormalMessageFormat);
}
