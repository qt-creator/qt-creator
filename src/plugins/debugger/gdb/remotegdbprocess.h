/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef REMOTEGDBPROCESS_H
#define REMOTEGDBPROCESS_H

#include "abstractgdbprocess.h"

#include <coreplugin/ssh/sshconnection.h>

#include <QtCore/QByteArray>
#include <QtCore/QQueue>

namespace Debugger {
namespace Internal {

class RemotePlainGdbAdapter;

class RemoteGdbProcess : public AbstractGdbProcess
{
    Q_OBJECT
public:
    RemoteGdbProcess(const Core::SshServerInfo &server,
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

    static const QByteArray CtrlC;

private slots:
    void handleGdbOutput();
    void handleAppOutput();
    void handleErrOutput();

private:
    enum CmdState { CmdNotYetSent, CmdSent, CmdReceived };

    static QByteArray mkFifoCmdLine(const QByteArray &file);
    static QByteArray readerCmdLine(const QByteArray &file);

    int findAnchor(const QByteArray &data) const;
    bool handleAppOrErrOutput(Core::InteractiveSshConnection::Ptr &conn,
             CmdState &cmdState, QByteArray &initialOutput,
             const QByteArray &file, const QByteArray &output);
    qint64 sendInput(const QByteArray &data);
    void startGdb();
    void stopReaders();
    QByteArray removeCarriageReturn(const QByteArray &data);
    void checkForGdbExit(QByteArray &output);

    static const QByteArray AppOutputFile;
    static const QByteArray ErrOutputFile;

    Core::SshServerInfo m_serverInfo;
    Core::InteractiveSshConnection::Ptr m_gdbConn;
    Core::InteractiveSshConnection::Ptr m_appOutputConn;
    Core::InteractiveSshConnection::Ptr m_errOutputConn;
    QByteArray m_gdbOutput;
    QByteArray m_errorOutput;
    QString m_command;
    QStringList m_cmdArgs;
    QString m_wd;
    QQueue<QByteArray> m_inputToSend;
    QByteArray m_currentGdbOutput;
    QByteArray m_initialAppOutput;
    QByteArray m_initialErrOutput;
    QByteArray m_lastSeqNr;
    QByteArray m_startCmdLine;

    CmdState m_gdbState;
    CmdState m_appOutputReaderState;
    CmdState m_errOutputReaderState;

    RemotePlainGdbAdapter *m_adapter;
};

} // namespace Internal
} // namespace Debugger

#endif // REMOTEGDBPROCESS_H
