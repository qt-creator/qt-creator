/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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
#include "maddedevicetester.h"

#include "maemoconstants.h"
#include "maemoglobal.h"

#include <remotelinux/linuxdevice.h>
#include <utils/qtcassert.h>
#include <ssh/sshremoteprocessrunner.h>

#include <QRegExp>

using namespace RemoteLinux;
using namespace QSsh;

namespace Madde {
namespace Internal {
namespace {
const char QmlToolingDirectory[] = "/usr/lib/qt4/plugins/qmltooling";
} // anonymous namespace

MaddeDeviceTester::MaddeDeviceTester(QObject *parent)
    : AbstractLinuxDeviceTester(parent),
      m_genericTester(new GenericLinuxDeviceTester(this)),
      m_state(Inactive),
      m_processRunner(0)
{
}

MaddeDeviceTester::~MaddeDeviceTester()
{
}

void MaddeDeviceTester::testDevice(const ProjectExplorer::IDevice::ConstPtr &deviceConfiguration)
{
    QTC_ASSERT(m_state == Inactive, return);

    m_deviceConfiguration = deviceConfiguration;
    m_result = TestSuccess;

    m_state = GenericTest;
    connect(m_genericTester, SIGNAL(progressMessage(QString)), SIGNAL(progressMessage(QString)));
    connect(m_genericTester, SIGNAL(errorMessage(QString)), SIGNAL(errorMessage(QString)));
    connect(m_genericTester, SIGNAL(finished(RemoteLinux::AbstractLinuxDeviceTester::TestResult)),
        SLOT(handleGenericTestFinished(RemoteLinux::AbstractLinuxDeviceTester::TestResult)));
    m_genericTester->testDevice(deviceConfiguration);
}

void MaddeDeviceTester::stopTest()
{
    QTC_ASSERT(m_state != Inactive, return);

    switch (m_state) {
    case Inactive:
        break;
    case GenericTest:
        m_genericTester->stopTest();
        break;
    case QtTest:
    case MadDeveloperTest:
    case QmlToolingTest:
        m_processRunner->cancel();
        break;
    }

    m_result = TestFailure;
    setFinished();
}

void MaddeDeviceTester::handleGenericTestFinished(TestResult result)
{
    QTC_ASSERT(m_state == GenericTest, return);

    if (result == TestFailure) {
        m_result = TestFailure;
        setFinished();
        return;
    }

    if (!m_processRunner)
        m_processRunner = new SshRemoteProcessRunner(this);
    connect(m_processRunner, SIGNAL(connectionError()), SLOT(handleConnectionError()));
    connect(m_processRunner, SIGNAL(processClosed(int)), SLOT(handleProcessFinished(int)));

    const QString qtInfoCmd = QLatin1String("dpkg-query -W -f "
            "'${Package} ${Version} ${Status}\n' 'libqt*' |grep ' installed$'");
    emit progressMessage(tr("Checking for Qt libraries..."));
    m_state = QtTest;
    m_processRunner->run(qtInfoCmd.toUtf8(), m_deviceConfiguration->sshParameters());
}

void MaddeDeviceTester::handleConnectionError()
{
    QTC_ASSERT(m_state != Inactive, return);

    emit errorMessage(tr("SSH connection error: %1\n")
        .arg(m_processRunner->lastConnectionErrorString()));
    m_result = TestFailure;
    setFinished();
}

void MaddeDeviceTester::handleProcessFinished(int exitStatus)
{
    switch (m_state) {
    case QtTest:
        handleQtTestFinished(exitStatus);
        break;
    case MadDeveloperTest:
        handleMadDeveloperTestFinished(exitStatus);
        break;
    case QmlToolingTest:
        handleQmlToolingTestFinished(exitStatus);
        break;
    default:
        qWarning("%s: Unexpected state %d.", Q_FUNC_INFO, m_state);
    }
}

void MaddeDeviceTester::handleQtTestFinished(int exitStatus)
{
    if (exitStatus != SshRemoteProcess::NormalExit
            || m_processRunner->processExitCode() != 0) {
        const QByteArray stdErr = m_processRunner->readAllStandardError();
        if (!stdErr.isEmpty()) {
            emit errorMessage(tr("Error checking for Qt libraries: %1\n")
                .arg(QString::fromUtf8(stdErr)));
        } else {
            emit errorMessage(tr("Error checking for Qt libraries.\n"));
        }

        m_result = TestFailure;
    } else {
        emit progressMessage(processedQtLibsList());
    }

    emit progressMessage(tr("Checking for connectivity support..."));
    m_state = MadDeveloperTest;
    m_processRunner->run(QString(QLatin1String("test -x") + MaemoGlobal::devrootshPath()).toUtf8(),
        m_deviceConfiguration->sshParameters());
}

void MaddeDeviceTester::handleMadDeveloperTestFinished(int exitStatus)
{
    if (exitStatus != SshRemoteProcess::NormalExit) {
        const QByteArray stdErr = m_processRunner->readAllStandardError();
        if (!stdErr.isEmpty()) {
            emit errorMessage(tr("Error checking for connectivity tool: %1\n")
                .arg(QString::fromUtf8(stdErr)));
        } else {
            emit errorMessage(tr("Error checking for connectivity tool.\n"));
        }
        m_result = TestFailure;
    } else if (m_processRunner->processExitCode() != 0) {
        QString message = tr("Connectivity tool not installed on device. "
            "Deployment currently not possible.");
        if (m_deviceConfiguration->type() == HarmattanOsType) {
            message += tr("Please switch the device to developer mode "
                "via Settings -> Security.");
        }
        emit errorMessage(message + QLatin1Char('\n'));
        m_result = TestFailure;
    } else {
        emit progressMessage(tr("Connectivity tool present.\n"));
    }

    if (m_deviceConfiguration->type() != HarmattanOsType) {
        setFinished();
        return;
    }

    emit progressMessage(tr("Checking for QML tooling support..."));
    m_state = QmlToolingTest;
    m_processRunner->run(QString(QLatin1String("test -d ")
        + QLatin1String(QmlToolingDirectory)).toUtf8(), m_deviceConfiguration->sshParameters());
}

void MaddeDeviceTester::handleQmlToolingTestFinished(int exitStatus)
{
    if (exitStatus != SshRemoteProcess::NormalExit) {
        const QByteArray stdErr = m_processRunner->readAllStandardError();
        if (!stdErr.isEmpty()) {
            emit errorMessage(tr("Error checking for QML tooling support: %1\n")
                .arg(QString::fromUtf8(stdErr)));
        } else {
            emit errorMessage(tr("Error checking for QML tooling support.\n"));
        }
        m_result = TestFailure;
    } else if (m_processRunner->processExitCode() != 0) {
        emit errorMessage(tr("Missing directory '%1'. You will not be able to do "
            "QML debugging on this device.\n").arg(QLatin1String(QmlToolingDirectory)));
        m_result = TestFailure;
    } else {
        emit progressMessage(tr("QML tooling support present.\n"));
    }

    setFinished();
}

QString MaddeDeviceTester::processedQtLibsList()
{
    QString unfilteredLibs = QString::fromUtf8(m_processRunner->readAllStandardOutput());
    QString filteredLibs;
    const QString patternString
            = QLatin1String("(\\S+) (\\S*(\\d+)\\.(\\d+)\\.(\\d+)\\S*) \\S+ \\S+ \\S+");
    QRegExp packagePattern(patternString);
    int index = packagePattern.indexIn(unfilteredLibs);
    if (index == -1)
        return tr("No Qt packages installed.");

    do {
        filteredLibs += QLatin1String("    ") + packagePattern.cap(1) + QLatin1String(": ")
            + packagePattern.cap(2) + QLatin1Char('\n');
        index = packagePattern.indexIn(unfilteredLibs, index + packagePattern.cap(0).length());
    } while (index != -1);
    return filteredLibs;
}

 void MaddeDeviceTester::setFinished()
{
    m_state = Inactive;
    disconnect(m_genericTester, 0, this, 0);
    if (m_processRunner)
        disconnect(m_processRunner, 0, this, 0);
    emit finished(m_result);
}

} // namespace Internal
} // namespace Madde
