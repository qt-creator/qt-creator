// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "linuxdevicetester.h"

#include "linuxdevice.h"
#include "remotelinuxtr.h"

#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class GenericLinuxDeviceTesterPrivate
{
public:
    GenericLinuxDeviceTesterPrivate(GenericLinuxDeviceTester *tester) : q(tester) {}

    QStringList commandsToTest() const;

    GroupItem connectionTask() const;
    GroupItem echoTask(const QString &contents) const;
    GroupItem unameTask() const;
    GroupItem gathererTask() const;
    GroupItem transferTask(FileTransferMethod method) const;
    GroupItem transferTasks() const;
    GroupItem commandTasks() const;

    GenericLinuxDeviceTester *q = nullptr;
    LinuxDevice::Ptr m_device;
    TaskTreeRunner m_taskTreeRunner;
    QStringList m_extraCommands;
    GroupItems m_extraTests;
};

QStringList GenericLinuxDeviceTesterPrivate::commandsToTest() const
{
    static const QStringList s_commandsToTest = {"base64",
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

    QStringList commands = s_commandsToTest + m_extraCommands;
    commands.removeDuplicates();
    Utils::sort(commands);
    return commands;
}

GroupItem GenericLinuxDeviceTesterPrivate::connectionTask() const
{
    const auto onSetup = [this](Async<bool> &task) {
        emit q->progressMessage(Tr::tr("Connecting to device..."));
        task.setConcurrentCallData([device = m_device] { return device->tryToConnect(); });
    };
    const auto onDone = [this](const Async<bool> &task) {
        const bool success = task.isResultAvailable() && task.result();
        if (success) {
            // TODO: For master: move the '\n' outside of Tr().
            emit q->progressMessage(Tr::tr("Connected. Now doing extended checks.") + "\n");
        } else {
            emit q->errorMessage(
                Tr::tr("Basic connectivity test failed, device is considered unusable.") + '\n');
        }
        return toDoneResult(success);
    };
    return AsyncTask<bool>(onSetup, onDone);
}

GroupItem GenericLinuxDeviceTesterPrivate::echoTask(const QString &contents) const
{
    const auto onSetup = [this, contents](Process &process) {
        emit q->progressMessage(Tr::tr("Sending echo to device..."));
        process.setCommand({m_device->filePath("echo"), {contents}});
    };
    const auto onDone = [this, contents](const Process &process, DoneWith result) {
        if (result != DoneWith::Success) {
            const QString stdErrOutput = process.cleanedStdErr();
            if (!stdErrOutput.isEmpty())
                emit q->errorMessage(Tr::tr("echo failed: %1").arg(stdErrOutput) + '\n');
            else
                emit q->errorMessage(Tr::tr("echo failed.") + '\n');
            return;
        }
        const QString reply = Utils::chopIfEndsWith(process.cleanedStdOut(), '\n');
        if (reply != contents) {
            emit q->errorMessage(Tr::tr("Device replied to echo with unexpected contents: \"%1\"")
                                     .arg(reply) + '\n');
        } else {
            emit q->progressMessage(Tr::tr("Device replied to echo with expected contents.")
                                    + '\n');
        }
    };
    return ProcessTask(onSetup, onDone);
}

GroupItem GenericLinuxDeviceTesterPrivate::unameTask() const
{
    const auto onSetup = [this](Process &process) {
        emit q->progressMessage(Tr::tr("Checking kernel version..."));
        process.setCommand({m_device->filePath("uname"), {"-rsm"}});
    };
    const auto onDone = [this](const Process &process, DoneWith result) {
        if (result == DoneWith::Success) {
            emit q->progressMessage(process.cleanedStdOut());
            return;
        }
        const QString stdErrOutput = process.cleanedStdErr();
        if (!stdErrOutput.isEmpty())
            emit q->errorMessage(Tr::tr("uname failed: %1").arg(stdErrOutput) + '\n');
        else
            emit q->errorMessage(Tr::tr("uname failed.") + '\n');
    };
    return Group {
        finishAllAndSuccess,
        ProcessTask(onSetup, onDone)
    };
}

GroupItem GenericLinuxDeviceTesterPrivate::gathererTask() const
{
    const Storage<PortsOutputData> portsStorage;

    const auto onSetup = [this] {
        emit q->progressMessage(Tr::tr("Checking if specified ports are available..."));
    };
    const auto onDone = [this, portsStorage] {
        const auto ports = *portsStorage;
        if (!ports) {
            emit q->errorMessage(Tr::tr("Error gathering ports: %1").arg(ports.error()) + '\n'
                                 + Tr::tr("Some tools will not work out of the box.\n"));
        } else if (ports->isEmpty()) {
            emit q->progressMessage(Tr::tr("All specified ports are available.") + '\n');
        } else {
            const QString portList = transform(*ports, [](const Port &port) {
                return QString::number(port.number());
            }).join(", ");
            emit q->errorMessage(Tr::tr("The following specified ports are currently in use: %1")
                .arg(portList) + '\n');
        }
        return true;
    };

    return Group {
        portsStorage,
        onGroupSetup(onSetup),
        m_device->portsGatheringRecipe(portsStorage),
        onGroupDone(onDone)
    };
}

GroupItem GenericLinuxDeviceTesterPrivate::transferTask(FileTransferMethod method) const
{
    const auto onSetup = [this, method](FileTransfer &transfer) {
        emit q->progressMessage(Tr::tr("Checking whether \"%1\" works...")
                                .arg(FileTransfer::transferMethodName(method)));
        transfer.setTransferMethod(method);
        transfer.setTestDevice(m_device);
    };
    const auto onDone = [this, method](const FileTransfer &transfer, DoneWith result) {
        const QString methodName = FileTransfer::transferMethodName(method);
        if (result == DoneWith::Success) {
            emit q->progressMessage(Tr::tr("\"%1\" is functional.\n").arg(methodName));
            if (method == FileTransferMethod::Rsync)
                m_device->setExtraData(Constants::SUPPORTS_RSYNC, true);
            else if (method == FileTransferMethod::Sftp)
                m_device->setExtraData(Constants::SUPPORTS_SFTP, true);
            return;
        }
        const ProcessResultData resultData = transfer.resultData();
        QString error;
        if (resultData.m_error == QProcess::FailedToStart) {
            error = Tr::tr("Failed to start \"%1\": %2").arg(methodName, resultData.m_errorString)
                    + "\n";
        } else if (resultData.m_exitStatus == QProcess::CrashExit) {
            error = Tr::tr("\"%1\" crashed.").arg(methodName) + "\n";
        } else if (resultData.m_exitCode != 0) {
            error = Tr::tr("\"%1\" failed with exit code %2: %3")
                        .arg(methodName)
                        .arg(resultData.m_exitCode)
                        .arg(resultData.m_errorString)
                    + "\n";
        }
        emit q->errorMessage(error);
        if (method == FileTransferMethod::Rsync)
            m_device->setExtraData(Constants::SUPPORTS_RSYNC, false);
        else if (method == FileTransferMethod::Sftp)
            m_device->setExtraData(Constants::SUPPORTS_SFTP, false);

        const QVariant supportsRSync = m_device->extraData(Constants::SUPPORTS_RSYNC);
        const QVariant supportsSftp = m_device->extraData(Constants::SUPPORTS_SFTP);
        if (supportsRSync.isValid() && !supportsRSync.toBool()
            && supportsSftp.isValid() && !supportsSftp.toBool()) {
            const QString generic = FileTransfer::transferMethodName(FileTransferMethod::GenericCopy);
            const QString sftp = FileTransfer::transferMethodName(FileTransferMethod::Sftp);
            const QString rsync = FileTransfer::transferMethodName(FileTransferMethod::Rsync);
            emit q->progressMessage(Tr::tr("\"%1\" will be used for deployment, because \"%2\" "
                                           "and \"%3\" are not available.")
                                        .arg(generic, sftp, rsync)
                                    + "\n");
        }
    };
    return FileTransferTestTask(onSetup, onDone);
}

GroupItem GenericLinuxDeviceTesterPrivate::transferTasks() const
{
    return Group {
        continueOnSuccess,
        transferTask(FileTransferMethod::GenericCopy),
        transferTask(FileTransferMethod::Sftp),
        transferTask(FileTransferMethod::Rsync),
        onGroupDone([this] {
            emit q->errorMessage(Tr::tr("Deployment to this device will not work out of the box.")
                                 + "\n");
        }, CallDoneIf::Error)
    };
}

GroupItem GenericLinuxDeviceTesterPrivate::commandTasks() const
{
    const LoopList iterator(commandsToTest());

    const auto onSetup = [this, iterator](Process &process) {
        const QString &commandName = *iterator;
        emit q->progressMessage(Tr::tr("%1...").arg(commandName));
        CommandLine command{m_device->filePath("/bin/sh"), {"-c"}};
        command.addArgs(QLatin1String("\"command -v %1\"").arg(commandName), CommandLine::Raw);
        process.setCommand(command);
    };
    const auto onDone = [this, iterator](const Process &process, DoneWith result) {
        const QString &commandName = *iterator;
        if (result == DoneWith::Success) {
            emit q->progressMessage(Tr::tr("%1 found.").arg(commandName));
            return;
        }
        const QString message = process.result() == ProcessResult::StartFailed
                ? Tr::tr("An error occurred while checking for %1.").arg(commandName)
                  + '\n' + process.errorString()
                : Tr::tr("%1 not found.").arg(commandName);
        emit q->errorMessage(message);
    };

    return For (iterator) >> Do {
        continueOnError,
        onGroupSetup([this] {
            emit q->progressMessage(Tr::tr("Checking if required commands are available..."));
        }),
        ProcessTask(onSetup, onDone)
    };
}

} // namespace Internal

using namespace Internal;

GenericLinuxDeviceTester::GenericLinuxDeviceTester(const IDevice::Ptr &device, QObject *parent)
    : DeviceTester(device, parent), d(new GenericLinuxDeviceTesterPrivate(this))
{
    connect(&d->m_taskTreeRunner, &TaskTreeRunner::done, this, [this](DoneWith result) {
        emit finished(result == DoneWith::Success ? TestSuccess : TestFailure);
    });
}

GenericLinuxDeviceTester::~GenericLinuxDeviceTester() = default;

void GenericLinuxDeviceTester::setExtraCommandsToTest(const QStringList &extraCommands)
{
    d->m_extraCommands = extraCommands;
}

void GenericLinuxDeviceTester::setExtraTests(const GroupItems &extraTests)
{
    d->m_extraTests = extraTests;
}

void GenericLinuxDeviceTester::testDevice()
{
    QTC_ASSERT(!d->m_taskTreeRunner.isRunning(), return);

    d->m_device = std::static_pointer_cast<LinuxDevice>(device());

    const Group root {
        d->connectionTask(),
        d->echoTask("Hello"), // No quoting necessary
        d->echoTask("Hello Remote World!"), // Checks quoting, too.
        d->unameTask(),
        d->gathererTask(),
        d->transferTasks(),
        d->m_extraTests,
        d->commandTasks()
    };
    d->m_taskTreeRunner.start(root);
}

void GenericLinuxDeviceTester::stopTest()
{
    QTC_ASSERT(d->m_taskTreeRunner.isRunning(), return);
    d->m_taskTreeRunner.reset();
    emit finished(TestFailure);
}

} // namespace RemoteLinux
