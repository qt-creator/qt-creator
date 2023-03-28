// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "linuxdevicetester.h"

#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/devicesupport/filetransfer.h>

#include <utils/algorithm.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;
using namespace Utils::Tasking;

namespace RemoteLinux {
namespace Internal {

struct TransferStorage
{
    bool sftpWorks = false;
};

class GenericLinuxDeviceTesterPrivate
{
public:
    GenericLinuxDeviceTesterPrivate(GenericLinuxDeviceTester *tester) : q(tester) {}

    QStringList commandsToTest() const;

    TaskItem echoTask() const;
    TaskItem unameTask() const;
    TaskItem gathererTask() const;
    TaskItem transferTask(FileTransferMethod method,
                          const TreeStorage<TransferStorage> &storage) const;
    TaskItem transferTasks() const;
    TaskItem commandTask(const QString &commandName) const;
    TaskItem commandTasks() const;

    GenericLinuxDeviceTester *q = nullptr;
    IDevice::Ptr m_device;
    std::unique_ptr<TaskTree> m_taskTree;
    QStringList m_extraCommands;
    QList<TaskItem> m_extraTests;
};

static const char s_echoContents[] = "Hello Remote World!";

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

TaskItem GenericLinuxDeviceTesterPrivate::echoTask() const
{
    const auto setup = [this](QtcProcess &process) {
        emit q->progressMessage(Tr::tr("Sending echo to device..."));
        process.setCommand({m_device->filePath("echo"), {s_echoContents}});
    };
    const auto done = [this](const QtcProcess &process) {
        const QString reply = process.cleanedStdOut().chopped(1); // Remove trailing '\n'
        if (reply != s_echoContents)
            emit q->errorMessage(Tr::tr("Device replied to echo with unexpected contents.") + '\n');
        else
            emit q->progressMessage(Tr::tr("Device replied to echo with expected contents.") + '\n');
    };
    const auto error = [this](const QtcProcess &process) {
        const QString stdErrOutput = process.cleanedStdErr();
        if (!stdErrOutput.isEmpty())
            emit q->errorMessage(Tr::tr("echo failed: %1").arg(stdErrOutput) + '\n');
        else
            emit q->errorMessage(Tr::tr("echo failed.") + '\n');
    };
    return Process(setup, done, error);
}

TaskItem GenericLinuxDeviceTesterPrivate::unameTask() const
{
    const auto setup = [this](QtcProcess &process) {
        emit q->progressMessage(Tr::tr("Checking kernel version..."));
        process.setCommand({m_device->filePath("uname"), {"-rsm"}});
    };
    const auto done = [this](const QtcProcess &process) {
        emit q->progressMessage(process.cleanedStdOut());
    };
    const auto error = [this](const QtcProcess &process) {
        const QString stdErrOutput = process.cleanedStdErr();
        if (!stdErrOutput.isEmpty())
            emit q->errorMessage(Tr::tr("uname failed: %1").arg(stdErrOutput) + '\n');
        else
            emit q->errorMessage(Tr::tr("uname failed.") + '\n');
    };
    return Tasking::Group {
        optional,
        Process(setup, done, error)
    };
}

TaskItem GenericLinuxDeviceTesterPrivate::gathererTask() const
{
    const auto setup = [this](DeviceUsedPortsGatherer &gatherer) {
        emit q->progressMessage(Tr::tr("Checking if specified ports are available..."));
        gatherer.setDevice(m_device);
    };
    const auto done = [this](const DeviceUsedPortsGatherer &gatherer) {
        if (gatherer.usedPorts().isEmpty()) {
            emit q->progressMessage(Tr::tr("All specified ports are available.") + '\n');
        } else {
            const QString portList = transform(gatherer.usedPorts(), [](const Port &port) {
                return QString::number(port.number());
            }).join(", ");
            emit q->errorMessage(Tr::tr("The following specified ports are currently in use: %1")
                .arg(portList) + '\n');
        }
    };
    const auto error = [this](const DeviceUsedPortsGatherer &gatherer) {
        emit q->errorMessage(Tr::tr("Error gathering ports: %1").arg(gatherer.errorString()) + '\n');
    };
    return PortGatherer(setup, done, error);
}

TaskItem GenericLinuxDeviceTesterPrivate::transferTask(FileTransferMethod method,
                                          const TreeStorage<TransferStorage> &storage) const
{
    const auto setup = [this, method](FileTransfer &transfer) {
        emit q->progressMessage(Tr::tr("Checking whether \"%1\" works...")
                                .arg(FileTransfer::transferMethodName(method)));
        transfer.setTransferMethod(method);
        transfer.setTestDevice(m_device);
    };
    const auto done = [this, method, storage](const FileTransfer &) {
        const QString methodName = FileTransfer::transferMethodName(method);
        emit q->progressMessage(Tr::tr("\"%1\" is functional.\n").arg(methodName));
        if (method == FileTransferMethod::Rsync)
            m_device->setExtraData(Constants::SupportsRSync, true);
        else
            storage->sftpWorks = true;
    };
    const auto error = [this, method, storage](const FileTransfer &transfer) {
        const QString methodName = FileTransfer::transferMethodName(method);
        const ProcessResultData resultData = transfer.resultData();
        QString error;
        if (resultData.m_error == QProcess::FailedToStart) {
            error = Tr::tr("Failed to start \"%1\": %2\n").arg(methodName, resultData.m_errorString);
        } else if (resultData.m_exitStatus == QProcess::CrashExit) {
            error = Tr::tr("\"%1\" crashed.\n").arg(methodName);
        } else if (resultData.m_exitCode != 0) {
            error = Tr::tr("\"%1\" failed with exit code %2: %3\n")
                    .arg(methodName).arg(resultData.m_exitCode).arg(resultData.m_errorString);
        }
        emit q->errorMessage(error);
        if (method == FileTransferMethod::Rsync) {
            m_device->setExtraData(Constants::SupportsRSync, false);
            if (!storage->sftpWorks)
                return;
            const QString sftp = FileTransfer::transferMethodName(FileTransferMethod::Sftp);
            const QString rsync = methodName;
            emit q->progressMessage(Tr::tr("\"%1\" will be used for deployment, because \"%2\" "
                                           "is not available.\n").arg(sftp, rsync));
        }
    };
    return TransferTest(setup, done, error);
}

TaskItem GenericLinuxDeviceTesterPrivate::transferTasks() const
{
    TreeStorage<TransferStorage> storage;
    return Tasking::Group {
        continueOnDone,
        Storage(storage),
        transferTask(FileTransferMethod::GenericCopy, storage),
        transferTask(FileTransferMethod::Sftp, storage),
        transferTask(FileTransferMethod::Rsync, storage),
        OnGroupError([this] { emit q->errorMessage(Tr::tr("Deployment to this device will not "
                                                          "work out of the box.\n"));
        })
    };
}

TaskItem GenericLinuxDeviceTesterPrivate::commandTask(const QString &commandName) const
{
    const auto setup = [this, commandName](QtcProcess &process) {
        emit q->progressMessage(Tr::tr("%1...").arg(commandName));
        CommandLine command{m_device->filePath("/bin/sh"), {"-c"}};
        command.addArgs(QLatin1String("\"command -v %1\"").arg(commandName), CommandLine::Raw);
        process.setCommand(command);
    };
    const auto done = [this, commandName](const QtcProcess &) {
        emit q->progressMessage(Tr::tr("%1 found.").arg(commandName));
    };
    const auto error = [this, commandName](const QtcProcess &process) {
        const QString message = process.result() == ProcessResult::StartFailed
                ? Tr::tr("An error occurred while checking for %1.").arg(commandName)
                  + '\n' + process.errorString()
                : Tr::tr("%1 not found.").arg(commandName);
        emit q->errorMessage(message);
    };
    return Process(setup, done, error);
}

TaskItem GenericLinuxDeviceTesterPrivate::commandTasks() const
{
    QList<TaskItem> tasks {continueOnError};
    tasks.append(OnGroupSetup([this] {
        emit q->progressMessage(Tr::tr("Checking if required commands are available..."));
    }));
    for (const QString &commandName : commandsToTest())
        tasks.append(commandTask(commandName));
    return Tasking::Group {tasks};
}

} // namespace Internal

using namespace Internal;

GenericLinuxDeviceTester::GenericLinuxDeviceTester(QObject *parent)
    : DeviceTester(parent), d(new GenericLinuxDeviceTesterPrivate(this))
{
}

GenericLinuxDeviceTester::~GenericLinuxDeviceTester() = default;

void GenericLinuxDeviceTester::setExtraCommandsToTest(const QStringList &extraCommands)
{
    d->m_extraCommands = extraCommands;
}

void GenericLinuxDeviceTester::setExtraTests(const QList<TaskItem> &extraTests)
{
    d->m_extraTests = extraTests;
}

void GenericLinuxDeviceTester::testDevice(const IDevice::Ptr &deviceConfiguration)
{
    QTC_ASSERT(!d->m_taskTree, return);

    d->m_device = deviceConfiguration;

    auto allFinished = [this](DeviceTester::TestResult testResult) {
        emit finished(testResult);
        d->m_taskTree.release()->deleteLater();
    };

    QList<TaskItem> taskItems = {
        d->echoTask(),
        d->unameTask(),
        d->gathererTask(),
        d->transferTasks()
    };
    if (!d->m_extraTests.isEmpty())
        taskItems << Group { d->m_extraTests };
    taskItems << d->commandTasks()
              << OnGroupDone(std::bind(allFinished, TestSuccess))
              << OnGroupError(std::bind(allFinished, TestFailure));

    d->m_taskTree.reset(new TaskTree(taskItems));
    d->m_taskTree->start();
}

void GenericLinuxDeviceTester::stopTest()
{
    QTC_ASSERT(d->m_taskTree, return);
    d->m_taskTree.reset();
    emit finished(TestFailure);
}

} // namespace RemoteLinux
