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

#include "startgdbserverdialog.h"

#include "debuggercore.h"
#include "debuggermainwindow.h"
#include "debuggerplugin.h"
#include "debuggerkitinformation.h"
#include "debuggerrunner.h"
#include "debuggerruncontrolfactory.h"
#include "debuggerstartparameters.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/kitchooser.h>
#include <projectexplorer/devicesupport/deviceprocesslist.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/pathchooser.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

#include <QVariant>
#include <QMessageBox>

using namespace Core;
using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;


namespace Debugger {
namespace Internal {

class StartGdbServerDialogPrivate
{
public:
    StartGdbServerDialogPrivate() {}

    DeviceProcessesDialog *dialog;
    bool startServerOnly;
    DeviceProcess process;
    Kit *kit;
    IDevice::ConstPtr device;

    DeviceUsedPortsGatherer gatherer;
    SshRemoteProcessRunner runner;
};

GdbServerStarter::GdbServerStarter(DeviceProcessesDialog *dlg, bool startServerOnly)
  : QObject(dlg)
{
    d = new StartGdbServerDialogPrivate;
    d->dialog = dlg;
    d->kit = dlg->kitChooser()->currentKit();
    d->process = dlg->currentProcess();
    d->device = DeviceKitInformation::device(d->kit);
    d->startServerOnly = startServerOnly;
}

GdbServerStarter::~GdbServerStarter()
{
    delete d;
}

void GdbServerStarter::handleRemoteError(const QString &errorMsg)
{
    QMessageBox::critical(0, tr("Remote Error"), errorMsg);
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
    connect(&d->gatherer, SIGNAL(error(QString)), SLOT(portGathererError(QString)));
    connect(&d->gatherer, SIGNAL(portListReady()), SLOT(portListReady()));
    d->gatherer.start(d->device);
}

void GdbServerStarter::portListReady()
{
    PortList ports = d->device->freePorts();
    const int port = d->gatherer.getNextFreePort(&ports);
    if (port == -1) {
        QTC_ASSERT(false, /**/);
        emit logMessage(tr("Process aborted"));
        return;
    }

    connect(&d->runner, SIGNAL(connectionError()), SLOT(handleConnectionError()));
    connect(&d->runner, SIGNAL(processStarted()), SLOT(handleProcessStarted()));
    connect(&d->runner, SIGNAL(readyReadStandardOutput()), SLOT(handleProcessOutputAvailable()));
    connect(&d->runner, SIGNAL(readyReadStandardError()), SLOT(handleProcessErrorOutput()));
    connect(&d->runner, SIGNAL(processClosed(int)), SLOT(handleProcessClosed(int)));

    QByteArray cmd = "/usr/bin/gdbserver --attach :"
            + QByteArray::number(port) + ' ' + QByteArray::number(d->process.pid);
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
            if (!d->startServerOnly)
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
    if (QFileInfo(candidate).exists())
        localExecutable = candidate;
    if (localExecutable.isEmpty()) {
        binary = d->process.cmdLine.section(QLatin1Char(' '), 0, 0);
        candidate = sysroot + QLatin1Char('/') + binary;
        if (QFileInfo(candidate).exists())
            localExecutable = candidate;
    }
    if (localExecutable.isEmpty()) {
        candidate = sysroot + QLatin1String("/usr/bin/") + binary;
        if (QFileInfo(candidate).exists())
            localExecutable = candidate;
    }
    if (localExecutable.isEmpty()) {
        candidate = sysroot + QLatin1String("/bin/") + binary;
        if (QFileInfo(candidate).exists())
            localExecutable = candidate;
    }
    if (localExecutable.isEmpty()) {
        QMessageBox::warning(DebuggerPlugin::mainWindow(), tr("Warning"),
            tr("Cannot find local executable for remote process \"%1\".")
                .arg(d->process.exe));
        return;
    }

    QList<Abi> abis = Abi::abisOfBinary(Utils::FileName::fromString(localExecutable));
    if (abis.isEmpty()) {
        QMessageBox::warning(DebuggerPlugin::mainWindow(), tr("Warning"),
            tr("Cannot find ABI for remote process \"%1\".")
                .arg(d->process.exe));
        return;
    }

    DebuggerStartParameters sp;
    QTC_ASSERT(fillParameters(&sp, d->kit), return);
    sp.masterEngineType = GdbEngineType;
    sp.connParams.port = port;
    sp.remoteChannel = sp.connParams.host + QLatin1Char(':') + QString::number(sp.connParams.port);
    sp.displayName = tr("Remote: \"%1:%2\"").arg(sp.connParams.host).arg(port);
    sp.executable = localExecutable;
    sp.startMode = AttachToRemoteServer;
    sp.closeMode = KillAtClose;
    DebuggerRunControlFactory::createAndScheduleRun(sp);
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
