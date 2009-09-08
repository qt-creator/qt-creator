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

#include "trkutils.h"
#include "trkdevicex.h"

#include <QtCore/QCoreApplication>
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

#include <QtGui/QApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QTextBlock>
#include <QtGui/QTextEdit>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

/*
fetch-register          p                       info registers
set-register            P                       set
binary-download         X                       load, set
read-aux-vector         qXfer:auxv:read         info auxv
symbol-lookup           qSymbol                 Detecting multiple threads
attach                  vAttach                 attach
verbose-resume          vCont                   Stepping or resuming multiple threads
run                     vRun                    run
software-breakpoint     Z0                      break
hardware-breakpoint     Z1                      hbreak
write-watchpoint        Z2                      watch
read-watchpoint         Z3                      rwatch
access-watchpoint       Z4                      awatch
target-features         qXfer:features:read     set architecture
library-info            qXfer:libraries:read    info sharedlibrary
memory-map              qXfer:memory-map:read   info mem
read-spu-object         qXfer:spu:read          info spu
write-spu-object        qXfer:spu:write         info spu
get-thread-local-
storage-address         qGetTLSAddr             Displaying __thread variables
supported-packets       qSupported              Remote communications parameters
pass-signals            QPassSignals            handle signal
hostio-close-packet     vFile:close             remote get, remote put
hostio-open-packet      vFile:open              remote get, remote put
hostio-pread-packet     vFile:pread             remote get, remote put
hostio-pwrite-packet    vFile:pwrite            remote get, remote put
hostio-unlink-packet    vFile:unlink            remote delete
*/

using namespace trk;

enum { KnownRegisters = RegisterPSGdb + 1};

static const char *registerNames[KnownRegisters] =
{
    "A1", "A2", "A3", "A4",
    0, 0, 0, 0,
    0, 0, 0, "AP",
    "IP", "SP", "LR", "PC",
    "PSTrk", 0, 0, 0,
    0, 0, 0, 0,
    0, "PSGdb"
};

static QByteArray dumpRegister(int n, uint value)
{
    QByteArray ba;
    ba += ' ';
    if (n < KnownRegisters && registerNames[n]) {
        ba += registerNames[n];
    } else {
        ba += '#';
        ba += QByteArray::number(n);
    }
    ba += "=" + hexxNumber(value);
    return ba;
}

#define TrkCB(s) TrkCallback(this, &Adapter::s)
#define GdbCB(s) GdbCallback(this, &Adapter::s)

struct GdbResult
{
    QByteArray data;
};

///////////////////////////////////////////////////////////////////////
//
// Adapter
//
///////////////////////////////////////////////////////////////////////

class Adapter : public QObject
{
    Q_OBJECT

public:
    typedef TrkFunctor1<const TrkResult &> TrkCallback;
    typedef TrkFunctor1<const GdbResult &> GdbCallback;

    Adapter();
    ~Adapter();
    void setGdbServerName(const QString &name);
    QString gdbServerIP() const;
    uint gdbServerPort() const;
    void setTrkServerName(const QString &name) { m_trkServerName = name; }
    void setVerbose(int verbose) { m_verbose = verbose; }
    void setSerialFrame(bool b) { m_serialFrame = b; }
    void setRegisterEndianness(Endianness r) { m_registerEndianness = r; }
    void setBufferedMemoryRead(bool b) { m_bufferedMemoryRead = b; }

public slots:
    void startInferior();

signals:
    void output(const QString &senderName, const QString &data);

private slots:
    void handleProcError(QProcess::ProcessError error);
    void handleProcFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleProcStarted();
    void handleProcStateChanged(QProcess::ProcessState newState);
    void run();
    void startGdb();

private:
    friend class RunnerGui;
    void connectProcess(QProcess *proc);
    void sendOutput(QObject *sender, const QString &data);
    void sendOutput(const QString &data) { sendOutput(0, data); }

    QString m_trkServerName; // 
    QString m_gdbServerName; // 127.0.0.1:(2222+uid)

    QProcess m_gdbProc;
    QProcess m_rfcommProc;
    bool m_running;

public:
    //
    // TRK
    //
    bool openTrkPort(const QString &port, QString *errorMessage);
    void sendTrkMessage(byte code,
        TrkCallback callback = TrkCallback(),
        const QByteArray &data = QByteArray(),
        const QVariant &cookie = QVariant());
    Q_SLOT void handleTrkResult(const trk::TrkResult &data);
    Q_SLOT void handleTrkError(const QString &msg);

    // convenience messages
    void waitForTrkFinished();
    void sendTrkAck(byte token);

    // kill process and breakpoints
    void cleanUp();

    void handleCpuType(const TrkResult &result);
    void handleCreateProcess(const TrkResult &result);
    void handleClearBreakpoint(const TrkResult &result);
    void handleSignalContinue(const TrkResult &result);
    void handleWaitForFinished(const TrkResult &result);
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

    void clearTrkBreakpoint(const Breakpoint &bp);

    // set breakpoints behind gdb's back
    void setTrkBreakpoint(const Breakpoint &bp);
    void handleSetTrkBreakpoint(const TrkResult &result);
    void handleSetTrkMainBreakpoint(const TrkResult &result);

    void readMemory(uint addr, uint len);
    void interruptInferior();

    TrkDevice m_trkDevice;

    QList<Breakpoint> m_breakpoints;

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

    Q_SLOT void handleGdbReadyReadStandardError();
    Q_SLOT void handleGdbReadyReadStandardOutput();
    void logMessage(const QString &msg, bool force = false);
    Q_SLOT void trkLogMessage(const QString &msg);

    void handleInfoAddress(const GdbResult &result);
    void handleInfoMainAddress(const GdbResult &result);

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

    // Debuggee state
    void executeCommand(const QString &msg);
    Session m_session; // global-ish data (process id, target information)
    Snapshot m_snapshot; // local-ish data (memory and registers)
    int m_verbose;
    Endianness m_registerEndianness;
    bool m_serialFrame;
    bool m_bufferedMemoryRead;
};

Adapter::Adapter()
{
    m_running = false;
    m_gdbAckMode = true;
    m_verbose = 2;
    m_registerEndianness = LittleEndian;
    //m_serialFrame = true;
    m_serialFrame = false;
    m_bufferedMemoryRead = true;
    //m_bufferedMemoryRead = false;
    // m_breakpoints.append(Breakpoint(0x0040)); // E32Main
    m_breakpoints.append(Breakpoint(0x0cc8)); // E32Main
    m_breakpoints.append(Breakpoint(0x0cd0)); // E32Main
    m_trkServerName = "/dev/rfcomm0";

    uid_t userId = getuid();
    m_gdbServerName = QString("127.0.0.1:%1").arg(2222 + userId);

    m_gdbProc.setObjectName("GDB PROCESS");
    connectProcess(&m_gdbProc);
    connect(&m_gdbProc, SIGNAL(readyReadStandardError()),
        this, SLOT(handleGdbReadyReadStandardError()));
    connect(&m_gdbProc, SIGNAL(readyReadStandardOutput()),
        this, SLOT(handleGdbReadyReadStandardOutput()));

    m_rfcommProc.setObjectName("RFCOMM PROCESS");
    connectProcess(&m_rfcommProc);
    connect(&m_rfcommProc, SIGNAL(readyReadStandardError()),
        this, SLOT(handleRfcommReadyReadStandardError()));
    connect(&m_rfcommProc, SIGNAL(readyReadStandardOutput()),
        this, SLOT(handleRfcommReadyReadStandardOutput()));

    connect(&m_trkDevice, SIGNAL(logMessage(QString)),
        this, SLOT(trkLogMessage(QString)));
}

Adapter::~Adapter()
{
    m_gdbServer.close();
    logMessage("Shutting down.\n", true);
}

void Adapter::trkLogMessage(const QString &msg)
{
    logMessage("TRK " + msg);
}

void Adapter::setGdbServerName(const QString &name)
{
    m_gdbServerName = name;
}

QString Adapter::gdbServerIP() const
{
    int pos = m_gdbServerName.indexOf(':');
    if (pos == -1)
        return m_gdbServerName;
    return m_gdbServerName.left(pos);
}

uint Adapter::gdbServerPort() const
{
    int pos = m_gdbServerName.indexOf(':');
    if (pos == -1)
        return 0;
    return m_gdbServerName.mid(pos + 1).toUInt();
}

QByteArray Adapter::trkContinueMessage()
{
    QByteArray ba;
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    return ba;
}

QByteArray Adapter::trkReadRegisterMessage()
{
    QByteArray ba;
    appendByte(&ba, 0); // Register set, only 0 supported
    appendShort(&ba, 0);
    appendShort(&ba, RegisterCount - 1); // last register
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    return ba;
}

QByteArray Adapter::trkReadMemoryMessage(uint addr, uint len)
{
    QByteArray ba;
    appendByte(&ba, 0x08); // Options, FIXME: why?
    appendShort(&ba, len);
    appendInt(&ba, addr);
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    return ba;
}

void Adapter::startInferior()
{
    QString errorMessage;
    if (!openTrkPort(m_trkServerName, &errorMessage)) {
        logMessage(errorMessage, true);
        logMessage("LOOPING");
        QTimer::singleShot(1000, this, SLOT(startInferior()));
        return;
    }

    m_trkDevice.sendTrkInitialPing();
    sendTrkMessage(0x01); // Connect
    sendTrkMessage(0x05, TrkCB(handleSupportMask));
    sendTrkMessage(0x06, TrkCB(handleCpuType));
    sendTrkMessage(0x04, TrkCB(handleTrkVersions)); // Versions
    //sendTrkMessage(0x09); // Unrecognized command
    //sendTrkMessage(0x4a, 0,
    //    "10 " + formatString("C:\\data\\usingdlls.sisx")); // Open File
    //sendTrkMessage(0x4B, 0, "00 00 00 01 73 1C 3A C8"); // Close File

    QByteArray ba;
    appendByte(&ba, 0); // ?
    appendByte(&ba, 0); // ?
    appendByte(&ba, 0); // ?

    QByteArray file("C:\\sys\\bin\\filebrowseapp.exe");
    appendString(&ba, file, TargetByteOrder);
    sendTrkMessage(0x40, TrkCB(handleCreateProcess), ba); // Create Item
    //sendTrkMessage(TRK_WRITE_QUEUE_NOOP_CODE, TrkCB(startGdbServer));
}

void Adapter::logMessage(const QString &msg, bool force)
{
    if (m_verbose || force)
        emit output(QString(), msg);
}

//
// Gdb
//
void Adapter::handleGdbConnection()
{
    logMessage("HANDLING GDB CONNECTION");

    m_gdbConnection = m_gdbServer.nextPendingConnection();
    connect(m_gdbConnection, SIGNAL(disconnected()),
            m_gdbConnection, SLOT(deleteLater()));
    connect(m_gdbConnection, SIGNAL(readyRead()),
            this, SLOT(readGdbServerCommand()));
}

static inline QString msgGdbPacket(const QString &p)
{
    return QLatin1String("gdb:                              ") + p;
}

void Adapter::readGdbServerCommand()
{
    QByteArray packet = m_gdbConnection->readAll();
    m_gdbReadBuffer.append(packet);

    logMessage("gdb: -> " + QString::fromAscii(packet));
    if (packet != m_gdbReadBuffer)
        logMessage("buffer: " + m_gdbReadBuffer);

    QByteArray &ba = m_gdbReadBuffer;
    while (ba.size()) {
        char code = ba.at(0);
        ba = ba.mid(1);

        if (code == '+') {
            //logMessage("ACK");
            continue;
        }

        if (code == '-') {
            logMessage("NAK: Retransmission requested");
            continue;
        }

        if (code == char(0x03)) {
            logMessage("INTERRUPT RECEIVED");
            interruptInferior();
            continue;
        }

        if (code != '$') {
            logMessage("Broken package (2) " + quoteUnprintableLatin1(ba)
                + hexNumber(code));
            continue;
        }

        int pos = ba.indexOf('#');
        if (pos == -1) {
            logMessage("Invalid checksum format in "
                + quoteUnprintableLatin1(ba));
            continue;
        }

        bool ok = false;
        uint checkSum = ba.mid(pos + 1, 2).toUInt(&ok, 16);
        if (!ok) {
            logMessage("Invalid checksum format 2 in "
                + quoteUnprintableLatin1(ba));
            return;
        }

        //logMessage(QString("Packet checksum: %1").arg(checkSum));
        byte sum = 0;
        for (int i = 0; i < pos; ++i)
            sum += ba.at(i);

        if (sum != checkSum) {
            logMessage(QString("ERROR: Packet checksum wrong: %1 %2 in "
                + quoteUnprintableLatin1(ba)).arg(checkSum).arg(sum));
        }

        QByteArray cmd = ba.left(pos);
        ba.remove(0, pos + 3);
        handleGdbServerCommand(cmd);
    }
}

bool Adapter::sendGdbServerPacket(const QByteArray &packet, bool doFlush)
{
    if (!m_gdbConnection) {
        logMessage(QString::fromLatin1("Cannot write to gdb: No connection (%1)")
            .arg(QString::fromLatin1(packet)), true);
        return false;
    }
    if (m_gdbConnection->state() != QAbstractSocket::ConnectedState) {
        logMessage(QString::fromLatin1("Cannot write to gdb: Not connected (%1)")
            .arg(QString::fromLatin1(packet)), true);
        return false;
    }
    if (m_gdbConnection->write(packet) == -1) {
        logMessage(QString::fromLatin1("Cannot write to gdb: %1 (%2)")
            .arg(m_gdbConnection->errorString()).arg(QString::fromLatin1(packet)), true);
        return false;
    }
    if (doFlush)
        m_gdbConnection->flush();
    return true;
}

void Adapter::sendGdbServerAck()
{
    if (!m_gdbAckMode)
        return;
    QByteArray packet = "+";
    logMessage("gdb: <- " + packet);
    sendGdbServerPacket(packet, false);
}

void Adapter::sendGdbServerMessage(const QByteArray &msg, const QByteArray &logNote)
{
    byte sum = 0;
    for (int i = 0; i != msg.size(); ++i)
        sum += msg.at(i);

    char checkSum[30];
    qsnprintf(checkSum, sizeof(checkSum) - 1, "%02x ", sum);

    //logMessage(QString("Packet checksum: %1").arg(sum));

    QByteArray packet;
    packet.append("$");
    packet.append(msg);
    packet.append('#');
    packet.append(checkSum);
    int pad = qMax(0, 24 - packet.size());
    logMessage("gdb: <- " + packet + QByteArray(pad, ' ') + logNote);
    sendGdbServerPacket(packet, true);
}

void Adapter::sendGdbServerMessageAfterTrkResponse(const QByteArray &msg,
    const QByteArray &logNote)
{
    QByteArray ba = msg + char(1) + logNote;
    sendTrkMessage(TRK_WRITE_QUEUE_NOOP_CODE, TrkCB(reportToGdb), "", ba); // Answer gdb
}

void Adapter::reportToGdb(const TrkResult &result)
{
    QByteArray message = result.cookie.toByteArray();
    QByteArray note;
    int pos = message.lastIndexOf(char(1)); // HACK
    if (pos != -1) {
        note = message.mid(pos + 1);
        message = message.left(pos);
    }
    message.replace("@CODESEG@", hexNumber(m_session.codeseg));
    message.replace("@DATASEG@", hexNumber(m_session.dataseg));
    message.replace("@PID@", hexNumber(m_session.pid));
    message.replace("@TID@", hexNumber(m_session.tid));
    sendGdbServerMessage(message, note);
}

QByteArray Adapter::trkBreakpointMessage(uint addr, uint len, bool armMode)
{
    QByteArray ba;
    appendByte(&ba, 0x82);  // unused option
    appendByte(&ba, armMode /*bp.mode == ArmMode*/ ? 0x00 : 0x01);
    appendInt(&ba, addr);
    appendInt(&ba, len);
    appendInt(&ba, 0x00000001);
    appendInt(&ba, m_session.pid);
    appendInt(&ba, 0xFFFFFFFF);
    return ba;
}

void Adapter::handleGdbServerCommand(const QByteArray &cmd)
{
    // http://sourceware.org/gdb/current/onlinedocs/gdb_34.html
    if (0) {}

    else if (cmd == "!") {
        sendGdbServerAck();
        //sendGdbServerMessage("", "extended mode not enabled");
        sendGdbServerMessage("OK", "extended mode enabled");
    }

    else if (cmd.startsWith("?")) {
        logMessage(msgGdbPacket(QLatin1String("Query halted")));
        // Indicate the reason the target halted.
        // The reply is the same as for step and continue.
        sendGdbServerAck();
        // The command below will trigger fetching a stack trace while
        // the process does not seem to be fully functional. Most notably
        // the PC points to a 0x9..., which is not in "our" range
        //sendGdbServerMessage("T05library:r;", "target halted (library load)");
        //sendGdbServerMessage("S05", "target halted (trap)");
        sendGdbServerMessage("S00", "target halted (trap)");
        //sendGdbServerMessage("O" + QByteArray("Starting...").toHex());
    }

    else if (cmd == "c") {
        logMessage(msgGdbPacket(QLatin1String("Continue")));
        sendGdbServerAck();
        QByteArray ba;
        appendByte(&ba, 0); // options
        appendInt(&ba, 0); // start address
        appendInt(&ba, 0); // end address
        appendInt(&ba, m_session.pid);
        appendInt(&ba, m_session.tid);
        sendTrkMessage(0x18, TrkCallback(), ba);
    }

    else if (cmd.startsWith("C")) {
        logMessage(msgGdbPacket(QLatin1String("Continue with signal")));
        // C sig[;addr] Continue with signal sig (hex signal number)
        //Reply: See section D.3 Stop Reply Packets, for the reply specifications.
        sendGdbServerAck();
        bool ok = false;
        uint signalNumber = cmd.mid(1).toInt(&ok, 16);
        QByteArray ba;
        appendInt(&ba, m_session.pid);
        appendInt(&ba, m_session.tid);
        sendTrkMessage(0x18, TrkCB(handleSignalContinue), ba, signalNumber); // Continue
    }

    else if (cmd.startsWith("D")) {
        sendGdbServerAck();
        sendGdbServerMessage("OK", "shutting down");
        qApp->quit();
    }

    else if (cmd == "g") {
        logMessage(msgGdbPacket(QLatin1String("Read registers")));
        // Read general registers.
        //sendGdbServerMessage("00000000", "read registers");
        sendGdbServerAck();
        sendTrkMessage(0x12, TrkCB(handleAndReportReadRegisters),
            trkReadRegisterMessage());
    }

    else if (cmd.startsWith("Hc")) {
        logMessage(msgGdbPacket(QLatin1String("Set thread & continue")));
        // Set thread for subsequent operations (`m', `M', `g', `G', et.al.).
        // for step and continue operations
        //$Hc-1#09
        sendGdbServerAck();
        sendGdbServerMessage("OK", "Set current thread for step & continue");
    }

    else if (cmd.startsWith("Hg")) {
        logMessage(msgGdbPacket(QLatin1String("Set thread")));
        // Set thread for subsequent operations (`m', `M', `g', `G', et.al.).
        // for 'other operations.  0 - any thread
        //$Hg0#df
        sendGdbServerAck();
        m_session.currentThread = cmd.mid(2).toInt(0, 16);
        sendGdbServerMessage("OK", "Set current thread "
            + QByteArray::number(m_session.currentThread));
    }

    else if (cmd == "k") {
        logMessage(msgGdbPacket(QLatin1String("kill")));
        // kill
        sendGdbServerAck();
        QByteArray ba;
        appendByte(&ba, 0); // ?
        appendByte(&ba, 0); // Sub-command: Delete Process
        appendInt(&ba, m_session.pid);
        sendTrkMessage(0x41, TrkCallback(), ba, "Delete process"); // Delete Item
        sendGdbServerMessageAfterTrkResponse("", "process killed");
    }

    else if (cmd.startsWith("m")) {
        logMessage(msgGdbPacket(QLatin1String("Read memory")));
        // m addr,length
        sendGdbServerAck();
        uint addr = 0, len = 0;
        do {
            const int pos = cmd.indexOf(',');
            if (pos == -1)
                break;
            bool ok;
            addr = cmd.mid(1, pos - 1).toUInt(&ok, 16);
            if (!ok)
                break;
            len = cmd.mid(pos + 1).toUInt(&ok, 16);
            if (!ok)
                break;
        } while (false);
        if (len) {
            readMemory(addr, len);
        } else {
            sendGdbServerMessage("E20", "Error " + cmd);
        }
    }
    else if (cmd.startsWith("p")) {
        logMessage(msgGdbPacket(QLatin1String("read register")));
        // 0xf == current instruction pointer?
        //sendGdbServerMessage("0000", "current IP");
        sendGdbServerAck();
        #if 0
          A1 = 0,	 first integer-like argument
          A4 = 3,	 last integer-like argument
          AP = 11,
          IP = 12,
          SP = 13,	 Contains address of top of stack
          LR = 14,	 address to return to from a function call
          PC = 15,	 Contains program counter
          F0 = 16,	 first floating point register
          F3 = 19,	 last floating point argument register
          F7 = 23, 	 last floating point register
          FPS = 24,	 floating point status register
          PS = 25,	 Contains processor status
          WR0,		 WMMX data registers.
          WR15 = WR0 + 15,
          WC0,		 WMMX control registers.
          WCSSF = WC0 + 2,
          WCASF = WC0 + 3,
          WC7 = WC0 + 7,
          WCGR0,		WMMX general purpose registers.
          WCGR3 = WCGR0 + 3,
          WCGR7 = WCGR0 + 7,
          NUM_REGS,

          // Other useful registers.
          FP = 11,		Frame register in ARM code, if used.
          THUMB_FP = 7,		Frame register in Thumb code, if used.
          NUM_ARG_REGS = 4,
          LAST_ARG = A4,
          NUM_FP_ARG_REGS = 4,
          LAST_FP_ARG = F3
        #endif
        bool ok = false;
        const uint registerNumber = cmd.mid(1).toInt(&ok, 16);
        QByteArray logMsg = "Read Register";
        if (registerNumber == RegisterPSGdb) {
            QByteArray ba;
            appendInt(&ba, m_snapshot.registers[RegisterPSTrk], m_registerEndianness);
            logMsg += dumpRegister(registerNumber, m_snapshot.registers[RegisterPSTrk]);
            sendGdbServerMessage(ba.toHex(), logMsg);
        } else if (registerNumber < RegisterCount) {
            QByteArray ba;
            appendInt(&ba, m_snapshot.registers[registerNumber], m_registerEndianness);
            logMsg += dumpRegister(registerNumber, m_snapshot.registers[registerNumber]);
            sendGdbServerMessage(ba.toHex(), logMsg);
        } else {
            sendGdbServerMessage("0000", "read single unknown register #" + QByteArray::number(registerNumber));
            //sendGdbServerMessage("E01", "read single unknown register");
        }
    }

    else if (cmd == "qAttached") {
        //$qAttached#8f
        // 1: attached to an existing process
        // 0: created a new process
        sendGdbServerAck();
        sendGdbServerMessage("0", "new process created");
        //sendGdbServerMessage("1", "attached to existing process");
        //sendGdbServerMessage("E01", "new process created");
    }

    else if (cmd.startsWith("qC")) {
        logMessage(msgGdbPacket(QLatin1String("query thread id")));
        // Return the current thread ID
        //$qC#b4
        sendGdbServerAck();
        sendGdbServerMessageAfterTrkResponse("QC@TID@");
    }

    else if (cmd.startsWith("qSupported")) {
        //$qSupported#37
        //$qSupported:multiprocess+#c6
        //logMessage("Handling 'qSupported'");
        sendGdbServerAck();
        if (0)
            sendGdbServerMessage(QByteArray(), "nothing supported");
        else
            sendGdbServerMessage(
                "PacketSize=7cf;"
                //"QPassSignals+;"
                "qXfer:libraries:read+;"
                //"qXfer:auxv:read+;"
                "qXfer:features:read+");
    }

    else if (cmd == "qPacketInfo") {
        // happens with  gdb 6.4.50.20060226-cvs / CodeSourcery
        // deprecated by qSupported?
        sendGdbServerAck();
        sendGdbServerMessage("", "FIXME: nothing?");
    }

    else if (cmd == "qOffsets") {
        sendGdbServerAck();
        sendGdbServerMessageAfterTrkResponse("TextSeg=@CODESEG@;DataSeg=@DATASEG@");
    }

    else if (cmd == "qSymbol::") {
        if (m_verbose)
            logMessage(msgGdbPacket(QLatin1String("notify can handle symbol lookup")));
        // Notify the target that GDB is prepared to serve symbol lookup requests.
        sendGdbServerAck();
        if (1)
            sendGdbServerMessage("OK", "no further symbols needed");
        else
            sendGdbServerMessage("qSymbol:" + QByteArray("_Z7E32Mainv").toHex(), "ask for more");
    }

    else if (cmd.startsWith("qXfer:features:read:target.xml:")) {
        //  $qXfer:features:read:target.xml:0,7ca#46...Ack
        sendGdbServerAck();
        sendGdbServerMessage("l<target><architecture>symbianelf</architecture></target>");
    }

    else if (cmd == "QStartNoAckMode") {
        //$qSupported#37
        //logMessage("Handling 'QStartNoAckMode'");
        sendGdbServerAck();
        sendGdbServerMessage("OK", "ack no-ack mode");
        m_gdbAckMode = false;
    }

    else if (cmd.startsWith("QPassSignals")) {
        // list of signals to pass directly to inferior
        // $QPassSignals:e;10;14;17;1a;1b;1c;21;24;25;4c;#8f
        // happens only if "QPassSignals+;" is qSupported
        sendGdbServerAck();
        // FIXME: use the parameters
        sendGdbServerMessage("OK", "passing signals accepted");
    }

    else if (cmd == "s" || cmd.startsWith("vCont;s")) {
        logMessage(msgGdbPacket(QLatin1String("Step range")));
        logMessage("  from " + hexxNumber(m_snapshot.registers[RegisterPC]));
        sendGdbServerAck();
        m_running = true;
        QByteArray ba;
        appendByte(&ba, 1); // options
        appendInt(&ba, m_snapshot.registers[RegisterPC]); // start address
        appendInt(&ba, m_snapshot.registers[RegisterPC] + 4); // end address
        appendInt(&ba, m_session.pid);
        appendInt(&ba, m_session.tid);
        sendTrkMessage(0x19, TrkCB(handleStepRange), ba, "Step range");
        // FIXME: should be triggered by "real" stop"
        //sendGdbServerMessageAfterTrkResponse("S05", "target halted");
    }

    else if (cmd == "vCont?") {
        // actions supported by the vCont packet
        sendGdbServerAck();
        sendGdbServerMessage("OK"); // we don't support vCont.
        //sendGdbServerMessage("vCont;c");
    }

    else if (cmd == "vCont;c") {
        // vCont[;action[:thread-id]]...'
        sendGdbServerAck();
        m_running = true;
        sendTrkMessage(0x18, TrkCallback(), trkContinueMessage(), "CONTINUE");
    }

    else if (cmd.startsWith("vKill")) {
        // kill
        sendGdbServerAck();
        QByteArray ba;
        appendByte(&ba, 0); // Sub-command: Delete Process
        appendInt(&ba, m_session.pid);
        sendTrkMessage(0x41, TrkCallback(), ba, "Delete process"); // Delete Item
        sendGdbServerMessageAfterTrkResponse("", "process killed");
    }

    else if (0 && cmd.startsWith("Z0,")) {
        // Tell gdb  we don't support software breakpoints
        sendGdbServerMessage("");
    }

    else if (cmd.startsWith("Z0,") || cmd.startsWith("Z1,")) {
        // Insert breakpoint
        if (m_verbose)
            logMessage(msgGdbPacket(QLatin1String("Insert breakpoint")));
        // $z0,786a4ccc,4#99
        const int pos = cmd.lastIndexOf(',');
        bool ok = false;
        const uint addr = cmd.mid(3, pos - 3).toInt(&ok, 16);
        const uint len = cmd.mid(pos + 1).toInt(&ok, 16);
        qDebug() << "ADDR: " << hexNumber(addr) << " LEN: " << len;
        if (m_verbose)
            logMessage(QString::fromLatin1("Inserting breakpoint at 0x%1, %2")
                .arg(addr, 0, 16).arg(len));

        //---IDE------------------------------------------------------
        //  Command: 0x1B Set Break
        //BreakType: 0x82
        //  Options: 0x00
        //  Address: 0x78674340 (2020033344)    i.e + 0x00000340
        //   Length: 0x00000001 (1)
        //    Count: 0x00000000 (0)
        //ProcessID: 0x000001b5 (437)
        // ThreadID: 0xffffffff (-1)
        // [1B 09 82 00 78 67 43 40 00 00 00 01 00 00 00 00
        //  00 00 01 B5 FF FF FF FF]
        const QByteArray ba = trkBreakpointMessage(addr, len, m_session.pid);
        sendTrkMessage(0x1B, TrkCB(handleAndReportSetBreakpoint), ba);
        //m_session.toekn

        //---TRK------------------------------------------------------
        //  Command: 0x80 Acknowledge
        //    Error: 0x00
        // [80 09 00 00 00 00 0A]
    } else if (cmd.startsWith("qPart:") || cmd.startsWith("qXfer:"))  {
        QByteArray data  = cmd.mid(1 + cmd.indexOf(':'));
        // "qPart:auxv:read::0,147": Read OS auxiliary data (see info aux)
        bool handled = false;
        if (data.startsWith("auxv:read::")) {
            const int offsetPos = data.lastIndexOf(':') + 1;
            const int commaPos = data.lastIndexOf(',');
            if (commaPos != -1) {                
                bool ok1 = false, ok2 = false;
                const int offset = data.mid(offsetPos,  commaPos - offsetPos)
                    .toInt(&ok1, 16);
                const int length = data.mid(commaPos + 1).toInt(&ok2, 16);
                if (ok1 && ok2) {
                    const QString msg = QString::fromLatin1("Read of OS auxilary "
                        "vector (%1, %2) not implemented.").arg(offset).arg(length);
                    logMessage(msgGdbPacket(msg), true);
                    sendGdbServerMessage("E20", msg.toLatin1());
                    handled = true;
                }
            }
        } // auxv read
        if (!handled) {
            const QString msg = QLatin1String("FIXME unknown 'XFER'-request: ")
                + QString::fromAscii(cmd);
            logMessage(msgGdbPacket(msg), true);
            sendGdbServerMessage("E20", msg.toLatin1());
        }
    } // qPart/qXfer
    else {
        logMessage(msgGdbPacket(QLatin1String("FIXME unknown: ")
            + QString::fromAscii(cmd)));
    }
}

void Adapter::executeCommand(const QString &msg)
{
    if (msg == "EI") {
        sendGdbMessage("-exec-interrupt");
    } else if (msg == "C") {
        sendTrkMessage(0x18, TrkCallback(), trkContinueMessage(), "CONTINUE");
    } else if (msg == "R") {
        sendTrkMessage(0x18, TrkCB(handleReadRegisters),
            trkReadRegisterMessage(), "READ REGS");
    } else if (msg == "I") {
        interruptInferior();
    } else {
        logMessage("EXECUTING GDB COMMAND " + msg);
        sendGdbMessage(msg);
    }
}

bool Adapter::openTrkPort(const QString &port, QString *errorMessage)
{
    connect(&m_trkDevice, SIGNAL(messageReceived(trk::TrkResult)),
        this, SLOT(handleTrkResult(trk::TrkResult)));
    connect(&m_trkDevice, SIGNAL(error(QString)),
        this, SLOT(handleTrkError(QString)));
    if (m_verbose > 1)
        m_trkDevice.setVerbose(true);
    m_trkDevice.setSerialFrame(m_serialFrame);
    return m_trkDevice.open(port, errorMessage);
}

void Adapter::sendTrkMessage(byte code, TrkCallback callback,
    const QByteArray &data, const QVariant &cookie)
{
    m_trkDevice.sendTrkMessage(code, callback, data, cookie);
}

void Adapter::waitForTrkFinished()
{
    // initiate one last roundtrip to ensure all is flushed
    sendTrkMessage(0x0, TrkCB(handleWaitForFinished));
}

void Adapter::sendTrkAck(byte token)
{
    logMessage(QString("SENDING ACKNOWLEDGEMENT FOR TOKEN %1").arg(int(token)));
    m_trkDevice.sendTrkAck(token);
}

void Adapter::handleTrkError(const QString &msg)
{
    logMessage("## TRK ERROR: " + msg);
}

void Adapter::handleTrkResult(const TrkResult &result)
{
    if (result.isDebugOutput) {
        sendTrkAck(result.token);
        logMessage(QLatin1String("APPLICATION OUTPUT: ") +
            QString::fromAscii(result.data));
        sendGdbServerMessage("O" + result.data.toHex());
        return;
    }
    logMessage("READ TRK " + result.toString());
    QByteArray prefix = "READ BUF:                                       ";
    QByteArray str = result.toString().toUtf8();
    switch (result.code) {
        case 0x80: // ACK
            break;
        case 0xff: { // NAK. This mostly means transmission error, not command failed.
            QString logMsg;
            QTextStream(&logMsg) << prefix << "NAK: for token=" << result.token
                << " ERROR: " << errorMessage(result.data.at(0)) << ' ' << str;
            logMessage(logMsg, true);
            break;
        }
        case 0x90: { // Notified Stopped
            // 90 01   78 6a 40 40   00 00 07 23   00 00 07 24  00 00
            const char *data = result.data.data();
            const uint addr = extractInt(data);
            const uint pid = extractInt(data + 4);
            const uint tid = extractInt(data + 8);
            logMessage(prefix + QString::fromLatin1("NOTE: PID %1/TID %2 "
                "STOPPED at 0x%3").arg(pid).arg(tid).arg(addr, 0, 16));
            sendTrkAck(result.token);
            if (addr) {
                // Todo: Do not send off GdbMessages if a synced gdb
                // query is pending, queue instead
                if (m_running) {
                    m_running = false;
                    sendGdbServerMessage("S05", "Target stopped");
                }
            } else {
                if (m_verbose)
                    logMessage(QLatin1String("Ignoring stop at 0"));
            }
            break;
        }
        case 0x91: { // Notify Exception (obsolete)
            logMessage(prefix + "NOTE: EXCEPTION  " + str);
            sendTrkAck(result.token);
            break;
        }
        case 0x92: { //
            logMessage(prefix + "NOTE: INTERNAL ERROR: " + str);
            sendTrkAck(result.token);
            break;
        }

        // target->host OS notification
        case 0xa0: { // Notify Created
            const char *data = result.data.data();
            const byte error = result.data.at(0);
            // type: 1 byte; for dll item, this value is 2.
            const byte type = result.data.at(1);
            const uint pid = extractInt(data + 2);
            const uint tid = extractInt(data + 6);
            const uint codeseg = extractInt(data + 10);
            const uint dataseg = extractInt(data + 14);
            const uint len = extractShort(data + 18);
            const QByteArray name = result.data.mid(20, len); // library name
            m_session.modules += QString::fromAscii(name);
            QString logMsg;
            QTextStream str(&logMsg);
            str << prefix << " NOTE: LIBRARY LOAD: token=" << result.token;
            if (error)
                str << " ERROR: " << int(error);
            str << " TYPE: " << int(type) << " PID: " << pid << " TID:   " <<  tid;
            str << " CODE: " << hexxNumber(codeseg);
            str << " DATA: " << hexxNumber(dataseg);
            str << " NAME: '" << name << '\'';
            logMessage(logMsg);
            // This lets gdb trigger a register update etc
            //sendGdbServerMessage("T05library:r;");
            sendTrkMessage(0x18, TrkCallback(), trkContinueMessage(), "CONTINUE");
            break;
        }
        case 0xa1: { // NotifyDeleted
            const ushort itemType = extractByte(result.data.data() + 1);
            const ushort len = result.data.size() > 12
                ? extractShort(result.data.data() + 10) : ushort(0);
            const QString name = len
                ? QString::fromAscii(result.data.mid(12, len)) : QString();
            if (!name.isEmpty())
                m_session.modules.removeAll(name);
            logMessage(QString::fromLatin1("%1 %2 UNLOAD: %3")
                .arg(QString::fromAscii(prefix))
                .arg(itemType ? QLatin1String("LIB") : QLatin1String("PROCESS"))
                .arg(name));
            sendTrkAck(result.token);
            if (itemType == 0) {
                sendGdbServerMessage("W00", "Process exited");
                //sendTrkMessage(0x02, TrkCB(handleDisconnect));
            }
            break;
        }
        case 0xa2: { // NotifyProcessorStarted
            logMessage(prefix + "NOTE: PROCESSOR STARTED: " + str);
            sendTrkAck(result.token);
            break;
        }
        case 0xa6: { // NotifyProcessorStandby
            logMessage(prefix + "NOTE: PROCESSOR STANDBY: " + str);
            sendTrkAck(result.token);
            break;
        }
        case 0xa7: { // NotifyProcessorReset
            logMessage(prefix + "NOTE: PROCESSOR RESET: " + str);
            sendTrkAck(result.token);
            break;
        }
        default: {
            logMessage(prefix + "INVALID: " + str);
            break;
        }
    }
}

void Adapter::handleCpuType(const TrkResult &result)
{
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 03 00  04 00 00 04 00 00 00]
    m_session.cpuMajor = result.data[1];
    m_session.cpuMinor = result.data[2];
    m_session.bigEndian = result.data[3];
    m_session.defaultTypeSize = result.data[4];
    m_session.fpTypeSize = result.data[5];
    m_session.extended1TypeSize = result.data[6];
    //m_session.extended2TypeSize = result.data[6];
    QString logMsg;
    QTextStream(&logMsg) << "HANDLE CPU TYPE: CPU=" << m_session.cpuMajor << '.'
        << m_session.cpuMinor << " bigEndian=" << m_session.bigEndian
        << " defaultTypeSize=" << m_session.defaultTypeSize
        << " fpTypeSize=" << m_session.fpTypeSize
        << " extended1TypeSize=" <<  m_session.extended1TypeSize;
    logMessage(logMsg);
}

void Adapter::setTrkBreakpoint(const Breakpoint &bp)
{
    //---IDE------------------------------------------------------
    //  Command: 0x1B Set Break
    //BreakType: 0x82
    //  Options: 0x00
    //  Address: 0x78674340 (2020033344)    i.e + 0x00000340
    //   Length: 0x00000001 (1)
    //    Count: 0x00000000 (0)
    //ProcessID: 0x000001b5 (437)
    // ThreadID: 0xffffffff (-1)
    // [1B 09 82 00 78 67 43 40 00 00 00 01 00 00 00 00
    //  00 00 01 B5 FF FF FF FF]
    const QByteArray ba = trkBreakpointMessage(m_session.codeseg + bp.offset,
        1, m_session.pid);
    sendTrkMessage(0x1B, TrkCB(handleSetTrkBreakpoint), ba);

    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 09 00 00 00 00 0A]
}

void Adapter::handleSetTrkBreakpoint(const TrkResult &result)
{
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 09 00 00 00 00 0A]
    const uint bpnr = extractInt(result.data.data());
    if (m_verbose)
        logMessage("SET BREAKPOINT " + hexxNumber(bpnr)
            + stringFromArray(result.data.data()));
}

void Adapter::handleCreateProcess(const TrkResult &result)
{
    //  40 00 00]
    //logMessage("       RESULT: " + result.toString());
    // [80 08 00   00 00 01 B5   00 00 01 B6   78 67 40 00   00 40 00 00]
    const char *data = result.data.data();
    m_session.pid = extractInt(data + 1);
    m_session.tid = extractInt(data + 5);
    m_session.codeseg = extractInt(data + 9);
    m_session.dataseg = extractInt(data + 13);

    logMessage("PID: " + hexxNumber(m_session.pid));
    logMessage("TID: " + hexxNumber(m_session.tid));
    logMessage("COD: " + hexxNumber(m_session.codeseg));
    logMessage("DAT: " + hexxNumber(m_session.dataseg));

    QByteArray ba;
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);

    startGdb();


#if 0
    //---IDE------------------------------------------------------
    //  Command: 0x42 Read Info
    //          [42 0C 00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F
    //  72 70 68 69 63 44 4C 4C 32 2E 64 6C 6C 00]
    sendTrkMessage(0x42, TrkCB(handleReadInfo),
        "00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F "
        "72 70 68 69 63 44 4C 4C 32 2E 64 6C 6C 00");
    //sendTrkMessage(0x42, TrkCB(handleReadInfo),
    //        "00 01 00 00 00 00");
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x20 Unspecified general OS-related error
    // [80 0C 20]


    //---IDE------------------------------------------------------
    //  Command: 0x42 Read Info
    // [42 0D 00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F
    //  72 70 68 69 63 44 4C 4C 31 2E 64 6C 6C 00]
    sendTrkMessage(0x42, TrkCB(handleReadInfo),
        "00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F "
        "72 70 68 69 63 44 4C 4C 31 2E 64 6C 6C 00");
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x20 Unspecified general OS-related error
    // [80 0D 20]
#endif

    //sendTrkMessage(0x18, TrkCB(handleStop),
    //    "01 " + formatInt(m_session.pid) + formatInt(m_session.tid));
}

void Adapter::handleReadRegisters(const TrkResult &result)
{
    logMessage("       RESULT: " + result.toString());
    // [80 0B 00   00 00 00 00   C9 24 FF BC   00 00 00 00   00
    //  60 00 00   00 00 00 00   78 67 79 70   00 00 00 00   00...]
    if (result.errorCode()) {
        logMessage("ERROR: " + result.errorString());
        return;
    }
    const char *data = result.data.data() + 1; // Skip ok byte
    for (int i = 0; i < RegisterCount; ++i)
        m_snapshot.registers[i] = extractInt(data + 4 * i);
} 

void Adapter::handleAndReportReadRegisters(const TrkResult &result)
{
    handleReadRegisters(result);
    QByteArray ba;
    for (int i = 0; i < 16; ++i) {
        const uint reg = m_registerEndianness == LittleEndian
            ? swapEndian(m_snapshot.registers[i]) : m_snapshot.registers[i];
        ba += hexNumber(reg, 8);
    }
    QByteArray logMsg = "REGISTER CONTENTS: ";
    if (m_verbose > 1) {
        for (int i = 0; i < RegisterCount; ++i) {
            logMsg += dumpRegister(i, m_snapshot.registers[i]);
            logMsg += ' ';
        }
    }
    sendGdbServerMessage(ba, logMsg);
}

static inline QString msgMemoryReadError(int code, uint addr, uint len = 0)
{
    const QString lenS = len ? QString::number(len) : QLatin1String("<unknown>");
    return QString::fromLatin1("Memory read error %1 at: 0x%2 %3")
        .arg(code).arg(addr, 0 ,16).arg(lenS);
}

void Adapter::handleReadMemoryBuffered(const TrkResult &result)
{
    if (extractShort(result.data.data() + 1) + 3 != result.data.size())
        logMessage("\n BAD MEMORY RESULT: " + result.data.toHex() + "\n");
    const uint blockaddr = result.cookie.toUInt();
    if (const int errorCode = result.errorCode()) {
        logMessage(msgMemoryReadError(errorCode, blockaddr));
        return;
    }
    const QByteArray ba = result.data.mid(3);
    m_snapshot.memory.insert(blockaddr, ba);
}

// Format log message for memory access with some smartness about registers
QByteArray Adapter::memoryReadLogMessage(uint addr, uint len, const QByteArray &ba) const
{
    QByteArray logMsg = "memory contents";
    if (m_verbose > 1) {
        logMsg += " addr: " + hexxNumber(addr);
        // indicate dereferencing of registers
        if (len == 4) {
            if (addr == m_snapshot.registers[RegisterPC]) {
                logMsg += "[PC]";
            } else if (addr == m_snapshot.registers[RegisterPSTrk]) {
                logMsg += "[PSTrk]";
            } else if (addr == m_snapshot.registers[RegisterSP]) {
                logMsg += "[SP]";
            } else if (addr == m_snapshot.registers[RegisterLR]) {
                logMsg += "[LR]";
            } else if (addr > m_snapshot.registers[RegisterSP] &&
                    (addr - m_snapshot.registers[RegisterSP]) < 10240) {
                logMsg += "[SP+"; // Stack area ...stack seems to be top-down
                logMsg += QByteArray::number(addr - m_snapshot.registers[RegisterSP]);
                logMsg += ']';
            }
        }
        logMsg += " length ";
        logMsg += QByteArray::number(len);
        logMsg += " :";
        logMsg += stringFromArray(ba, 16);
    }
    return logMsg;
}

void Adapter::reportReadMemoryBuffered(const TrkResult &result)
{
    const qulonglong cookie = result.cookie.toULongLong();
    const uint addr = cookie >> 32;
    const uint len = uint(cookie);

    // Gdb accepts less memory according to documentation.
    // Send E on complete failure.
    QByteArray ba;
    uint blockaddr = (addr / MemoryChunkSize) * MemoryChunkSize;
    for (; blockaddr < addr + len; blockaddr += MemoryChunkSize) {
        const Snapshot::Memory::const_iterator it = m_snapshot.memory.constFind(blockaddr);
        if (it == m_snapshot.memory.constEnd())
            break;
        ba.append(it.value());
    }
    const int previousChunkOverlap = addr % MemoryChunkSize;
    if (previousChunkOverlap != 0 && ba.size() > previousChunkOverlap)
        ba.remove(0, previousChunkOverlap);
    if (ba.size() > int(len))
        ba.truncate(len);

    if (ba.isEmpty()) {
        ba = "E20";
        sendGdbServerMessage(ba, msgMemoryReadError(32, addr, len).toLatin1());
    } else {
        sendGdbServerMessage(ba.toHex(), memoryReadLogMessage(addr, len, ba));
    }
}

void Adapter::handleReadMemoryUnbuffered(const TrkResult &result)
{
    //logMessage("UNBUFFERED MEMORY READ: " + stringFromArray(result.data));
    const uint blockaddr = result.cookie.toUInt();
    if (extractShort(result.data.data() + 1) + 3 != result.data.size())
        logMessage("\n BAD MEMORY RESULT: " + result.data.toHex() + "\n");
    if (const int errorCode = result.errorCode()) {
        const QByteArray ba = "E20";
        sendGdbServerMessage(ba, msgMemoryReadError(32, blockaddr).toLatin1());
    } else {
        const QByteArray ba = result.data.mid(3);
        sendGdbServerMessage(ba.toHex(), memoryReadLogMessage(blockaddr, ba.size(), ba));
    }
}

void Adapter::handleStepRange(const TrkResult &result)
{
    // [80 0f 00]
    if (result.errorCode()) {
        logMessage("ERROR: " + result.errorString());
        return;
    }
    logMessage("STEPPING FINISHED ");
    //sendGdbServerMessage("S05", "Stepping finished");
}

void Adapter::handleAndReportSetBreakpoint(const TrkResult &result)
{
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 09 00 00 00 00 0A]
    uint bpnr = extractByte(result.data.data());
    logMessage("SET BREAKPOINT " + hexxNumber(bpnr) + " "
         + stringFromArray(result.data.data()));
    sendGdbServerMessage("OK");
}

void Adapter::clearTrkBreakpoint(const Breakpoint &bp)
{
    //---IDE------------------------------------------------------
    //  Command: 0x1C Clear Break
    // [1C 25 00 00 00 0A 78 6A 43 40]
    QByteArray ba;
    appendByte(&ba, 0x00);
    appendShort(&ba, bp.number);
    appendInt(&ba, m_session.codeseg + bp.offset);
    sendTrkMessage(0x1C, TrkCB(handleClearBreakpoint), ba);
}

void Adapter::handleClearBreakpoint(const TrkResult &result)
{
    Q_UNUSED(result);
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 09 00 00 00 00 0A]
    logMessage("CLEAR BREAKPOINT ");
}

void Adapter::handleSignalContinue(const TrkResult &result)
{
    int signalNumber = result.cookie.toInt();
    logMessage("   HANDLE SIGNAL CONTINUE: " + stringFromArray(result.data));
    logMessage("NUMBER" + QString::number(signalNumber));
    sendGdbServerMessage("O" + QByteArray("Console output").toHex());
    sendGdbServerMessage("W81"); // "Process exited with result 1
}

void Adapter::handleWaitForFinished(const TrkResult &result)
{
    logMessage("   FINISHED: " + stringFromArray(result.data));
    //qApp->exit(1);
}

void Adapter::handleSupportMask(const TrkResult &result)
{
    const char *data = result.data.data();
    QByteArray str;
    for (int i = 0; i < 32; ++i) {
        //str.append("  [" + formatByte(data[i]) + "]: ");
        for (int j = 0; j < 8; ++j)
        if (data[i] & (1 << j))
            str.append(QByteArray::number(i * 8 + j, 16));
    }
    logMessage("SUPPORTED: " + str);
 }

void Adapter::handleTrkVersions(const TrkResult &result)
{
    QString logMsg;
    QTextStream str(&logMsg);
    str << "Versions: ";
    if (result.data.size() >= 5) {
        str << "Trk version " << int(result.data.at(1)) << '.'
            << int(result.data.at(2))
            << ", Protocol version " << int(result.data.at(3))
             << '.' << int(result.data.at(4));
    }
    logMessage(logMsg);
}

void Adapter::handleDisconnect(const TrkResult & /*result*/)
{
    logMessage(QLatin1String("Trk disconnected"), true);
}

void Adapter::cleanUp()
{
    //
    //---IDE------------------------------------------------------
    //  Command: 0x41 Delete Item
    //  Sub Cmd: Delete Process
    //ProcessID: 0x0000071F (1823)
    // [41 24 00 00 00 00 07 1F]
    logMessage(QString::fromLatin1("Cleanup PID=%1").arg(m_session.pid), true);
    if (!m_session.pid)
        return;

    QByteArray ba;
    appendByte(&ba, 0x00);
    appendByte(&ba, 0x00);
    appendInt(&ba, m_session.pid);
    sendTrkMessage(0x41, TrkCallback(), ba, "Delete process");

    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 24 00]

    foreach (const Breakpoint &bp, m_breakpoints)
        clearTrkBreakpoint(bp);

    sendTrkMessage(0x02, TrkCB(handleDisconnect));
    //---IDE------------------------------------------------------
    //  Command: 0x1C Clear Break
    // [1C 25 00 00 00 0A 78 6A 43 40]

        //---TRK------------------------------------------------------
        //  Command: 0xA1 Notify Deleted
        // [A1 09 00 00 00 00 00 00 00 00 07 1F]
        //---IDE------------------------------------------------------
        //  Command: 0x80 Acknowledge
        //    Error: 0x00
        // [80 09 00]

    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 25 00]

    //---IDE------------------------------------------------------
    //  Command: 0x1C Clear Break
    // [1C 26 00 00 00 0B 78 6A 43 70]
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 26 00]


    //---IDE------------------------------------------------------
    //  Command: 0x02 Disconnect
    // [02 27]
//    sendTrkMessage(0x02, TrkCB(handleDisconnect));
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    // Error: 0x00
}

void Adapter::readMemory(uint addr, uint len)
{
    Q_ASSERT(len < (2 << 16));

    // We try to get medium-sized chunks of data from the device
    if (m_verbose > 2)
        logMessage(QString::fromLatin1("readMemory %1 bytes from 0x%2 blocksize=%3")
            .arg(len).arg(addr, 0, 16).arg(MemoryChunkSize));

    if (m_bufferedMemoryRead) {
        uint blockaddr = (addr / MemoryChunkSize) * MemoryChunkSize;
        for (; blockaddr < addr + len; blockaddr += MemoryChunkSize) {
            if (!m_snapshot.memory.contains(blockaddr)) {
                if (m_verbose)
                    logMessage(QString::fromLatin1("Requesting buffered "
                        "memory %1 bytes from 0x%2")
                    .arg(MemoryChunkSize).arg(blockaddr, 0, 16));
                sendTrkMessage(0x10, TrkCB(handleReadMemoryBuffered),
                    trkReadMemoryMessage(blockaddr, MemoryChunkSize),
                    QVariant(blockaddr));
            }
        }
        const qulonglong cookie = (qulonglong(addr) << 32) + len;
        sendTrkMessage(TRK_WRITE_QUEUE_NOOP_CODE, TrkCB(reportReadMemoryBuffered),
            QByteArray(), cookie);
    } else {
        if (m_verbose)
            logMessage(QString::fromLatin1("Requesting unbuffered memory %1 "
                "bytes from 0x%2").arg(len).arg(addr, 0, 16));
        sendTrkMessage(0x10, TrkCB(handleReadMemoryUnbuffered),
           trkReadMemoryMessage(addr, len), QVariant(addr));
    }
}

void Adapter::interruptInferior()
{
    QByteArray ba;
    // stop the thread (2) or the process (1) or the whole system (0)
    // We choose 2, as 1 does not seem to work.
    appendByte(&ba, 2);
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid); // threadID: 4 bytes Variable number of bytes.
    sendTrkMessage(0x1a, TrkCallback(), ba, "Interrupting...");
}

void Adapter::connectProcess(QProcess *proc)
{
    connect(proc, SIGNAL(error(QProcess::ProcessError)),
        this, SLOT(handleProcError(QProcess::ProcessError)));
    connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)),
        this, SLOT(handleProcFinished(int, QProcess::ExitStatus)));
    connect(proc, SIGNAL(started()),
        this, SLOT(handleProcStarted()));
    connect(proc, SIGNAL(stateChanged(QProcess::ProcessState)),
        this, SLOT(handleProcStateChanged(QProcess::ProcessState)));
}

void Adapter::sendOutput(QObject *sender, const QString &data)
{
    if (sender)
        emit output(sender->objectName() + " : ", data);
    else
        emit output(QString(), data);
}

void Adapter::handleProcError(QProcess::ProcessError error)
{
    sendOutput(sender(), QString("Process Error %1").arg(error));
}

void Adapter::handleProcFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    sendOutput(sender(),
        QString("ProcessFinished %1 %2").arg(exitCode).arg(exitStatus));
}

void Adapter::handleProcStarted()
{
    sendOutput(sender(), QString("Process Started"));
}

void Adapter::handleProcStateChanged(QProcess::ProcessState newState)
{
    sendOutput(sender(), QString("Process State %1").arg(newState));
}

void Adapter::run()
{
    startInferior();
    sendOutput("### Starting Adapter");
    m_rfcommProc.start("rfcomm listen /dev/rfcomm0 1");
}

void Adapter::startGdb()
{
    if (!m_gdbServer.listen(QHostAddress(gdbServerIP()), gdbServerPort())) {
        logMessage(QString("Unable to start the gdb server at %1: %2.")
            .arg(m_gdbServerName).arg(m_gdbServer.errorString()), true);
        QCoreApplication::exit(5);
        return;
    }

    logMessage(QString("Gdb server running on %1.\nRegister endianness: %3.")
        .arg(m_gdbServerName).arg(m_registerEndianness), true);

    connect(&m_gdbServer, SIGNAL(newConnection()),
        this, SLOT(handleGdbConnection()));

    logMessage("STARTING GDB");
    QStringList gdbArgs;
    gdbArgs.append("--nx"); // Do not read .gdbinit file
    gdbArgs.append("-i");
    gdbArgs.append("mi");
    m_gdbProc.start(QDir::currentPath() + "/cs-gdb", gdbArgs);
    m_gdbProc.waitForStarted();

    sendGdbMessage("set confirm off"); // confirm potentially dangerous operations?
    sendGdbMessage("set endian little");
    sendGdbMessage("set remotebreak on");
    sendGdbMessage("set breakpoint pending on");
    sendGdbMessage("set trust-readonly-sections on");

    // FIXME: "remote noack" does not seem to be supported on cs-gdb?
    //sendGdbMessage("set remote noack-packet");

    // FIXME: creates a lot of noise a la  '&"putpkt: Junk: Ack " &'
    // even thouhg the communication seems sane
    //sendGdbMessage("set debug remote 1"); // creates l

    //sendGdbMessage("target remote " + m_gdbServerName);
//    sendGdbMessage("target extended-remote " + m_gdbServerName);
    //sendGdbMessage("target extended-async " + m_gdbServerName);
    //sendGdbMessage("set remotecache ...") // Set cache use for remote targets 
    //sendGdbMessage("file filebrowseapp.sym");
//    sendGdbMessage("add-symbol-file filebrowseapp.sym " + m_baseAddress);
//    sendGdbMessage("symbol-file filebrowseapp.sym");
//    sendGdbMessage("print E32Main");
//    sendGdbMessage("break E32Main");
    //sendGdbMessage("continue");
    //sendGdbMessage("info files");
    //sendGdbMessage("file filebrowseapp.sym -readnow");

    sendGdbMessage("add-symbol-file filebrowseapp.sym "
        + hexxNumber(m_session.codeseg));
    sendGdbMessage("symbol-file filebrowseapp.sym");

    // -symbol-info-address not implemented in cs-gdb 6.4-6.8 (at least)
    sendGdbMessage("info address E32Main", GdbCB(handleInfoMainAddress)); 
        
#if 1
    // FIXME: Gdb based version. That's the goal
    //sendGdbMessage("break E32Main");
    //sendGdbMessage("continue");
    QByteArray ba;
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    //sendTrkMessage(0x18, TrkCB(handleContinueAfterCreateProcess), 
    // trkContinueMessage(), "CONTINUE");
#else
    // Directly talk to TRK. Works for now...
    sendGdbMessage("break E32Main");
    sendGdbMessage("break filebrowseappui.cpp:39");
    //foreach (const Breakpoint &bp, m_breakpoints)
    //    setTrkBreakpoint(bp);
    //sendTrkContinue();
   //         sendTrkMessage(0x18, TrkCallback(), trkContinueMessage(), "CONTINUE");
#endif
}

void Adapter::sendGdbMessage(const QString &msg, GdbCallback callback,
    const QVariant &cookie)
{
    static int token = 0;
    ++token;
    GdbCommand data;
    data.command = msg;
    data.callback = callback;
    data.cookie = cookie;
    m_gdbCookieForToken[++token] = data;
    logMessage(QString("<- GDB: %1 %2").arg(token).arg(msg));
    m_gdbProc.write(QString("%1%2\n").arg(token).arg(msg).toLatin1());
}

void Adapter::handleGdbReadyReadStandardError()
{
    QByteArray ba = qobject_cast<QProcess *>(sender())->readAllStandardError();
    sendOutput(sender(), QString("stderr: %1").arg(QString::fromLatin1(ba)));
}

void Adapter::handleGdbReadyReadStandardOutput()
{
    QByteArray ba = qobject_cast<QProcess *>(sender())->readAllStandardOutput();
    QString str = QString::fromLatin1(ba);
    // FIXME: fragile. merge with gdbengine logic
#if 0
    QRegExp re(QString(".*([0-9]+)[^]done.*"));
    int pos = re.indexIn(str);
    if (pos == -1) {
        logMessage(QString("\n-> GDB: %1 %**% %2 %**%\n").arg(str).arg(pos));
        return;
    }
    int token = re.cap(1).toInt();
    logMessage(QString("\n-> GDB: %1 %2##\n").arg(token).arg(QString::fromLatin1(ba)));
    if (!token)
        return;
    GdbCommand cmd = m_gdbCookieForToken.take(token);
    logMessage("FOUND CALLBACK FOR " + cmd.command);
    GdbResult result;
    result.data = ba;
    if (!cmd.callback.isNull())
        cmd.callback(result);
#else
    QRegExp re(QString("Symbol .._Z7E32Mainv.. is a function at address 0x(.*)\\."));
    if (re.indexIn(str) == -1) {
        logMessage(QString("-> GDB: %1").arg(str));
        return;
    }
    logMessage(QString("-> GDB MAIN: %1").arg(re.cap(1)));
    GdbResult result;
    result.data = re.cap(1).toLatin1();
    handleInfoMainAddress(result);
#endif
}

void Adapter::handleInfoMainAddress(const GdbResult &result)
{
    bool ok;
    uint addr = result.data.toInt(&ok, 16);
    const QByteArray ba = trkBreakpointMessage(addr, 1);
    sendTrkMessage(0x1B, TrkCB(handleSetTrkMainBreakpoint), ba);
}

void Adapter::handleSetTrkMainBreakpoint(const TrkResult &result)
{
    //---TRK------------------------------------------------------
    // [80 09 00 00 00 00 0A]
    const uint bpnr = extractInt(result.data.data());
    logMessage("SET MAIN BREAKPOINT " + hexxNumber(bpnr)
        + stringFromArray(result.data.data()));

    // 'continue after initial break'
    QByteArray ba;
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    sendTrkMessage(0x18, TrkCallback(), ba, "CONTINUE");

    // this attaches
    sendGdbMessage("target remote " + m_gdbServerName);
}

void Adapter::handleInfoAddress(const GdbResult &result)
{
    Q_UNUSED(result);
    // FIXME
}

void Adapter::handleRfcommReadyReadStandardError()
{
    QByteArray ba = qobject_cast<QProcess *>(sender())->readAllStandardError();
    sendOutput(sender(), QString("stderr: %1").arg(QString::fromLatin1(ba)));
}

void Adapter::handleRfcommReadyReadStandardOutput()
{
    QByteArray ba = qobject_cast<QProcess *>(sender())->readAllStandardOutput();
    sendOutput(sender(), QString("stdout: %1").arg(QString::fromLatin1(ba)));
}

///////////////////////////////////////////////////////////////////////
//
// RunnerGui
//
///////////////////////////////////////////////////////////////////////

class RunnerGui : public QTextEdit
{
    Q_OBJECT

public:
    RunnerGui(Adapter *adapter);
    void keyPressEvent(QKeyEvent *ev);

private slots:
    void handleOutput(const QString &senderName, const QString &data);

private:
    Adapter *m_adapter;
};

RunnerGui::RunnerGui(Adapter *adapter)
    : m_adapter(adapter) 
{
    resize(1200, 1000);
    connect(adapter, SIGNAL(output(QString,QString)),
        this, SLOT(handleOutput(QString,QString)));
}

void RunnerGui::handleOutput(const QString &senderName, const QString &data)
{
    append(senderName + data);
    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::End);
    setTextCursor(tc);
/*
    QString str = data;
    int pos1 = str.indexOf("#");
    int pos2 = str.indexOf(")", pos1);
    if (pos1 != -1 && pos2 != -1)
        str = str.left(pos1) + "<b>" + str.mid(pos1, pos2 - pos1 + 1)
            + "</b> " + str.mid(pos2 + 1);
    str.replace("\\t", QString(QChar(0x09)));
    str.replace("\\n", QString("\n"));
    insertHtml(str + "\n");
*/
    setCurrentCharFormat(QTextCharFormat());
    ensureCursorVisible();

}

void RunnerGui::keyPressEvent(QKeyEvent *ev)
{
    if (ev->modifiers() == Qt::ControlModifier && ev->key() == Qt::Key_Return)
        m_adapter->executeCommand(textCursor().block().text());
    else
        QTextEdit::keyPressEvent(ev);
}

///////////////////////////////////////////////////////////////////////
//
// main
//
///////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Adapter adapter;
    RunnerGui gui(&adapter);
    gui.show();
    QTimer::singleShot(0, &adapter, SLOT(run()));
    return app.exec();
}

#include "runner.moc"
