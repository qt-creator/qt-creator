/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include <ssh/sftpchannel.h>
#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocess.h>

#include <QCoreApplication>
#include <QList>
#include <QObject>
#include <QPair>
#include <QTimer>

#include <cstdlib>

using namespace QSsh;

class Test : public QObject {
    Q_OBJECT
public:
    Test()
    {
        m_timeoutTimer.setSingleShot(true);
        m_connection = new SshConnection(SshConnectionParameters());
        if (m_connection->state() != SshConnection::Unconnected) {
            qDebug("Error: Newly created SSH connection has state %d.",
                m_connection->state());
        }

        if (m_connection->createRemoteProcess(""))
            qDebug("Error: Unconnected SSH connection creates remote process.");
        if (m_connection->createSftpChannel())
            qDebug("Error: Unconnected SSH connection creates SFTP channel.");

        SshConnectionParameters noHost;
        noHost.host = QLatin1String("hgdfxgfhgxfhxgfchxgcf");
        noHost.port = 12345;
        noHost.timeout = 10;
        noHost.authenticationType
                = SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods;

        SshConnectionParameters noUser;
        noUser.host = QLatin1String("localhost");
        noUser.port = 22;
        noUser.timeout = 30;
        noUser.authenticationType
                = SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods;
        noUser.userName = QLatin1String("dumdidumpuffpuff");
        noUser.password = QLatin1String("whatever");

        SshConnectionParameters wrongPwd;
        wrongPwd.host = QLatin1String("localhost");
        wrongPwd.port = 22;
        wrongPwd.timeout = 30;
        wrongPwd.authenticationType
                = SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods;
        wrongPwd.userName = QLatin1String("root");
        noUser.password = QLatin1String("thiscantpossiblybeapasswordcanit");

        SshConnectionParameters invalidKeyFile;
        invalidKeyFile.host = QLatin1String("localhost");
        invalidKeyFile.port = 22;
        invalidKeyFile.timeout = 30;
        invalidKeyFile.authenticationType = SshConnectionParameters::AuthenticationTypePublicKey;
        invalidKeyFile.userName = QLatin1String("root");
        invalidKeyFile.privateKeyFile
            = QLatin1String("somefilenamethatwedontexpecttocontainavalidkey");

        // TODO: Create a valid key file and check for authentication error.

        m_testSet << TestItem("Behavior with non-existing host",
            noHost, ErrorList() << SshSocketError);
        m_testSet << TestItem("Behavior with non-existing user", noUser,
            ErrorList() << SshSocketError << SshTimeoutError
                << SshAuthenticationError);
        m_testSet << TestItem("Behavior with wrong password", wrongPwd,
            ErrorList() << SshSocketError << SshTimeoutError
                << SshAuthenticationError);
        m_testSet << TestItem("Behavior with invalid key file", invalidKeyFile,
            ErrorList() << SshSocketError << SshTimeoutError
                << SshKeyFileError);

        runNextTest();
    }

    ~Test()
    {
        delete m_connection;
    }

private:
    void handleConnected()
    {
        qDebug("Error: Received unexpected connected() signal.");
        qApp->exit(EXIT_FAILURE);
    }

    void handleDisconnected()
    {
        qDebug("Error: Received unexpected disconnected() signal.");
        qApp->exit(EXIT_FAILURE);
    }

    void handleDataAvailable(const QString &msg)
    {
        qDebug("Error: Received unexpected dataAvailable() signal. "
            "Message was: '%s'.", qPrintable(msg));
        qApp->exit(EXIT_FAILURE);
    }

    void handleError(QSsh::SshError error)
    {
        if (m_testSet.isEmpty()) {
            qDebug("Error: Received error %d, but no test was running.", error);
            qApp->exit(EXIT_FAILURE);
        }

        const TestItem testItem = m_testSet.takeFirst();
        if (testItem.allowedErrors.contains(error)) {
            qDebug("Received error %d, as expected.", error);
            if (m_testSet.isEmpty()) {
                qDebug("All tests finished successfully.");
                qApp->quit();
            } else {
                runNextTest();
            }
        } else {
            qDebug("Received unexpected error %d.", error);
            qApp->exit(EXIT_FAILURE);
        }
    }

    void handleTimeout()
    {
        if (m_testSet.isEmpty()) {
            qDebug("Error: timeout, but no test was running.");
            qApp->exit(EXIT_FAILURE);
        }
        const TestItem testItem = m_testSet.takeFirst();
        qDebug("Error: The following test timed out: %s", testItem.description);
    }

    void runNextTest()
    {
        if (m_connection) {
            disconnect(m_connection, 0, this, 0);
            delete m_connection;
            }
        m_connection = new SshConnection(m_testSet.first().params);
        connect(m_connection, &SshConnection::connected, this, &Test::handleConnected);
        connect(m_connection, &SshConnection::disconnected, this, &Test::handleDisconnected);
        connect(m_connection, &SshConnection::dataAvailable, this, &Test::handleDataAvailable);
        connect(m_connection, &SshConnection::error, this, &Test::handleError);
        const TestItem &nextItem = m_testSet.first();
        m_timeoutTimer.stop();
        m_timeoutTimer.setInterval(qMax(10000, nextItem.params.timeout * 1000));
        qDebug("Testing: %s", nextItem.description);
        m_connection->connectToHost();
    }

    SshConnection *m_connection;
    typedef QList<SshError> ErrorList;
    struct TestItem {
        TestItem(const char *d, const SshConnectionParameters &p,
            const ErrorList &e) : description(d), params(p), allowedErrors(e) {}

        const char *description;
        SshConnectionParameters params;
        ErrorList allowedErrors;
    };
    QList<TestItem> m_testSet;
    QTimer m_timeoutTimer;
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Test t;

    return a.exec();
}


#include "main.moc"
