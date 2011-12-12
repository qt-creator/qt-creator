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
#ifndef LINUXDEVICETESTER_H
#define LINUXDEVICETESTER_H

#include "remotelinux_export.h"

#include "simplerunner.h"

#include <utils/ssh/sshconnection.h>

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

QT_FORWARD_DECLARE_CLASS(QString)

namespace RemoteLinux {
class LinuxDeviceConfiguration;
class RemoteLinuxUsedPortsGatherer;

namespace Internal {
class LinuxDeviceTesterPrivate;
} // namespace Internal

// -----------------------------------------------------------------------
// LinuxDeviceTester:
// -----------------------------------------------------------------------

class REMOTELINUX_EXPORT LinuxDeviceTester : public SimpleRunner
{
    Q_OBJECT

public:
    enum TestResult { TestSuccess = 0, TestFailure, TestCriticalFailure };

    LinuxDeviceTester(const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfiguration,
                      const QString &headline, const QString &commandline);
    ~LinuxDeviceTester();

    virtual QString headLine() const;

protected:
    int processFinished(int exitStatus);

private:
    Internal::LinuxDeviceTesterPrivate *const d;
};

class REMOTELINUX_EXPORT AuthenticationTester : public LinuxDeviceTester
{
    Q_OBJECT

public:
    AuthenticationTester(const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfiguration);

protected slots:
    void handleStdOutput(const QByteArray &data);

protected:
    int processFinished(int exitStatus);

private:
    bool m_authenticationSucceded;
};

class REMOTELINUX_EXPORT UsedPortsTester : public LinuxDeviceTester
{
    Q_OBJECT

public:
    UsedPortsTester(const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfiguration);
    ~UsedPortsTester();

    QString commandLine() const;

    void run();
    void cancel();

private:
    RemoteLinuxUsedPortsGatherer *const gatherer;
};

} // namespace RemoteLinux

#endif // LINUXDEVICETESTER_H
