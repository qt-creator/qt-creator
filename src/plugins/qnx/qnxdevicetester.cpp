/**************************************************************************
**
** Copyright (C) 2015 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qnxdevicetester.h"
#include "qnxdeviceconfiguration.h"

#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>

using namespace Qnx;
using namespace Qnx::Internal;

QnxDeviceTester::QnxDeviceTester(QObject *parent)
    : ProjectExplorer::DeviceTester(parent)
    , m_result(TestSuccess)
    , m_state(Inactive)
    , m_currentCommandIndex(-1)
{
    m_genericTester = new RemoteLinux::GenericLinuxDeviceTester(this);
    connect(m_genericTester, SIGNAL(progressMessage(QString)), SIGNAL(progressMessage(QString)));
    connect(m_genericTester, SIGNAL(errorMessage(QString)), SIGNAL(errorMessage(QString)));
    connect(m_genericTester, SIGNAL(finished(ProjectExplorer::DeviceTester::TestResult)),
        SLOT(handleGenericTestFinished(ProjectExplorer::DeviceTester::TestResult)));

    m_processRunner = new QSsh::SshRemoteProcessRunner(this);
    connect(m_processRunner, SIGNAL(connectionError()), SLOT(handleConnectionError()));
    connect(m_processRunner, SIGNAL(processClosed(int)), SLOT(handleProcessFinished(int)));

    m_commandsToTest << QLatin1String("awk")
                     << QLatin1String("grep")
                     << QLatin1String("kill")
                     << QLatin1String("netstat")
                     << QLatin1String("print")
                     << QLatin1String("printf")
                     << QLatin1String("ps")
                     << QLatin1String("read")
                     << QLatin1String("sed")
                     << QLatin1String("sleep")
                     << QLatin1String("uname");
}

void QnxDeviceTester::testDevice(const ProjectExplorer::IDevice::ConstPtr &deviceConfiguration)
{
    QTC_ASSERT(m_state == Inactive, return);

    m_deviceConfiguration = deviceConfiguration;

    m_state = GenericTest;
    m_genericTester->testDevice(deviceConfiguration);
}

void QnxDeviceTester::stopTest()
{
    QTC_ASSERT(m_state != Inactive, return);

    switch (m_state) {
    case Inactive:
        break;
    case GenericTest:
        m_genericTester->stopTest();
        break;
    case CommandsTest:
        m_processRunner->cancel();
        break;
    }

    m_result = TestFailure;
    setFinished();
}

void QnxDeviceTester::handleGenericTestFinished(TestResult result)
{
    QTC_ASSERT(m_state == GenericTest, return);

    if (result == TestFailure) {
        m_result = TestFailure;
        setFinished();
        return;
    }

    m_state = CommandsTest;

    QnxDeviceConfiguration::ConstPtr qnxDevice = m_deviceConfiguration.dynamicCast<const QnxDeviceConfiguration>();
    m_commandsToTest.append(versionSpecificCommandsToTest(qnxDevice->qnxVersion()));

    testNextCommand();
}

void QnxDeviceTester::handleProcessFinished(int exitStatus)
{
    QTC_ASSERT(m_state == CommandsTest, return);

    const QString command = m_commandsToTest[m_currentCommandIndex];
    if (exitStatus == QSsh::SshRemoteProcess::NormalExit) {
        if (m_processRunner->processExitCode() == 0) {
            emit progressMessage(tr("%1 found.").arg(command) + QLatin1Char('\n'));
        } else {
            emit errorMessage(tr("%1 not found.").arg(command) + QLatin1Char('\n'));
            m_result = TestFailure;
        }
    } else {
        emit errorMessage(tr("An error occurred checking for %1.").arg(command)  + QLatin1Char('\n'));
        m_result = TestFailure;
    }
    testNextCommand();
}

void QnxDeviceTester::handleConnectionError()
{
    QTC_ASSERT(m_state == CommandsTest, return);

    m_result = TestFailure;
    emit errorMessage(tr("SSH connection error: %1").arg(m_processRunner->lastConnectionErrorString()) + QLatin1Char('\n'));
    setFinished();
}

void QnxDeviceTester::testNextCommand()
{
    ++m_currentCommandIndex;

    if (m_currentCommandIndex >= m_commandsToTest.size()) {
        setFinished();
        return;
    }

    QString command = m_commandsToTest[m_currentCommandIndex];
    emit progressMessage(tr("Checking for %1...").arg(command));

    m_processRunner->run("command -v " + command.toLatin1(), m_deviceConfiguration->sshParameters());
}

void QnxDeviceTester::setFinished()
{
    m_state = Inactive;
    disconnect(m_genericTester, 0, this, 0);
    if (m_processRunner)
        disconnect(m_processRunner, 0, this, 0);
    emit finished(m_result);
}

QStringList QnxDeviceTester::versionSpecificCommandsToTest(int versionNumber) const
{
    QStringList result;
    if (versionNumber > 0x060500)
        result << QLatin1String("slog2info");

    return result;
}
