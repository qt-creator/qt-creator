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

#include <ssh/sftpchannel.h>
#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocess.h>

#include <QCoreApplication>
#include <QList>
#include <QObject>
#include <QPair>
#include <QTimer>

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
        noHost.authenticationType = SshConnectionParameters::AuthenticationByPassword;

        SshConnectionParameters noUser;
        noUser.host = QLatin1String("localhost");
        noUser.port = 22;
        noUser.timeout = 30;
        noUser.authenticationType = SshConnectionParameters::AuthenticationByPassword;
        noUser.userName = QLatin1String("dumdidumpuffpuff");
        noUser.password = QLatin1String("whatever");

        SshConnectionParameters wrongPwd;
        wrongPwd.host = QLatin1String("localhost");
        wrongPwd.port = 22;
        wrongPwd.timeout = 30;
        wrongPwd.authenticationType = SshConnectionParameters::AuthenticationByPassword;
        wrongPwd.userName = QLatin1String("root");
        noUser.password = QLatin1String("thiscantpossiblybeapasswordcanit");

        SshConnectionParameters invalidKeyFile;
        invalidKeyFile.host = QLatin1String("localhost");
        invalidKeyFile.port = 22;
        invalidKeyFile.timeout = 30;
        invalidKeyFile.authenticationType = SshConnectionParameters::AuthenticationByKey;
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

private slots:
    void handleConnected()
    {
        qDebug("Error: Received unexpected connected() signal.");
        qApp->quit();
    }

    void handleDisconnected()
    {
        qDebug("Error: Received unexpected disconnected() signal.");
        qApp->quit();
    }

    void handleDataAvailable(const QString &msg)
    {
        qDebug("Error: Received unexpected dataAvailable() signal. "
            "Message was: '%s'.", qPrintable(msg));
        qApp->quit();
    }

    void handleError(QSsh::SshError error)
    {
        if (m_testSet.isEmpty()) {
            qDebug("Error: Received error %d, but no test was running.", error);
            qApp->quit();
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
            qApp->quit();
        }
    }

    void handleTimeout()
    {
        if (m_testSet.isEmpty()) {
            qDebug("Error: timeout, but no test was running.");
            qApp->quit();
        }
        const TestItem testItem = m_testSet.takeFirst();
        qDebug("Error: The following test timed out: %s", testItem.description);
    }

private:
    void runNextTest()
    {
        if (m_connection) {
            disconnect(m_connection, 0, this, 0);
            delete m_connection;
            }
        m_connection = new SshConnection(m_testSet.first().params);
        connect(m_connection, SIGNAL(connected()), SLOT(handleConnected()));
        connect(m_connection, SIGNAL(disconnected()), SLOT(handleDisconnected()));
        connect(m_connection, SIGNAL(dataAvailable(QString)), SLOT(handleDataAvailable(QString)));
        connect(m_connection, SIGNAL(error(QSsh::SshError)), SLOT(handleError(QSsh::SshError)));
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
