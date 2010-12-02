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

#ifndef DEBUGGER_TCFTRKGDBADAPTER_H
#define DEBUGGER_TCFTRKGDBADAPTER_H

#include "abstractgdbadapter.h"
#include "localgdbprocess.h"
#include "callback.h"
#include "trkutils.h"
#include "symbian.h"

#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringList>
#include <QtCore/QHash>

QT_BEGIN_NAMESPACE
class QTcpServer;
class QTcpSocket;
class QIODevice;
QT_END_NAMESPACE

namespace tcftrk {
    struct TcfTrkCommandResult;
    class TcfTrkDevice;
    class TcfTrkEvent;
    class TcfTrkRunControlModuleLoadContextSuspendedEvent;
}

namespace Debugger {
namespace Internal {

struct MemoryRange;
struct GdbResult;

///////////////////////////////////////////////////////////////////////
//
// TcfTrkGdbAdapter
//
///////////////////////////////////////////////////////////////////////

class TcfTrkGdbAdapter : public AbstractGdbAdapter
{
    Q_OBJECT

public:
    typedef trk::Callback<const GdbResult &> GdbResultCallback;
    typedef trk::Callback<const tcftrk::TcfTrkCommandResult &> TcfTrkCallback;
    typedef trk::Callback<const GdbResponse &> GdbCallback;

    explicit TcfTrkGdbAdapter(GdbEngine *engine);
    virtual ~TcfTrkGdbAdapter();
    void setGdbServerName(const QString &name);
    QString gdbServerName() const { return m_gdbServerName; }

    Q_SLOT void setVerbose(const QVariant &value);
    void setVerbose(int verbose);
    void setBufferedMemoryRead(bool b) { m_bufferedMemoryRead = b; }

    void trkReloadRegisters();
    void trkReloadThreads();

signals:
    void output(const QString &msg);

public:
    //
    // Implementation of GdbProcessBase
    //
    void start(const QString &program, const QStringList &args,
        QIODevice::OpenMode mode = QIODevice::ReadWrite);
    void write(const QByteArray &data);
    bool isTrkAdapter() const { return true; }

    virtual DumperHandling dumperHandling() const { return DumperNotAvailable; }

private:
    void startAdapter();
    void setupInferior();
    void runEngine();
    void interruptInferior();
    void shutdownInferior();
    void shutdownAdapter();
    void sendRunControlTerminateCommand();
    void handleRunControlTerminate(const tcftrk::TcfTrkCommandResult &);
    void sendRegistersGetMCommand();
    void handleWriteRegister(const tcftrk::TcfTrkCommandResult &result);
    void reportRegisters();
    void handleReadRegisters(const tcftrk::TcfTrkCommandResult &result);
    void handleRegisterChildren(const tcftrk::TcfTrkCommandResult &result);
    void handleAndReportReadRegisters(const tcftrk::TcfTrkCommandResult &result);
    void handleAndReportReadRegister(const tcftrk::TcfTrkCommandResult &result);
    void handleAndReportReadRegistersAfterStop(const tcftrk::TcfTrkCommandResult &result);
    QByteArray stopMessage() const;
    void handleAndReportSetBreakpoint(const tcftrk::TcfTrkCommandResult &result);
    void handleClearBreakpoint(const tcftrk::TcfTrkCommandResult &result);
    void readMemory(uint addr, uint len, bool buffered);
    void handleReadMemoryBuffered(const tcftrk::TcfTrkCommandResult &result);
    void handleReadMemoryUnbuffered(const tcftrk::TcfTrkCommandResult &result);
    void handleWriteMemory(const tcftrk::TcfTrkCommandResult &result);
    void tryAnswerGdbMemoryRequest(bool buffered);
    inline void sendMemoryGetCommand(const MemoryRange &range, bool buffered);
    void addThread(unsigned id);
    inline QByteArray mainThreadContextId() const;
    inline QByteArray currentThreadContextId() const;

    AbstractGdbProcess *gdbProc() { return &m_gdbProc; }

    void cleanup();

    void handleTargetRemote(const GdbResponse &response);

    QString m_gdbServerName; // 127.0.0.1:(2222+uid)
    bool m_running;
    int m_stopReason;
    tcftrk::TcfTrkDevice *m_trkDevice;
    QSharedPointer<QIODevice> m_trkIODevice;

    //
    // Gdb
    //
    Q_SLOT void handleGdbConnection();
    Q_SLOT void readGdbServerCommand();
    Q_SLOT void tcftrkDeviceError(const QString  &);
    void startGdb();
    Q_SLOT void tcftrkEvent(const tcftrk::TcfTrkEvent &knownEvent);
    void handleTcfTrkRunControlModuleLoadContextSuspendedEvent(const tcftrk::TcfTrkRunControlModuleLoadContextSuspendedEvent &e);
    inline void sendTrkContinue();
    void sendTrkStepRange();
    void handleStep(const tcftrk::TcfTrkCommandResult &result);
    void handleCreateProcess(const tcftrk::TcfTrkCommandResult &result);

    void readGdbResponse();
    void handleGdbServerCommand(const QByteArray &cmd);
    void sendGdbServerMessage(const QByteArray &msg,
        const QByteArray &logNote = QByteArray());
    void sendGdbServerAck();
    bool sendGdbServerPacket(const QByteArray &packet, bool doFlush);
    void gdbSetCurrentThread(const QByteArray &cmd, const char *why);

    void logMessage(const QString &msg, int channel = LogDebug);  // triggers output() if m_verbose
    Q_SLOT void trkLogMessage(const QString &msg);

    QPointer<QTcpServer> m_gdbServer;
    QPointer<QTcpSocket> m_gdbConnection;
    QByteArray m_gdbReadBuffer;
    bool m_gdbAckMode;

    // Debuggee state
    trk::Session m_session; // global-ish data (process id, target information)
    Symbian::Snapshot m_snapshot; // local-ish data (memory and registers)
    QString m_remoteExecutable;
    unsigned m_uid;
    QStringList m_remoteArguments;
    QString m_symbolFile;
    QString m_symbolFileFolder;
    int m_verbose;
    bool m_bufferedMemoryRead;
    bool m_firstResumableExeLoadedEvent;
    // gdb wants registers, but we don't have the names yet. Continue in handler for names
    bool m_registerRequestPending;
    QByteArray m_tcfProcessId;
    LocalGdbProcess m_gdbProc;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_TCFTRKGDBADAPTER_H
