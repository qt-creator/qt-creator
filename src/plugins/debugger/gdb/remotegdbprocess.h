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

#ifndef REMOTEGDBPROCESS_H
#define REMOTEGDBPROCESS_H

#include "abstractgdbprocess.h"

#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocess.h>

#include <QByteArray>
#include <QQueue>

namespace Debugger {
namespace Internal {

class GdbRemotePlainEngine;

class RemoteGdbProcess : public AbstractGdbProcess
{
    Q_OBJECT
public:
    RemoteGdbProcess(const QSsh::SshConnectionParameters &server,
                     GdbRemotePlainEngine *adapter, QObject *parent = 0);

    virtual QByteArray readAllStandardOutput();
    virtual QByteArray readAllStandardError();

    virtual void start(const QString &cmd, const QStringList &args);
    virtual bool waitForStarted();
    virtual qint64 write(const QByteArray &data);
    virtual void kill();
    virtual bool interrupt();

    virtual QProcess::ProcessState state() const;
    virtual QString errorString() const;

    virtual QProcessEnvironment processEnvironment() const;
    virtual void setProcessEnvironment(const QProcessEnvironment &env);
    virtual void setEnvironment(const QStringList &env);
    virtual void setWorkingDirectory(const QString &dir);

    void interruptInferior();
    void realStart(const QString &cmd, const QStringList &args,
        const QString &executableFilePath);

    static const QByteArray CtrlC;

signals:
    void started();
    void startFailed();

private slots:
    void handleConnected();
    void handleConnectionError();
    void handleFifoCreationFinished(int exitStatus);
    void handleAppOutputReaderStarted();
    void handleAppOutputReaderFinished(int exitStatus);
    void handleGdbStarted();
    void handleGdbFinished(int exitStatus);
    void handleGdbOutput();
    void handleAppOutput();
    void handleErrOutput();

private:
    enum State {
        Inactive, Connecting, CreatingFifo, StartingFifoReader,
        StartingGdb, RunningGdb
    };

    static QByteArray readerCmdLine(const QByteArray &file);

    int findAnchor(const QByteArray &data) const;
    void sendInput(const QByteArray &data);
    QByteArray removeCarriageReturn(const QByteArray &data);
    void emitErrorExit(const QString &error);
    void setState(State newState);

    QSsh::SshConnectionParameters m_connParams;
    QSsh::SshConnection *m_conn;
    QSsh::SshRemoteProcess::Ptr m_gdbProc;
    QSsh::SshRemoteProcess::Ptr m_appOutputReader;
    QSsh::SshRemoteProcess::Ptr m_fifoCreator;
    QByteArray m_gdbOutput;
    QByteArray m_errorOutput;
    QString m_command;
    QStringList m_cmdArgs;
    QString m_wd;
    QQueue<QByteArray> m_inputToSend;
    QByteArray m_currentGdbOutput;
    QByteArray m_lastSeqNr;
    QString m_error;
    QByteArray m_appOutputFileName;
    State m_state;

    GdbRemotePlainEngine *m_adapter;
};

} // namespace Internal
} // namespace Debugger

#endif // REMOTEGDBPROCESS_H
