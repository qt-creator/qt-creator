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

#ifndef REMOTEPROCESSTEST_H
#define REMOTEPROCESSTEST_H

#include <ssh/sshremoteprocessrunner.h>

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QTextStream)
QT_FORWARD_DECLARE_CLASS(QTimer)

class RemoteProcessTest : public QObject
{
    Q_OBJECT
public:
    RemoteProcessTest(const QSsh::SshConnectionParameters &params);
    ~RemoteProcessTest();
    void run();

private slots:
    void handleConnectionError();
    void handleProcessStarted();
    void handleProcessStdout();
    void handleProcessStderr();
    void handleProcessClosed(int exitStatus);
    void handleTimeout();
    void handleReadyRead();
    void handleReadyReadStdout();
    void handleReadyReadStderr();
    void handleConnected();

private:
    enum State {
        Inactive, TestingSuccess, TestingFailure, TestingCrash, TestingTerminal, TestingIoDevice,
        TestingProcessChannels
    };

    QString testString() const;
    void handleSuccessfulCrashTest();
    void handleSuccessfulIoTest();

    const QSsh::SshConnectionParameters m_sshParams;
    QTimer * const m_timeoutTimer;
    QTextStream *m_textStream;
    QSsh::SshRemoteProcess::Ptr m_catProcess;
    QSsh::SshRemoteProcess::Ptr m_echoProcess;
    QSsh::SshConnection *m_sshConnection;
    QSsh::SshRemoteProcessRunner * const m_remoteRunner;
    QByteArray m_remoteStdout;
    QByteArray m_remoteStderr;
    QByteArray m_remoteData;
    State m_state;
    bool m_started;
};


#endif // REMOTEPROCESSTEST_H
