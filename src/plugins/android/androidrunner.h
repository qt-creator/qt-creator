/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt.io/licensing
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

#ifndef ANDROIDRUNNER_H
#define ANDROIDRUNNER_H

#include "androidconfigurations.h"

#include <projectexplorer/runconfiguration.h>

#include <QObject>
#include <QTimer>
#include <QTcpSocket>
#include <QThread>
#include <QProcess>
#include <QMutex>

namespace Android {
class AndroidRunConfiguration;

namespace Internal {

class AndroidRunner : public QThread
{
    Q_OBJECT

    enum DebugHandShakeType {
        PingPongFiles,
        SocketHandShake
    };

public:
    AndroidRunner(QObject *parent, AndroidRunConfiguration *runConfig,
                  Core::Id runMode);
    ~AndroidRunner();

    QString displayName() const;

public slots:
    void start();
    void stop();
    void handleRemoteDebuggerRunning();

signals:
    void remoteServerRunning(const QByteArray &serverChannel, int pid);
    void remoteProcessStarted(int gdbServerPort, int qmlPort);
    void remoteProcessFinished(const QString &errString = QString());

    void remoteOutput(const QString &output);
    void remoteErrorOutput(const QString &output);

private slots:
    void checkPID();
    void logcatReadStandardError();
    void logcatReadStandardOutput();
    void asyncStart();

private:
    void adbKill(qint64 pid);
    QStringList selector() const { return m_selector; }
    void forceStop();
    QByteArray runPs();
    void findPs();
    void logcatProcess(const QByteArray &text, QByteArray &buffer, bool onlyError);
    bool adbShellAmNeedsQuotes();

private:
    QProcess m_adbLogcatProcess;
    QTimer m_checkPIDTimer;
    bool m_wasStarted;
    int m_tries;

    QByteArray m_stdoutBuffer;
    QByteArray m_stderrBuffer;
    QString m_intentName;
    QString m_packageName;
    QString m_deviceSerialNumber;
    qint64 m_processPID;
    bool m_useCppDebugger;
    bool m_useQmlDebugger;
    bool m_useQmlProfiler;
    ushort m_localGdbServerPort; // Local end of forwarded debug socket.
    quint16 m_qmlPort;
    bool m_useLocalQtLibs;
    QString m_pingFile;
    QString m_pongFile;
    QString m_gdbserverPath;
    QString m_gdbserverSocket;
    QString m_localLibs;
    QString m_localJars;
    QString m_localJarsInitClasses;
    QString m_adb;
    bool m_isBusyBox;
    QStringList m_selector;
    QMutex m_mutex;
    QRegExp m_logCatRegExp;
    DebugHandShakeType m_handShakeMethod;
    QTcpSocket *m_socket;
    bool m_customPort;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDRUNNER_H
