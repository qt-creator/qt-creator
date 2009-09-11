/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DEBUGGER_SYMBIANADAPTER_H
#define DEBUGGER_SYMBIANADAPTER_H

#include "trkutils.h"
#include "trkclient.h"
#include "gdbprocessbase.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QPointer>
#include <QtCore/QProcess>
#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QMainWindow>
#include <QtGui/QKeyEvent>
#include <QtGui/QTextBlock>
#include <QtGui/QTextEdit>
#include <QtGui/QToolBar>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

namespace Debugger {
namespace Internal {

struct GdbResult
{
    QByteArray data;
};

///////////////////////////////////////////////////////////////////////
//
// SymbianAdapter
//
///////////////////////////////////////////////////////////////////////

class SymbianAdapter : public GdbProcessBase
{
    Q_OBJECT

public:
    typedef trk::TrkResult TrkResult;
    typedef Callback<const TrkResult &> TrkCallback;
    typedef Callback<const GdbResult &> GdbCallback;

    SymbianAdapter();
    ~SymbianAdapter();
    void setGdbServerName(const QString &name);
    QString gdbServerName() const { return m_gdbServerName; }
    QString gdbServerIP() const;
    uint gdbServerPort() const;
    void setVerbose(int verbose) { m_verbose = verbose; }
    void setSerialFrame(bool b) { m_serialFrame = b; }
    void setBufferedMemoryRead(bool b) { m_bufferedMemoryRead = b; }
    trk::Session &session() { return m_session; }

public slots:
    void startInferior();
    void run();

signals:
    void output(const QString &msg);
    void startSuccessful();
    void startFailed();

private slots:
    void startGdb();

private:
    friend class RunnerGui;

    QString m_rfcommDevice;  // /dev/rfcomm0
    QString m_gdbServerName; // 127.0.0.1:(2222+uid)

    QProcess m_gdbProc;
    QProcess m_rfcommProc;
    bool m_running;

public:
    //
    // Implementation of GdbProcessBase
    //
    void start(const QString &program, const QStringList &args,
        QIODevice::OpenMode mode = QIODevice::ReadWrite);
    void kill();
    void terminate();
    bool waitForFinished(int msecs = 30000);
    QProcess::ProcessState state() const;
    QString errorString() const;
    QByteArray readAllStandardError();
    QByteArray readAllStandardOutput();
    qint64 write(const char *data);
    void setWorkingDirectory(const QString &dir);
    void setEnvironment(const QStringList &env);
    bool isAdapter() const { return true; }
    void attach(GdbEngine *engine) const;

    //
    // TRK
    //
    void sendTrkMessage(byte code,
        TrkCallback callback = TrkCallback(),
        const QByteArray &data = QByteArray(),
        const QVariant &cookie = QVariant());
    Q_SLOT void handleTrkResult(const trk::TrkResult &data);
    Q_SLOT void handleTrkError(const QString &msg);

    // convenience messages
    void sendTrkAck(byte token);

    void handleCpuType(const TrkResult &result);
    void handleCreateProcess(const TrkResult &result);
    void handleClearBreakpoint(const TrkResult &result);
    void handleSignalContinue(const TrkResult &result);
    void handleStop(const TrkResult &result);
    void handleSupportMask(const TrkResult &result);
    void handleTrkVersions(const TrkResult &result);
    void handleDisconnect(const TrkResult &result);

    void handleAndReportCreateProcess(const TrkResult &result);
    void handleAndReportReadRegisters(const TrkResult &result);
    QByteArray memoryReadLogMessage(uint addr, uint len, const QByteArray &ba) const;
    QByteArray trkContinueMessage();
    QByteArray trkReadRegisterMessage();
    QByteArray trkReadMemoryMessage(uint addr, uint len);
    QByteArray trkBreakpointMessage(uint addr, uint len, bool armMode = true);
    void handleAndReportSetBreakpoint(const TrkResult &result);
    void handleReadMemoryBuffered(const TrkResult &result);
    void handleReadMemoryUnbuffered(const TrkResult &result);
    void handleStepRange(const TrkResult &result);
    void handleReadRegisters(const TrkResult &result);
    void reportReadMemoryBuffered(const TrkResult &result);
    void reportToGdb(const TrkResult &result);

    void readMemory(uint addr, uint len);
    void interruptInferior();

    trk::TrkDevice m_trkDevice;

    //
    // Gdb
    //
    struct GdbCommand
    {
        GdbCommand() : flags(0), callback(GdbCallback()), callbackName(0) {}

        int flags;
        GdbCallback callback;
        const char *callbackName;
        QString command;
        QVariant cookie;
        //QTime postTime;
    };

    void sendGdbMessage(const QString &msg,
        GdbCallback callback = GdbCallback(),
        const QVariant &cookie = QVariant());
    Q_SLOT void handleGdbConnection();
    Q_SLOT void readGdbServerCommand();
    void readGdbResponse();
    void handleGdbServerCommand(const QByteArray &cmd);
    void sendGdbServerMessage(const QByteArray &msg,
        const QByteArray &logNote = QByteArray());
    void sendGdbServerMessageAfterTrkResponse(const QByteArray &msg,
        const QByteArray &logNote = QByteArray());
    void sendGdbServerAck();
    bool sendGdbServerPacket(const QByteArray &packet, bool doFlush);

    Q_SLOT void handleGdbError(QProcess::ProcessError error);
    Q_SLOT void handleGdbFinished(int exitCode, QProcess::ExitStatus exitStatus);
    Q_SLOT void handleGdbStarted();
    Q_SLOT void handleGdbStateChanged(QProcess::ProcessState newState);

    void logMessage(const QString &msg);  // triggers output() if m_verbose
    Q_SLOT void trkLogMessage(const QString &msg);

    QTcpServer m_gdbServer;
    QPointer<QTcpSocket> m_gdbConnection;
    QByteArray m_gdbReadBuffer;
    bool m_gdbAckMode;

    QHash<int, GdbCommand> m_gdbCookieForToken;

    //
    // Rfcomm
    //
    Q_SLOT void handleRfcommReadyReadStandardError();
    Q_SLOT void handleRfcommReadyReadStandardOutput();
    Q_SLOT void handleRfcommError(QProcess::ProcessError error);
    Q_SLOT void handleRfcommFinished(int exitCode, QProcess::ExitStatus exitStatus);
    Q_SLOT void handleRfcommStarted();
    Q_SLOT void handleRfcommStateChanged(QProcess::ProcessState newState);

    // Debuggee state
    Q_SLOT void executeCommand(const QString &msg);
    trk::Session m_session; // global-ish data (process id, target information)
    trk::Snapshot m_snapshot; // local-ish data (memory and registers)
    int m_verbose;
    bool m_serialFrame;
    bool m_bufferedMemoryRead;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_SYMBIANADAPTER_H
