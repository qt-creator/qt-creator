// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxdevicetester.h"

#include "qnxconstants.h"
#include "qnxtr.h"

#include <utils/process.h>

using namespace Utils;

namespace Qnx::Internal {

QnxDeviceTester::QnxDeviceTester(QObject *parent)
    : RemoteLinux::GenericLinuxDeviceTester(parent)
{}

void QnxDeviceTester::testDevice(const ProjectExplorer::IDevice::Ptr &device)
{
    static const QStringList commandsToTest {
        "awk",
        "cat",
        "cut",
        "df",
        "grep",
        "kill",
        "netstat",
        "mkdir",
        "print",
        "printf",
        "pidin",
        "read",
        "rm",
        "sed",
        "sleep",
        "tail",
        "uname",
        "slog2info"
    };

    setExtraCommandsToTest(commandsToTest);

    using namespace Tasking;

    auto setupHandler = [device, this](Process &process) {
        emit progressMessage(Tr::tr("Checking that files can be created in %1...")
                .arg(Constants::QNX_TMP_DIR));
        const QString pidFile = QString("%1/qtc_xxxx.pid").arg(Constants::QNX_TMP_DIR);
        const CommandLine cmd(device->filePath("/bin/sh"),
            {"-c", QLatin1String("rm %1 > /dev/null 2>&1; echo ABC > %1 && rm %1").arg(pidFile)});
        process.setCommand(cmd);
    };
    auto doneHandler = [this](const Process &) {
        emit progressMessage(Tr::tr("Files can be created in /var/run.") + '\n');
    };
    auto errorHandler = [this](const Process &process) {
        const QString message = process.result() == ProcessResult::StartFailed
                ? Tr::tr("An error occurred while checking that files can be created in %1.")
                    .arg(Constants::QNX_TMP_DIR) + '\n' + process.errorString()
                : Tr::tr("Files cannot be created in %1.").arg(Constants::QNX_TMP_DIR);
        emit errorMessage(message + '\n');
    };
    setExtraTests({ProcessTask(setupHandler, doneHandler, errorHandler)});

    RemoteLinux::GenericLinuxDeviceTester::testDevice(device);
}

} // Qnx::Internal
