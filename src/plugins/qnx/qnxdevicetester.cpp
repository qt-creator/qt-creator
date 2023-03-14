// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxdevicetester.h"

#include "qnxconstants.h"
#include "qnxdevice.h"
#include "qnxtr.h"

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace Utils;

namespace Qnx::Internal {

QnxDeviceTester::QnxDeviceTester(QObject *parent)
    : ProjectExplorer::DeviceTester(parent)
{
    m_genericTester = new RemoteLinux::GenericLinuxDeviceTester(this);
    connect(m_genericTester, &DeviceTester::progressMessage,
            this, &DeviceTester::progressMessage);
    connect(m_genericTester, &DeviceTester::errorMessage,
            this, &DeviceTester::errorMessage);
    connect(m_genericTester, &DeviceTester::finished,
            this, &QnxDeviceTester::finished);
}

static QStringList versionSpecificCommandsToTest(int versionNumber)
{
    if (versionNumber > 0x060500)
        return {"slog2info"};
    return {};
}

void QnxDeviceTester::testDevice(const ProjectExplorer::IDevice::Ptr &device)
{
    static const QStringList s_commandsToTest = {"awk",
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
                                                 "uname"};
    m_device = device;
    QnxDevice::ConstPtr qnxDevice = m_device.dynamicCast<const QnxDevice>();
    m_genericTester->setExtraCommandsToTest(
                s_commandsToTest + versionSpecificCommandsToTest(qnxDevice->qnxVersion()));

    using namespace Tasking;

    auto setupHandler = [this](QtcProcess &process) {
        emit progressMessage(Tr::tr("Checking that files can be created in %1...")
                .arg(Constants::QNX_TMP_DIR));
        const QString pidFile = QString("%1/qtc_xxxx.pid").arg(Constants::QNX_TMP_DIR);
        const CommandLine cmd(m_device->filePath("/bin/sh"),
            {"-c", QLatin1String("rm %1 > /dev/null 2>&1; echo ABC > %1 && rm %1").arg(pidFile)});
        process.setCommand(cmd);
    };
    auto doneHandler = [this](const QtcProcess &) {
        emit progressMessage(Tr::tr("Files can be created in /var/run.") + '\n');
    };
    auto errorHandler = [this](const QtcProcess &process) {
        const QString message = process.result() == ProcessResult::StartFailed
                ? Tr::tr("An error occurred while checking that files can be created in %1.")
                    .arg(Constants::QNX_TMP_DIR) + '\n' + process.errorString()
                : Tr::tr("Files cannot be created in %1.").arg(Constants::QNX_TMP_DIR);
        emit errorMessage(message + '\n');
    };
    m_genericTester->setExtraTests({Process(setupHandler, doneHandler, errorHandler)});

    m_genericTester->testDevice(device);
}

void QnxDeviceTester::stopTest()
{
    m_genericTester->stopTest();
}

} // Qnx::Internal
