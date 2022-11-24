// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qnxdevicetester.h"
#include "qnxdevice.h"
#include "qnxtr.h"

#include <utils/qtcassert.h>

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
            this, &QnxDeviceTester::handleGenericTestFinished);

    connect(&m_varRunProcess, &QtcProcess::done, this, &QnxDeviceTester::handleVarRunDone);
}

void QnxDeviceTester::testDevice(const ProjectExplorer::IDevice::Ptr &deviceConfiguration)
{
    QTC_ASSERT(m_state == Inactive, return);
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

    m_deviceConfiguration = deviceConfiguration;
    m_state = GenericTest;

    QnxDevice::ConstPtr qnxDevice = m_deviceConfiguration.dynamicCast<const QnxDevice>();
    m_genericTester->setExtraCommandsToTest(
                s_commandsToTest + versionSpecificCommandsToTest(qnxDevice->qnxVersion()));
    m_genericTester->testDevice(deviceConfiguration);
}

void QnxDeviceTester::stopTest()
{
    QTC_ASSERT(m_state != Inactive, return);

    if (m_state == GenericTest)
        m_genericTester->stopTest();

    setFinished(TestFailure);
}

void QnxDeviceTester::handleGenericTestFinished(TestResult result)
{
    QTC_ASSERT(m_state == GenericTest, return);

    if (result == TestFailure) {
        setFinished(TestFailure);
        return;
    }

    m_state = VarRunTest;
    emit progressMessage(Tr::tr("Checking that files can be created in /var/run..."));
    const CommandLine cmd {m_deviceConfiguration->filePath("/bin/sh"),
        {"-c", QLatin1String("rm %1 > /dev/null 2>&1; echo ABC > %1 && rm %1")
                    .arg("/var/run/qtc_xxxx.pid")}};
    m_varRunProcess.setCommand(cmd);
    m_varRunProcess.start();
}

void QnxDeviceTester::handleVarRunDone()
{
    if (m_varRunProcess.result() == ProcessResult::FinishedWithSuccess) {
        emit progressMessage(Tr::tr("Files can be created in /var/run.") + '\n');
    } else {
        m_result = TestFailure;
        const QString message = m_varRunProcess.result() == ProcessResult::StartFailed
                ? Tr::tr("An error occurred while checking that files can be created in /var/run.")
                  + '\n' + m_varRunProcess.errorString()
                : Tr::tr("Files cannot be created in /var/run.");
        emit errorMessage(message + '\n');
    }
    setFinished(m_result);
}

void QnxDeviceTester::setFinished(TestResult result)
{
    if (m_result == TestSuccess)
        m_result = result;
    m_state = Inactive;
    disconnect(m_genericTester, nullptr, this, nullptr);
    m_varRunProcess.close();
    emit finished(m_result);
}

QStringList QnxDeviceTester::versionSpecificCommandsToTest(int versionNumber) const
{
    QStringList result;
    if (versionNumber > 0x060500)
        result << QLatin1String("slog2info");

    return result;
}

} // Qnx::Internal
