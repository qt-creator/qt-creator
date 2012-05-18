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
#ifndef MADDEDEVICETESTER_H
#define MADDEDEVICETESTER_H

#include <remotelinux/linuxdevicetester.h>

#include <QByteArray>

namespace QSsh {
class SshRemoteProcessRunner;
}

namespace Madde {
namespace Internal {

class MaddeDeviceTester : public RemoteLinux::AbstractLinuxDeviceTester
{
    Q_OBJECT
public:
    explicit MaddeDeviceTester(QObject *parent = 0);
    ~MaddeDeviceTester();

    void testDevice(const QSharedPointer<const RemoteLinux::LinuxDeviceConfiguration> &deviceConfiguration);
    void stopTest();

private slots:
    void handleGenericTestFinished(RemoteLinux::AbstractLinuxDeviceTester::TestResult result);
    void handleConnectionError();
    void handleStdout(const QByteArray &data);
    void handleStderr(const QByteArray &data);
    void handleProcessFinished(int exitStatus);

private:
    enum State { Inactive, GenericTest, QtTest, MadDeveloperTest, QmlToolingTest };

    void handleQtTestFinished(int exitStatus);
    void handleMadDeveloperTestFinished(int exitStatus);
    void handleQmlToolingTestFinished(int exitStatus);

    QString processedQtLibsList();
    void setFinished();

    RemoteLinux::GenericLinuxDeviceTester * const m_genericTester;
    State m_state;
    TestResult m_result;
    QSsh::SshRemoteProcessRunner *m_processRunner;
    QSharedPointer<const RemoteLinux::LinuxDeviceConfiguration> m_deviceConfiguration;
    QByteArray m_stdout;
    QByteArray m_stderr;
};

} // namespace Internal
} // namespace Madde

#endif // MADDEDEVICETESTER_H
