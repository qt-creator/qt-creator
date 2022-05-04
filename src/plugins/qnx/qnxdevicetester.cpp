/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com), KDAB (info@kdab.com)
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

#include "qnxdevicetester.h"
#include "qnxdevice.h"

#include <utils/qtcassert.h>

using namespace Utils;

namespace Qnx {
namespace Internal {

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

    connect(&m_process, &QtcProcess::done, this, &QnxDeviceTester::handleProcessDone);

    m_commandsToTest << QLatin1String("awk")
                     << QLatin1String("cat")
                     << QLatin1String("cut")
                     << QLatin1String("df")
                     << QLatin1String("grep")
                     << QLatin1String("kill")
                     << QLatin1String("netstat")
                     << QLatin1String("mkdir")
                     << QLatin1String("print")
                     << QLatin1String("printf")
                     << QLatin1String("pidin")
                     << QLatin1String("read")
                     << QLatin1String("rm")
                     << QLatin1String("sed")
                     << QLatin1String("sleep")
                     << QLatin1String("tail")
                     << QLatin1String("uname");
}

void QnxDeviceTester::testDevice(const ProjectExplorer::IDevice::Ptr &deviceConfiguration)
{
    QTC_ASSERT(m_state == Inactive, return);
    m_deviceConfiguration = deviceConfiguration;
    m_state = GenericTest;
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
    emit progressMessage(tr("Checking that files can be created in /var/run..."));
    const CommandLine cmd {m_deviceConfiguration->mapToGlobalPath("/bin/sh"),
        {"-c", QLatin1String("rm %1 > /dev/null 2>&1; echo ABC > %1 && rm %1")
                    .arg("/var/run/qtc_xxxx.pid")}};
    m_process.setCommand(cmd);
    m_process.start();
}

void QnxDeviceTester::handleProcessDone()
{
    if (m_state == VarRunTest)
        handleVarRunDone();
    else if (m_state == CommandsTest)
        handleCommandDone();
    else
        QTC_CHECK(false);
}

void QnxDeviceTester::handleVarRunDone()
{
    if (m_process.result() == ProcessResult::FinishedWithSuccess) {
        emit progressMessage(tr("Files can be created in /var/run.") + '\n');
    } else {
        m_result = TestFailure;
        const QString message = m_process.result() == ProcessResult::StartFailed
                ? tr("An error occurred while checking that files can be created in /var/run.")
                  + '\n' + m_process.errorString()
                : tr("Files cannot be created in /var/run.");
        emit errorMessage(message + '\n');
    }

    QnxDevice::ConstPtr qnxDevice = m_deviceConfiguration.dynamicCast<const QnxDevice>();
    m_commandsToTest.append(versionSpecificCommandsToTest(qnxDevice->qnxVersion()));

    testNextCommand();
}

void QnxDeviceTester::handleCommandDone()
{
    const QString command = m_commandsToTest[m_currentCommandIndex];
    if (m_process.result() == ProcessResult::FinishedWithSuccess) {
        emit progressMessage(tr("%1 found.").arg(command) + '\n');
    } else {
        m_result = TestFailure;
        const QString message = m_process.result() == ProcessResult::StartFailed
                ? tr("An error occurred while checking for %1.").arg(command)
                  + '\n' + m_process.errorString()
                : tr("%1 not found.").arg(command);
        emit errorMessage(message + '\n');
    }

    ++m_currentCommandIndex;
    testNextCommand();
}

void QnxDeviceTester::testNextCommand()
{
    m_state = CommandsTest;
    m_process.close();
    if (m_commandsToTest.size() == m_currentCommandIndex) {
        setFinished(TestSuccess);
        return;
    }

    const QString command = m_commandsToTest[m_currentCommandIndex];
    emit progressMessage(tr("Checking for %1...").arg(command));
    const CommandLine cmd {m_deviceConfiguration->mapToGlobalPath("command"), {"-v", command}};
    m_process.setCommand(cmd);
    m_process.start();
}

void QnxDeviceTester::setFinished(TestResult result)
{
    if (m_result == TestSuccess)
        m_result = result;
    m_state = Inactive;
    disconnect(m_genericTester, nullptr, this, nullptr);
    m_process.close();
    emit finished(m_result);
}

QStringList QnxDeviceTester::versionSpecificCommandsToTest(int versionNumber) const
{
    QStringList result;
    if (versionNumber > 0x060500)
        result << QLatin1String("slog2info");

    return result;
}

} // namespace Internal
} // namespace Qnx
