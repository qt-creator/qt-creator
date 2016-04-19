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

#include "startgdbserverdialog.h"

#include <debugger/debuggerengine.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerruncontrol.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <projectexplorer/kitchooser.h>
#include <projectexplorer/devicesupport/deviceprocesslist.h>
#include <projectexplorer/devicesupport/deviceprocessesdialog.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

using namespace Core;
using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;


namespace Debugger {
namespace Internal {

class StartGdbServerDialogPrivate
{
public:
    StartGdbServerDialogPrivate() : dialog(0), kit(0) {}

    DeviceProcessesDialog *dialog;
    bool attachToServer;
    DeviceProcessItem process;
    Kit *kit;
    IDevice::ConstPtr device;

    DeviceUsedPortsGatherer gatherer;
    SshRemoteProcessRunner runner;
};

GdbServerStarter::GdbServerStarter(DeviceProcessesDialog *dlg, bool attachAfterServerStart)
  : QObject(dlg)
{
    d = new StartGdbServerDialogPrivate;
    d->dialog = dlg;
    d->kit = dlg->kitChooser()->currentKit();
    d->process = dlg->currentProcess();
    d->device = DeviceKitInformation::device(d->kit);
    d->attachToServer = attachAfterServerStart;
}

GdbServerStarter::~GdbServerStarter()
{
    delete d;
}

void GdbServerStarter::handleRemoteError(const QString &errorMsg)
{
    AsynchronousMessageBox::critical(tr("Remote Error"), errorMsg);
}

void GdbServerStarter::portGathererError(const QString &text)
{
    logMessage(tr("Could not retrieve list of free ports:"));
    logMessage(text);
    logMessage(tr("Process aborted"));
}

void GdbServerStarter::run()
{
    QTC_ASSERT(d->device, return);
    connect(&d->gatherer, &DeviceUsedPortsGatherer::error,
            this, &GdbServerStarter::portGathererError);
    connect(&d->gatherer, &DeviceUsedPortsGatherer::portListReady,
            this, &GdbServerStarter::portListReady);
    d->gatherer.start(d->device);
}

void GdbServerStarter::portListReady()
{
    PortList ports = d->device->freePorts();
    const Port port = d->gatherer.getNextFreePort(&ports);
    if (!port.isValid()) {
        QTC_ASSERT(false, /**/);
        emit logMessage(tr("Process aborted"));
        return;
    }

    connect(&d->runner, &SshRemoteProcessRunner::connectionError,
            this, &GdbServerStarter::handleConnectionError);
    connect(&d->runner, &SshRemoteProcessRunner::processStarted,
            this, &GdbServerStarter::handleProcessStarted);
    connect(&d->runner, &SshRemoteProcessRunner::readyReadStandardOutput,
            this, &GdbServerStarter::handleProcessOutputAvailable);
    connect(&d->runner, &SshRemoteProcessRunner::readyReadStandardError,
            this, &GdbServerStarter::handleProcessErrorOutput);
    connect(&d->runner, &SshRemoteProcessRunner::processClosed,
            this, &GdbServerStarter::handleProcessClosed);

    QByteArray gdbServerPath = d->device->debugServerPath().toUtf8();
    if (gdbServerPath.isEmpty())
        gdbServerPath = "gdbserver";
    QByteArray cmd = gdbServerPath + " --attach :"
            + QByteArray::number(port.number()) + ' ' + QByteArray::number(d->process.pid);
    logMessage(tr("Running command: %1").arg(QString::fromLatin1(cmd)));
    d->runner.run(cmd, d->device->sshParameters());
}

void GdbServerStarter::handleConnectionError()
{
    logMessage(tr("Connection error: %1").arg(d->runner.lastConnectionErrorString()));
}

void GdbServerStarter::handleProcessStarted()
{
    logMessage(tr("Starting gdbserver..."));
}

void GdbServerStarter::handleProcessOutputAvailable()
{
    logMessage(QString::fromUtf8(d->runner.readAllStandardOutput().trimmed()));
}

void GdbServerStarter::handleProcessErrorOutput()
{
    const QByteArray ba = d->runner.readAllStandardError();
    logMessage(QString::fromUtf8(ba.trimmed()));
    // "Attached; pid = 16740"
    // "Listening on port 10000"
    foreach (const QByteArray &line, ba.split('\n')) {
        if (line.startsWith("Listening on port")) {
            const int port = line.mid(18).trimmed().toInt();
            logMessage(tr("Port %1 is now accessible.").arg(port));
            logMessage(tr("Server started on %1:%2")
                .arg(d->device->sshParameters().host).arg(port));
            if (d->attachToServer)
                attach(port);
        }
    }
}

void GdbServerStarter::attach(int port)
{
    QString sysroot = SysRootKitInformation::sysRoot(d->kit).toString();
    QString binary;
    QString localExecutable;
    QString candidate = sysroot + d->process.exe;
    if (QFileInfo::exists(candidate))
        localExecutable = candidate;
    if (localExecutable.isEmpty()) {
        binary = d->process.cmdLine.section(QLatin1Char(' '), 0, 0);
        candidate = sysroot + QLatin1Char('/') + binary;
        if (QFileInfo::exists(candidate))
            localExecutable = candidate;
    }
    if (localExecutable.isEmpty()) {
        candidate = sysroot + QLatin1String("/usr/bin/") + binary;
        if (QFileInfo::exists(candidate))
            localExecutable = candidate;
    }
    if (localExecutable.isEmpty()) {
        candidate = sysroot + QLatin1String("/bin/") + binary;
        if (QFileInfo::exists(candidate))
            localExecutable = candidate;
    }
    if (localExecutable.isEmpty()) {
        AsynchronousMessageBox::warning(tr("Warning"),
            tr("Cannot find local executable for remote process \"%1\".")
                .arg(d->process.exe));
        return;
    }

    QList<Abi> abis = Abi::abisOfBinary(FileName::fromString(localExecutable));
    if (abis.isEmpty()) {
        AsynchronousMessageBox::warning(tr("Warning"),
            tr("Cannot find ABI for remote process \"%1\".")
                .arg(d->process.exe));
        return;
    }

    DebuggerRunParameters rp;
    rp.masterEngineType = GdbEngineType;
    rp.connParams.host = d->device->sshParameters().host;
    rp.connParams.port = port;
    rp.remoteChannel = rp.connParams.host + QLatin1Char(':') + QString::number(rp.connParams.port);
    rp.displayName = tr("Remote: \"%1:%2\"").arg(rp.connParams.host).arg(port);
    rp.inferior.executable = localExecutable;
    rp.startMode = AttachToRemoteServer;
    rp.closeMode = KillAtClose;
    createAndScheduleRun(rp, d->kit);
}

void GdbServerStarter::handleProcessClosed(int status)
{
    logMessage(tr("Process gdbserver finished. Status: %1").arg(status));
}

void GdbServerStarter::logMessage(const QString &line)
{
    d->dialog->logMessage(line);
}

} // namespace Internal
} // namespace Debugger
