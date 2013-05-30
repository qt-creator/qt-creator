/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "qnxdevicetester.h"

#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>

using namespace Qnx;
using namespace Qnx::Internal;

QnxDeviceTester::QnxDeviceTester(QObject *parent)
    : RemoteLinux::AbstractLinuxDeviceTester(parent)
    , m_result(TestSuccess)
    , m_state(Inactive)
    , m_currentCommandIndex(-1)
{
    m_genericTester = new RemoteLinux::GenericLinuxDeviceTester(this);

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

    connect(m_genericTester, SIGNAL(progressMessage(QString)), SIGNAL(progressMessage(QString)));
    connect(m_genericTester, SIGNAL(errorMessage(QString)), SIGNAL(errorMessage(QString)));
    connect(m_genericTester, SIGNAL(finished(RemoteLinux::AbstractLinuxDeviceTester::TestResult)),
        SLOT(handleGenericTestFinished(RemoteLinux::AbstractLinuxDeviceTester::TestResult)));

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

void QnxDeviceTester::handleGenericTestFinished(RemoteLinux::AbstractLinuxDeviceTester::TestResult result)
{
    QTC_ASSERT(m_state == GenericTest, return);

    if (result == TestFailure) {
        m_result = TestFailure;
        setFinished();
        return;
    }

    m_state = CommandsTest;
    testNextCommand();
}

void QnxDeviceTester::handleProcessFinished(int exitStatus)
{
    QTC_ASSERT(m_state == CommandsTest, return);

    const QString command = m_commandsToTest[m_currentCommandIndex];
    if (exitStatus == QSsh::SshRemoteProcess::NormalExit) {
        if (m_processRunner->processExitCode() == 0) {
            emit progressMessage(tr("%1 found.\n").arg(command));
        } else {
            emit errorMessage(tr("%1 not found.\n").arg(command));
            m_result = TestFailure;
        }
    } else {
        emit errorMessage(tr("An error occurred checking for %1.\n").arg(command));
        m_result = TestFailure;
    }
    testNextCommand();
}

void QnxDeviceTester::handleConnectionError()
{
    QTC_ASSERT(m_state == CommandsTest, return);

    m_result = TestFailure;
    emit errorMessage(tr("SSH connection error: %1\n").arg(m_processRunner->lastConnectionErrorString()));
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
