// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "linuxdevicetester.h"

#include "remotelinuxtr.h"

#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/process.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

struct TransferStorage
{
    bool useGenericCopy = false;
};

class GenericLinuxDeviceTesterPrivate
{
public:
    GenericLinuxDeviceTesterPrivate(GenericLinuxDeviceTester *tester) : q(tester) {}

    QStringList commandsToTest() const;

    GroupItem echoTask(const QString &contents) const;
    GroupItem unameTask() const;
    GroupItem gathererTask() const;
    GroupItem transferTask(FileTransferMethod method,
                           const TreeStorage<TransferStorage> &storage) const;
    GroupItem transferTasks() const;
    GroupItem commandTask(const QString &commandName) const;
    GroupItem commandTasks() const;

    GenericLinuxDeviceTester *q = nullptr;
    IDevice::Ptr m_device;
    std::unique_ptr<TaskTree> m_taskTree;
    QStringList m_extraCommands;
    QList<GroupItem> m_extraTests;
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

GroupItem GenericLinuxDeviceTesterPrivate::echoTask(const QString &contents) const
{
    const auto setup = [this, contents](Process &process) {
        emit q->progressMessage(Tr::tr("Sending echo to device..."));
        process.setCommand({m_device->filePath("echo"), {contents}});
    };
    const auto done = [this, contents](const Process &process) {
        const QString reply = Utils::chopIfEndsWith(process.cleanedStdOut(), '\n');
        if (reply != contents)
            emit q->errorMessage(Tr::tr("Device replied to echo with unexpected contents: \"%1\"")
                    .arg(reply) + '\n');
        else
            emit q->progressMessage(Tr::tr("Device replied to echo with expected contents.") + '\n');
    };
    const auto error = [this](const Process &process) {
        const QString stdErrOutput = process.cleanedStdErr();
        if (!stdErrOutput.isEmpty())
            emit q->errorMessage(Tr::tr("echo failed: %1").arg(stdErrOutput) + '\n');
        else
            emit q->errorMessage(Tr::tr("echo failed.") + '\n');
    };
    return ProcessTask(setup, done, error);
}

GroupItem GenericLinuxDeviceTesterPrivate::unameTask() const
{
    const auto setup = [this](Process &process) {
        emit q->progressMessage(Tr::tr("Checking kernel version..."));
        process.setCommand({m_device->filePath("uname"), {"-rsm"}});
    };
    const auto done = [this](const Process &process) {
        emit q->progressMessage(process.cleanedStdOut());
    };
    const auto error = [this](const Process &process) {
        const QString stdErrOutput = process.cleanedStdErr();
        if (!stdErrOutput.isEmpty())
            emit q->errorMessage(Tr::tr("uname failed: %1").arg(stdErrOutput) + '\n');
        else
            emit q->errorMessage(Tr::tr("uname failed.") + '\n');
    };
    return Group {
        finishAllAndDone,
        ProcessTask(setup, done, error)
    };
}

GroupItem GenericLinuxDeviceTesterPrivate::gathererTask() const
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
        emit q->errorMessage(Tr::tr("Error gathering ports: %1").arg(gatherer.errorString()) + '\n'
                           + Tr::tr("Some tools will not work out of the box.\n"));
    };

    return Group {
        finishAllAndDone,
        DeviceUsedPortsGathererTask(setup, done, error)
    };
}

GroupItem GenericLinuxDeviceTesterPrivate::transferTask(FileTransferMethod method,
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
            m_device->setExtraData(Constants::SUPPORTS_RSYNC, true);
        else if (method == FileTransferMethod::Sftp)
            m_device->setExtraData(Constants::SUPPORTS_SFTP, true);
        else
            storage->useGenericCopy = true;
    };
    const auto error = [this, method, storage](const FileTransfer &transfer) {
        const QString methodName = FileTransfer::transferMethodName(method);
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
    return FileTransferTestTask(setup, done, error);
}

GroupItem GenericLinuxDeviceTesterPrivate::transferTasks() const
{
    TreeStorage<TransferStorage> storage;
    return Group{continueOnDone,
                 Tasking::Storage(storage),
                 transferTask(FileTransferMethod::GenericCopy, storage),
                 transferTask(FileTransferMethod::Sftp, storage),
                 transferTask(FileTransferMethod::Rsync, storage),
                 onGroupError([this] {
                     emit q->errorMessage(Tr::tr("Deployment to this device will not "
                                                 "work out of the box.")
                                          + "\n");
                 })};
}

GroupItem GenericLinuxDeviceTesterPrivate::commandTask(const QString &commandName) const
{
    const auto setup = [this, commandName](Process &process) {
        emit q->progressMessage(Tr::tr("%1...").arg(commandName));
        CommandLine command{m_device->filePath("/bin/sh"), {"-c"}};
        command.addArgs(QLatin1String("\"command -v %1\"").arg(commandName), CommandLine::Raw);
        process.setCommand(command);
    };
    const auto done = [this, commandName](const Process &) {
        emit q->progressMessage(Tr::tr("%1 found.").arg(commandName));
    };
    const auto error = [this, commandName](const Process &process) {
        const QString message = process.result() == ProcessResult::StartFailed
                ? Tr::tr("An error occurred while checking for %1.").arg(commandName)
                  + '\n' + process.errorString()
                : Tr::tr("%1 not found.").arg(commandName);
        emit q->errorMessage(message);
    };
    return ProcessTask(setup, done, error);
}

GroupItem GenericLinuxDeviceTesterPrivate::commandTasks() const
{
    QList<GroupItem> tasks {continueOnError};
    tasks.append(onGroupSetup([this] {
        emit q->progressMessage(Tr::tr("Checking if required commands are available..."));
    }));
    for (const QString &commandName : commandsToTest())
        tasks.append(commandTask(commandName));
    return Group {tasks};
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

void GenericLinuxDeviceTester::setExtraTests(const QList<GroupItem> &extraTests)
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

    QList<GroupItem> taskItems = {
        d->echoTask("Hello"), // No quoting necessary
        d->echoTask("Hello Remote World!"), // Checks quoting, too.
        d->unameTask(),
        d->gathererTask(),
        d->transferTasks()
    };
    if (!d->m_extraTests.isEmpty())
        taskItems << Group { d->m_extraTests };
    taskItems << d->commandTasks()
              << onGroupDone(std::bind(allFinished, TestSuccess))
              << onGroupError(std::bind(allFinished, TestFailure));

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
