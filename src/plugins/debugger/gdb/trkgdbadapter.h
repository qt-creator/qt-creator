/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef DEBUGGER_TRKGDBADAPTER_H
#define DEBUGGER_TRKGDBADAPTER_H

#include "abstractgdbadapter.h"
#include "localgdbprocess.h"
#include "trkutils.h"
#include "callback.h"
#include "symbian.h"

#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringList>
#include <QtCore/QHash>

QT_BEGIN_NAMESPACE
class QTcpServer;
class QTcpSocket;
QT_END_NAMESPACE

namespace trk {
struct TrkResult;
struct TrkMessage;
class TrkDevice;
}

namespace SymbianUtils {
class SymbianDevice;
}

namespace Debugger {
namespace Internal {

struct MemoryRange;
struct GdbResult;

///////////////////////////////////////////////////////////////////////
//
// TrkGdbAdapter
//
///////////////////////////////////////////////////////////////////////

class TrkGdbAdapter : public AbstractGdbAdapter
{
    Q_OBJECT

public:
    typedef trk::TrkResult TrkResult;
    typedef trk::Callback<const TrkResult &> TrkCallback;
    typedef trk::Callback<const GdbResult &> GdbCallback;

    TrkGdbAdapter(GdbEngine *engine);
    ~TrkGdbAdapter();
    void setGdbServerName(const QString &name);
    QString gdbServerName() const { return m_gdbServerName; }
    QString gdbServerIP() const;
    uint gdbServerPort() const;
    Q_SLOT void setVerbose(const QVariant &value);
    void setVerbose(int verbose);
    void setBufferedMemoryRead(bool b) { m_bufferedMemoryRead = b; }
    trk::Session &session() { return m_session; }
    void trkReloadRegisters();
    void trkReloadThreads();

signals:
    void output(const QString &msg);

private:
    QString m_gdbServerName; // 127.0.0.1:(2222+uid)

    bool m_running;

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
    bool initializeDevice(const QString &remoteChannel, QString *errorMessage);
    void setupInferior();
    void runEngine();
    void interruptInferior();
    void shutdownInferior();
    void shutdownAdapter();
    AbstractGdbProcess *gdbProc() { return &m_gdbProc; }

    void cleanup();
    void emitDelayedInferiorSetupFailed(const QString &msg);
    Q_SLOT void slotEmitDelayedInferiorSetupFailed();

    void handleTargetRemote(const GdbResponse &response);

    //
    // TRK
    //
    void sendTrkMessage(trk::byte code,
        TrkCallback callback = TrkCallback(),
        const QByteArray &data = QByteArray(),
        const QVariant &cookie = QVariant());
    Q_SLOT void handleTrkResult(const trk::TrkResult &data);
    Q_SLOT void handleTrkError(const QString &msg);
    void trkContinueAll(const char *why);
    void trkKill();
    void handleTrkContinueNext(const TrkResult &result);
    void trkContinueNext(int threadIndex);

    // convenience messages
    void sendTrkAck(trk::byte token);

    void handleCpuType(const TrkResult &result);
    void handleCreateProcess(const TrkResult &result);
    void handleClearBreakpoint(const TrkResult &result);
    void handleStop(const TrkResult &result);
    void handleSupportMask(const TrkResult &result);
    void handleTrkVersionsStartGdb(const TrkResult &result);
    Q_SLOT void slotStartGdb();
    void handleDisconnect(const TrkResult &result);
    void handleDeleteProcess(const TrkResult &result);
    void handleDeleteProcess2(const TrkResult &result);
    void handleAndReportCreateProcess(const TrkResult &result);
    void handleAndReportReadRegisters(const TrkResult &result);
    void handleAndReportReadRegister(const TrkResult &result);
    void handleAndReportReadRegistersAfterStop(const TrkResult &result);
    void reportRegisters();
    void handleAndReportSetBreakpoint(const TrkResult &result);
    void handleReadMemoryBuffered(const TrkResult &result);
    void handleReadMemoryUnbuffered(const TrkResult &result);
    void handleStep(const TrkResult &result);
    void handleReadRegisters(const TrkResult &result);
    void handleWriteRegister(const TrkResult &result);
    void handleWriteMemory(const TrkResult &result);
    void reportToGdb(const TrkResult &result);
    void gdbSetCurrentThread(const QByteArray &cmd, const char *why);
    //void reportReadMemoryBuffered(const TrkResult &result);
    //void reportReadMemoryUnbuffered(const TrkResult &result);

    void readMemory(uint addr, uint len, bool buffered);
    void writeMemory(uint addr, const QByteArray &data);

    void handleDirectTrk(const TrkResult &response);
    void directStep(uint addr);
    void handleDirectStep1(const TrkResult &response);
    void handleDirectStep2(const TrkResult &response);
    void handleDirectStep3(const TrkResult &response);

    void handleDirectWrite1(const TrkResult &response);
    void handleDirectWrite2(const TrkResult &response);
    void handleDirectWrite3(const TrkResult &response);
    void handleDirectWrite4(const TrkResult &response);
    void handleDirectWrite5(const TrkResult &response);
    void handleDirectWrite6(const TrkResult &response);
    void handleDirectWrite7(const TrkResult &response);
    void handleDirectWrite8(const TrkResult &response);
    void handleDirectWrite9(const TrkResult &response);

    QByteArray trkContinueMessage(uint threadId);
    QByteArray trkWriteRegisterMessage(trk::byte reg, uint value);
    QByteArray trkReadMemoryMessage(const MemoryRange &range);
    QByteArray trkWriteMemoryMessage(uint addr, const QByteArray &date);
    QByteArray trkBreakpointMessage(uint addr, uint len, bool armMode = true);
    QByteArray trkStepRangeMessage();
    QByteArray trkDeleteProcessMessage();
    QByteArray trkInterruptMessage();
    Q_SLOT void trkDeviceRemoved(const SymbianUtils::SymbianDevice &);

    QSharedPointer<trk::TrkDevice> m_trkDevice;
    QString m_adapterFailMessage;

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
    void tryAnswerGdbMemoryRequest(bool buffered);

    void logMessage(const QString &msg, int logChannel = LogDebug);  // triggers output() if m_verbose
    Q_SLOT void trkLogMessage(const QString &msg);

    QPointer<QTcpServer> m_gdbServer;
    QPointer<QTcpSocket> m_gdbConnection;
    QByteArray m_gdbReadBuffer;
    bool m_gdbAckMode;

    // Debuggee state
    trk::Session m_session; // global-ish data (process id, target information)
    Symbian::Snapshot m_snapshot; // local-ish data (memory and registers)
    QString m_remoteExecutable;
    QString m_remoteArguments;
    QString m_symbolFile;
    QString m_symbolFileFolder;
    int m_verbose;
    bool m_bufferedMemoryRead;
    LocalGdbProcess m_gdbProc;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_TRKGDBADAPTER_H
