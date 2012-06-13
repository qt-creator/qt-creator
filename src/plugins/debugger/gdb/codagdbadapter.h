/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef DEBUGGER_CODAGDBADAPTER_H
#define DEBUGGER_CODAGDBADAPTER_H

#include "gdbengine.h"
#include "localgdbprocess.h"
#include "callback.h"
#include "codautils.h"
#include "symbian.h"

#include <QPointer>
#include <QSharedPointer>
#include <QStringList>
#include <QHash>

QT_BEGIN_NAMESPACE
class QTcpServer;
class QTcpSocket;
class QIODevice;
QT_END_NAMESPACE

namespace Coda {
    struct CodaCommandResult;
    class CodaDevice;
    class CodaEvent;
    class CodaRunControlModuleLoadContextSuspendedEvent;
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
// CodaGdbAdapter
//
///////////////////////////////////////////////////////////////////////

class GdbCodaEngine : public GdbEngine
{
    Q_OBJECT

public:
    typedef Coda::Callback<const GdbResult &> GdbResultCallback;
    typedef Coda::Callback<const Coda::CodaCommandResult &> CodaCallback;
    typedef Coda::Callback<const GdbResponse &> GdbCallback;

    GdbCodaEngine(const DebuggerStartParameters &startParameters,
        DebuggerEngine *masterEngine);
    ~GdbCodaEngine();

    void setGdbServerName(const QString &name);
    QString gdbServerName() const { return m_gdbServerName; }

    Q_SLOT void setVerbose(const QVariant &value);
    void setVerbose(int verbose);
    void setBufferedMemoryRead(bool b) { m_bufferedMemoryRead = b; }

    void codaReloadRegisters();
    void codaReloadThreads();

signals:
    void output(const QString &msg);

public:
    //
    // Implementation of GdbProcessBase
    //
    void start(const QString &program, const QStringList &args,
        QIODevice::OpenMode mode = QIODevice::ReadWrite);
    void write(const QByteArray &data);
    bool isCodaAdapter() const { return true; }

    virtual DumperHandling dumperHandling() const { return DumperNotAvailable; }

private:
    void setupDeviceSignals();
    void setupEngine();
    void handleGdbStartFailed();
    void setupInferior();
    void runEngine();
    void interruptInferior2();
    void shutdownEngine();
    void sendRunControlTerminateCommand();
    void handleRunControlTerminate(const Coda::CodaCommandResult &);
    void sendRegistersGetMCommand();
    void handleWriteRegister(const Coda::CodaCommandResult &result);
    void reportRegisters();
    void handleReadRegisters(const Coda::CodaCommandResult &result);
    void handleRegisterChildren(const Coda::CodaCommandResult &result);
    void handleAndReportReadRegisters(const Coda::CodaCommandResult &result);
    void handleAndReportReadRegister(const Coda::CodaCommandResult &result);
    void handleAndReportReadRegistersAfterStop(const Coda::CodaCommandResult &result);
    QByteArray stopMessage() const;
    void handleAndReportSetBreakpoint(const Coda::CodaCommandResult &result);
    void handleClearBreakpoint(const Coda::CodaCommandResult &result);
    void readMemory(uint addr, uint len, bool buffered);
    void handleReadMemoryBuffered(const Coda::CodaCommandResult &result);
    void handleReadMemoryUnbuffered(const Coda::CodaCommandResult &result);
    void handleWriteMemory(const Coda::CodaCommandResult &result);
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

    QSharedPointer<Coda::CodaDevice> m_codaDevice;

    //
    // Gdb
    //
    Q_SLOT void handleGdbConnection();
    Q_SLOT void readGdbServerCommand();
    Q_SLOT void codaDeviceError(const QString &);
    Q_SLOT void codaDeviceRemoved(const SymbianUtils::SymbianDevice &dev);
    Q_SLOT void codaEvent(const Coda::CodaEvent &knownEvent);
    void handleCodaRunControlModuleLoadContextSuspendedEvent(const Coda::CodaRunControlModuleLoadContextSuspendedEvent &e);
    inline void sendContinue();
    void sendStepRange();
    void handleStep(const Coda::CodaCommandResult &result);
    void handleCreateProcess(const Coda::CodaCommandResult &result);

    void readGdbResponse();
    void handleGdbServerCommand(const QByteArray &cmd);
    void sendGdbServerMessage(const QByteArray &msg,
        const QByteArray &logNote = QByteArray());
    void sendGdbServerAck();
    bool sendGdbServerPacket(const QByteArray &packet, bool doFlush);
    void gdbSetCurrentThread(const QByteArray &cmd, const char *why);

    void logMessage(const QString &msg, int channel = LogDebug);  // triggers output() if m_verbose
    Q_SLOT void codaLogMessage(const QString &msg);

    QPointer<QTcpServer> m_gdbServer;
    QPointer<QTcpSocket> m_gdbConnection;
    QByteArray m_gdbReadBuffer;
    bool m_gdbAckMode;

    // Debuggee state
    Coda::Session m_session; // global-ish data (process id, target information)
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
    QByteArray m_codaProcessId;
    LocalGdbProcess m_gdbProc;
    bool m_firstHelloEvent;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_CODAGDBADAPTER_H
