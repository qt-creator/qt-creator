/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef REMOTEGDBPROCESS_H
#define REMOTEGDBPROCESS_H

#include "abstractgdbprocess.h"

#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>

#include <QtCore/QByteArray>
#include <QtCore/QQueue>

namespace Debugger {
namespace Internal {

class RemotePlainGdbAdapter;

class RemoteGdbProcess : public AbstractGdbProcess
{
    Q_OBJECT
public:
    RemoteGdbProcess(const Core::SshConnectionParameters &server,
                     RemotePlainGdbAdapter *adapter, QObject *parent = 0);

    virtual QByteArray readAllStandardOutput();
    virtual QByteArray readAllStandardError();

    virtual void start(const QString &cmd, const QStringList &args);
    virtual bool waitForStarted();
    virtual qint64 write(const QByteArray &data);
    virtual void kill();

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
    void handleGdbOutput(const QByteArray &output);
    void handleAppOutput(const QByteArray &output);
    void handleErrOutput(const QByteArray &output);

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

    Core::SshConnectionParameters m_connParams;
    Core::SshConnection::Ptr m_conn;
    Core::SshRemoteProcess::Ptr m_gdbProc;
    Core::SshRemoteProcess::Ptr m_appOutputReader;
    Core::SshRemoteProcess::Ptr m_fifoCreator;
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

    RemotePlainGdbAdapter *m_adapter;
};

} // namespace Internal
} // namespace Debugger

#endif // REMOTEGDBPROCESS_H
