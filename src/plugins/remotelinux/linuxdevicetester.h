/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QObject>
#include <QSharedPointer>

QT_FORWARD_DECLARE_CLASS(QString)

namespace QSsh {
class SshConnection;
}

namespace RemoteLinux {
class LinuxDeviceConfiguration;

namespace Internal {
class GenericLinuxDeviceTesterPrivate;
} // namespace Internal

class REMOTELINUX_EXPORT AbstractLinuxDeviceTester : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractLinuxDeviceTester)
public:
    enum TestResult { TestSuccess, TestFailure };

    virtual void testDevice(const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfiguration) = 0;
    virtual void stopTest() = 0;

signals:
    void progressMessage(const QString &message);
    void errorMessage(const QString &message);
    void finished(RemoteLinux::AbstractLinuxDeviceTester::TestResult result);

protected:
    explicit AbstractLinuxDeviceTester(QObject *parent = 0);
};


class REMOTELINUX_EXPORT GenericLinuxDeviceTester : public AbstractLinuxDeviceTester
{
    Q_OBJECT
public:
    explicit GenericLinuxDeviceTester(QObject *parent = 0);
    ~GenericLinuxDeviceTester();

    void testDevice(const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfiguration);
    void stopTest();

private slots:
    void handleConnected();
    void handleConnectionFailure();
    void handleProcessFinished(int exitStatus);
    void handlePortsGatheringError(const QString &message);
    void handlePortListReady();

private:
    void setFinished(TestResult result);

    Internal::GenericLinuxDeviceTesterPrivate * const d;
};

} // namespace RemoteLinux

#endif // LINUXDEVICETESTER_H
