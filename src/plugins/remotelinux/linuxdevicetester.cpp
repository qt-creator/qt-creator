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

#include "linuxdevicetester.h"

#include "remotelinux_constants.h"

#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <utils/algorithm.h>
#include <utils/port.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {
namespace {

enum State { Inactive,
             TestingEcho,
             TestingUname,
             TestingPorts,
             TestingSftp,
             TestingRsync,
             TestingCommands };

} // anonymous namespace

class GenericLinuxDeviceTesterPrivate
{
public:
    IDevice::Ptr device;
    QtcProcess echoProcess;
    QtcProcess unameProcess;
    DeviceUsedPortsGatherer portsGatherer;
    FileTransfer fileTransfer;
    State state = Inactive;
    bool sftpWorks = false;
    int currentCommandIndex = 0;
    bool commandFailed = false;
    QtcProcess commandsProcess;
};

const QStringList s_commandsToTest = {"base64",
                                      "cat",
                                      "chmod",
                                      "cp",
                                      "cut",
                                      "dd",
                                      "df",
                                      "echo",
                                      "eval",
                                      "exit",
                                      "kill",
                                      "ls",
                                      "mkdir",
                                      "mkfifo",
                                      "mktemp",
                                      "mv",
                                      "printf",
                                      "read",
                                      "readlink",
                                      "rm",
                                      "sed",
                                      "sh",
                                      "shift",
                                      "stat",
                                      "tail",
                                      "test",
                                      "trap",
                                      "touch",
                                      "which"};
// other possible commands (checked for qnx):
// "awk", "grep", "netstat", "print", "pidin", "sleep", "uname"

} // namespace Internal

using namespace Internal;

GenericLinuxDeviceTester::GenericLinuxDeviceTester(QObject *parent)
    : DeviceTester(parent), d(new GenericLinuxDeviceTesterPrivate)
{
    connect(&d->echoProcess, &QtcProcess::done, this,
            &GenericLinuxDeviceTester::handleEchoDone);
    connect(&d->unameProcess, &QtcProcess::done, this,
            &GenericLinuxDeviceTester::handleUnameDone);
    connect(&d->portsGatherer, &DeviceUsedPortsGatherer::error,
            this, &GenericLinuxDeviceTester::handlePortsGathererError);
    connect(&d->portsGatherer, &DeviceUsedPortsGatherer::portListReady,
            this, &GenericLinuxDeviceTester::handlePortsGathererDone);
    connect(&d->fileTransfer, &FileTransfer::done,
            this, &GenericLinuxDeviceTester::handleFileTransferDone);
    connect(&d->commandsProcess, &QtcProcess::done,
            this, &GenericLinuxDeviceTester::handleCommandDone);
}

GenericLinuxDeviceTester::~GenericLinuxDeviceTester() = default;

void GenericLinuxDeviceTester::testDevice(const IDevice::Ptr &deviceConfiguration)
{
    QTC_ASSERT(d->state == Inactive, return);

    d->device = deviceConfiguration;

    testEcho();
}

void GenericLinuxDeviceTester::stopTest()
{
    QTC_ASSERT(d->state != Inactive, return);

    switch (d->state) {
    case TestingEcho:
        d->echoProcess.close();
        break;
    case TestingPorts:
        d->portsGatherer.stop();
        break;
    case TestingUname:
        d->unameProcess.close();
        break;
    case TestingSftp:
    case TestingRsync:
        d->fileTransfer.stop();
        break;
    case TestingCommands:
        d->commandsProcess.close();
        break;
    case Inactive:
        break;
    }

    setFinished(TestFailure);
}

static const char s_echoContents[] = "Hello Remote World!";

void GenericLinuxDeviceTester::testEcho()
{
    d->state = TestingEcho;
    emit progressMessage(tr("Sending echo to device..."));

    d->echoProcess.setCommand({d->device->filePath("echo"), {s_echoContents}});
    d->echoProcess.start();
}

void GenericLinuxDeviceTester::handleEchoDone()
{
    QTC_ASSERT(d->state == TestingEcho, return);
    if (d->echoProcess.result() != ProcessResult::FinishedWithSuccess) {
        const QByteArray stdErrOutput = d->echoProcess.readAllStandardError();
        if (!stdErrOutput.isEmpty())
            emit errorMessage(tr("echo failed: %1").arg(QString::fromUtf8(stdErrOutput)) + '\n');
        else
            emit errorMessage(tr("echo failed.") + '\n');
        setFinished(TestFailure);
        return;
    }

    const QString reply = d->echoProcess.cleanedStdOut().chopped(1); // Remove trailing \n
    if (reply != s_echoContents)
        emit errorMessage(tr("Device replied to echo with unexpected contents.") + '\n');
    else
        emit progressMessage(tr("Device replied to echo with expected contents.") + '\n');

    testUname();
}

void GenericLinuxDeviceTester::testUname()
{
    d->state = TestingUname;
    emit progressMessage(tr("Checking kernel version..."));

    d->unameProcess.setCommand({d->device->filePath("uname"), {"-rsm"}});
    d->unameProcess.start();
}

void GenericLinuxDeviceTester::handleUnameDone()
{
    QTC_ASSERT(d->state == TestingUname, return);

    if (!d->unameProcess.errorString().isEmpty() || d->unameProcess.exitCode() != 0) {
        const QByteArray stderrOutput = d->unameProcess.readAllStandardError();
        if (!stderrOutput.isEmpty())
            emit errorMessage(tr("uname failed: %1").arg(QString::fromUtf8(stderrOutput)) + QLatin1Char('\n'));
        else
            emit errorMessage(tr("uname failed.") + QLatin1Char('\n'));
    } else {
        emit progressMessage(QString::fromUtf8(d->unameProcess.readAllStandardOutput()));
    }

    testPortsGatherer();
}

void GenericLinuxDeviceTester::testPortsGatherer()
{
    d->state = TestingPorts;
    emit progressMessage(tr("Checking if specified ports are available..."));

    d->portsGatherer.start(d->device);
}

void GenericLinuxDeviceTester::handlePortsGathererError(const QString &message)
{
    QTC_ASSERT(d->state == TestingPorts, return);

    emit errorMessage(tr("Error gathering ports: %1").arg(message) + QLatin1Char('\n'));
    setFinished(TestFailure);
}

void GenericLinuxDeviceTester::handlePortsGathererDone()
{
    QTC_ASSERT(d->state == TestingPorts, return);

    if (d->portsGatherer.usedPorts().isEmpty()) {
        emit progressMessage(tr("All specified ports are available.") + QLatin1Char('\n'));
    } else {
        const QString portList = transform(d->portsGatherer.usedPorts(), [](const Port &port) {
            return QString::number(port.number());
        }).join(", ");
        emit errorMessage(tr("The following specified ports are currently in use: %1")
            .arg(portList) + QLatin1Char('\n'));
    }

    testFileTransfer(FileTransferMethod::Sftp);
}

void GenericLinuxDeviceTester::testFileTransfer(FileTransferMethod method)
{
    switch (method) {
    case FileTransferMethod::Sftp:  d->state = TestingSftp;  break;
    case FileTransferMethod::Rsync: d->state = TestingRsync; break;
    }
    emit progressMessage(tr("Checking whether \"%1\" works...")
                         .arg(FileTransfer::transferMethodName(method)));

    d->fileTransfer.setTransferMethod(method);
    d->fileTransfer.test(d->device);
}

void GenericLinuxDeviceTester::handleFileTransferDone(const ProcessResultData &resultData)
{
    QTC_ASSERT(d->state == TestingSftp || d->state == TestingRsync, return);

    bool succeeded = false;
    QString error;
    const QString methodName = FileTransfer::transferMethodName(d->fileTransfer.transferMethod());
    if (resultData.m_error == QProcess::FailedToStart) {
        error = tr("Failed to start \"%1\": %2\n").arg(methodName, resultData.m_errorString);
    } else if (resultData.m_exitStatus == QProcess::CrashExit) {
        error = tr("\"%1\" crashed.\n").arg(methodName);
    } else if (resultData.m_exitCode != 0) {
        error = tr("\"%1\" failed with exit code %2: %3\n")
                .arg(methodName).arg(resultData.m_exitCode).arg(resultData.m_errorString);
    } else {
        succeeded = true;
    }

    if (succeeded)
        emit progressMessage(tr("\"%1\" is functional.\n").arg(methodName));
    else
        emit errorMessage(error);

    if (d->state == TestingSftp) {
        d->sftpWorks = succeeded;
        testFileTransfer(FileTransferMethod::Rsync);
    } else {
        if (!succeeded) {
            if (d->sftpWorks) {
                emit progressMessage(tr("SFTP will be used for deployment, because rsync "
                                        "is not available.\n"));
            } else {
                emit errorMessage(tr("Deployment to this device will not work out of the box.\n"));
            }
        }
        d->device->setExtraData(Constants::SupportsRSync, succeeded);
        if (d->sftpWorks || succeeded)
            testCommands();
        else
            setFinished(TestFailure);
    }
}

void GenericLinuxDeviceTester::testCommands()
{
    d->state = TestingCommands;
    emit progressMessage(tr("Checking if required commands are available..."));

    d->currentCommandIndex = 0;
    d->commandFailed = false;
    testNextCommand();
}

void GenericLinuxDeviceTester::testNextCommand()
{
    d->commandsProcess.close();
    if (s_commandsToTest.size() == d->currentCommandIndex) {
        setFinished(d->commandFailed ? TestFailure : TestSuccess);
        return;
    }

    const QString commandName = s_commandsToTest[d->currentCommandIndex];
    emit progressMessage(tr("%1...").arg(commandName));
    CommandLine command{d->device->filePath("/bin/sh"), {"-c"}};
    command.addArgs(QLatin1String("\"command -v %1\"").arg(commandName), CommandLine::Raw);
    d->commandsProcess.setCommand(command);
    d->commandsProcess.start();
}

void GenericLinuxDeviceTester::handleCommandDone()
{
    QTC_ASSERT(d->state == TestingCommands, return);

    const QString command = s_commandsToTest[d->currentCommandIndex];
    if (d->commandsProcess.result() == ProcessResult::FinishedWithSuccess) {
        emit progressMessage(tr("%1 found.").arg(command));
    } else {
        d->commandFailed = true;
        const QString message = d->commandsProcess.result() == ProcessResult::StartFailed
                ? tr("An error occurred while checking for %1.").arg(command)
                  + '\n' + d->commandsProcess.errorString()
                : tr("%1 not found.").arg(command);
        emit errorMessage(message);
    }

    ++d->currentCommandIndex;
    testNextCommand();
}

void GenericLinuxDeviceTester::setFinished(TestResult result)
{
    d->state = Inactive;
    emit finished(result);
}

} // namespace RemoteLinux
