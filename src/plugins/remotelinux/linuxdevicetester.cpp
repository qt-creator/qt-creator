/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "linuxdevicetester.h"

#include "linuxdeviceconfiguration.h"
#include "remotelinuxusedportsgatherer.h"

#include <utils/qtcassert.h>
#include <utils/ssh/sshremoteprocessrunner.h>
#include <utils/ssh/sshconnection.h>
#include <utils/ssh/sshconnectionmanager.h>

using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class LinuxDeviceTesterPrivate
{
public:
    LinuxDeviceTesterPrivate()
    { }

    QString headLine;
};

} // namespace Internal

using namespace Internal;

LinuxDeviceTester::LinuxDeviceTester(const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfiguration,
                                     const QString &headline, const QString &commandline) :
    SimpleRunner(deviceConfiguration, commandline),
    d(new LinuxDeviceTesterPrivate)
{
    d->headLine = headline;
}

LinuxDeviceTester::~LinuxDeviceTester()
{
    delete d;
}

QString LinuxDeviceTester::headLine() const
{
    return d->headLine;
}

int LinuxDeviceTester::processFinished(int exitStatus)
{
    return SimpleRunner::processFinished(exitStatus) == 0 ? TestSuccess : TestFailure;
}

AuthenticationTester::AuthenticationTester(const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfiguration) :
    LinuxDeviceTester(deviceConfiguration, tr("Checking authentication data..."),
                      QLatin1String("echo \"Success!\""))
{
    SshConnectionManager::instance().forceNewConnection(sshParameters());
}

void AuthenticationTester::handleStdOutput(const QByteArray &data)
{
    m_authenticationSucceded = data.contains("Success!");
    LinuxDeviceTester::handleStdOutput(data);
}

int AuthenticationTester::processFinished(int exitStatus)
{
    return LinuxDeviceTester::processFinished(exitStatus) == TestSuccess
            && m_authenticationSucceded ? TestSuccess : TestCriticalFailure;
}

UsedPortsTester::UsedPortsTester(const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfiguration) :
    LinuxDeviceTester(deviceConfiguration, tr("Checking for available ports..."), QString()),
    gatherer(new RemoteLinuxUsedPortsGatherer(deviceConfiguration))
{
    connect(gatherer, SIGNAL(aboutToStart()), this, SIGNAL(aboutToStart()));
    connect(gatherer, SIGNAL(errorMessage(QString)), this, SIGNAL(errorMessage(QString)));
    connect(gatherer, SIGNAL(finished(int)), this, SIGNAL(finished(int)));
    connect(gatherer, SIGNAL(progressMessage(QString)), this, SIGNAL(progressMessage(QString)));
    connect(gatherer, SIGNAL(started()), this, SIGNAL(started()));
}

UsedPortsTester::~UsedPortsTester()
{
    delete gatherer;
}

QString UsedPortsTester::commandLine() const
{
    return gatherer->commandLine();
}

void UsedPortsTester::run()
{
    gatherer->run();
}

void UsedPortsTester::cancel()
{
    gatherer->cancel();
}

} // namespace RemoteLinux
