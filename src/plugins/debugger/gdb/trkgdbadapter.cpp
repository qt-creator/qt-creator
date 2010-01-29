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

#include "trkgdbadapter.h"
#include "launcher.h"
#include "trkoptions.h"
#include "trkoptionspage.h"
#include "s60debuggerbluetoothstarter.h"
#include "bluetoothlistener_gui.h"

#include "debuggeractions.h"
#include "debuggerstringutils.h"
#ifndef STANDALONE_RUNNER
#include "gdbengine.h"
#endif

#include <utils/qtcassert.h>

#include <QtCore/QTimer>
#include <QtCore/QDir>

#ifdef Q_OS_WIN
#  include <windows.h>
#else
#  include <sys/types.h>
#  include <unistd.h>
#endif

#define CB(callback) \
    static_cast<GdbEngine::AdapterCallback>(&TrkGdbAdapter::callback), \
    STRINGIFY(callback)

#define TrkCB(s) TrkCallback(this, &TrkGdbAdapter::s)

//#define DEBUG_MEMORY  1
#if DEBUG_MEMORY
#   define MEMORY_DEBUG(s) qDebug() << s
#else
#   define MEMORY_DEBUG(s)
#endif
#define MEMORY_DEBUGX(s) qDebug() << s

using namespace trk;

namespace Debugger {
namespace Internal {

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

static QByteArray dumpRegister(uint n, uint value)
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

static void appendRegister(QByteArray *ba, uint regno, uint value)
{
    ba->append(hexNumber(regno, 2));
    ba->append(':');
    ba->append(hexNumber(swapEndian(value), 8));
    ba->append(';');
}

QDebug operator<<(QDebug d, MemoryRange range)
{
    return d << QString("[%1,%2] (size %3) ")
        .arg(range.from, 0, 16).arg(range.to, 0, 16).arg(range.size());
}

///////////////////////////////////////////////////////////////////////////
//
// MemoryRange
//
///////////////////////////////////////////////////////////////////////////

MemoryRange::MemoryRange(uint f, uint t)
    : from(f), to(t)
{
    QTC_ASSERT(f <= t, /**/);
}

bool MemoryRange::intersects(const MemoryRange &other) const
{
    Q_UNUSED(other);
    QTC_ASSERT(false, /**/);
    return false; // FIXME
}

void MemoryRange::operator-=(const MemoryRange &other)
{
    if (from == 0 && to == 0)
        return;
    MEMORY_DEBUG("      SUB: "  << *this << " - " << other);
    if (other.from <= from && to <= other.to) {
        from = to = 0;
        return;
    }
    if (other.from <= from && other.to <= to) {
        from = qMax(from, other.to);
        return;
    }
    if (from <= other.from && to <= other.to) {
        to = qMin(other.from, to);
        return;
    }
    // This would split the range.
    QTC_ASSERT(false, qDebug() << "Memory::operator-() not handled for: "
        << *this << " - " << other);
}

///////////////////////////////////////////////////////////////////////////
//
// Snapshot
//
///////////////////////////////////////////////////////////////////////////

void Snapshot::reset()
{
    memory.clear();
    for (int i = 0; i < RegisterCount; ++i)
        registers[i] = 0;
    registerValid = false;
    wantedMemory = MemoryRange();
}

void Snapshot::insertMemory(const MemoryRange &range, const QByteArray &ba)
{
    QTC_ASSERT(range.size() == uint(ba.size()),
        qDebug() << "RANGE: " << range << " BA SIZE: " << ba.size(); return);
    
    MEMORY_DEBUG("INSERT: " << range);
    // Try to combine with existing chunk.
    Snapshot::Memory::iterator it = memory.begin();
    Snapshot::Memory::iterator et = memory.end();
    for ( ; it != et; ++it) {
        if (range.from == it.key().to) {
            MEMORY_DEBUG("COMBINING " << it.key() << " AND " << range);
            QByteArray data = *it;
            data.append(ba);
            const MemoryRange res(it.key().from, range.to);
            memory.remove(it.key());
            MEMORY_DEBUG(" TO(1)  " << res);
            insertMemory(res, data);
            return;
        }
        if (it.key().from == range.to) {
            MEMORY_DEBUG("COMBINING " << range << " AND " << it.key());
            QByteArray data = ba;
            data.append(*it);
            const MemoryRange res(range.from, it.key().to);
            memory.remove(it.key());
            MEMORY_DEBUG(" TO(2)  " << res);
            insertMemory(res, data);
            return;
        }
    }

    // Not combinable, add chunk.
    memory.insert(range, ba);
}

///////////////////////////////////////////////////////////////////////////
//
// TrkGdbAdapter
//
///////////////////////////////////////////////////////////////////////////

TrkGdbAdapter::TrkGdbAdapter(GdbEngine *engine, const TrkOptionsPtr &options) :
    AbstractGdbAdapter(engine),
    m_options(options),
    m_overrideTrkDeviceType(-1),
    m_running(false),
    m_trkDevice(new trk::TrkDevice),
    m_gdbAckMode(true),
    m_verbose(0)
{
    m_bufferedMemoryRead = true;
    // Disable buffering if gdb's dcache is used.
    m_bufferedMemoryRead = false;

    m_gdbServer = 0;
    m_gdbConnection = 0;
    m_snapshot.reset();
#ifdef Q_OS_WIN
    const DWORD portOffset = GetCurrentProcessId() % 100;
#else
    const uid_t portOffset = getuid();
#endif
    m_gdbServerName = _("127.0.0.1:%1").arg(2222 + portOffset);

    connect(m_trkDevice.data(), SIGNAL(messageReceived(trk::TrkResult)),
        this, SLOT(handleTrkResult(trk::TrkResult)));
    connect(m_trkDevice.data(), SIGNAL(error(QString)),
        this, SLOT(handleTrkError(QString)));

    setVerbose(theDebuggerBoolSetting(VerboseLog));
    m_trkDevice->setSerialFrame(effectiveTrkDeviceType() != TrkOptions::BlueTooth);

    connect(m_trkDevice.data(), SIGNAL(logMessage(QString)),
        this, SLOT(trkLogMessage(QString)));
    connect(theDebuggerAction(VerboseLog), SIGNAL(valueChanged(QVariant)),
        this, SLOT(setVerbose(QVariant)));
}

TrkGdbAdapter::~TrkGdbAdapter()
{
    cleanup();
    logMessage("Shutting down.\n");
}

void TrkGdbAdapter::setVerbose(const QVariant &value)
{
    setVerbose(value.toInt());
}

void TrkGdbAdapter::setVerbose(int verbose)
{
    m_verbose = verbose;
    m_trkDevice->setVerbose(m_verbose);
}

QString TrkGdbAdapter::effectiveTrkDevice() const
{
    if (!m_overrideTrkDevice.isEmpty())
        return m_overrideTrkDevice;
    if (m_options->mode == TrkOptions::BlueTooth)
        return m_options->blueToothDevice;
    return m_options->serialPort;
}

int TrkGdbAdapter::effectiveTrkDeviceType() const
{
    if (m_overrideTrkDeviceType >= 0)
        return m_overrideTrkDeviceType;
    return m_options->mode;
}

void TrkGdbAdapter::trkLogMessage(const QString &msg)
{
    logMessage("TRK " + msg);
}

void TrkGdbAdapter::setGdbServerName(const QString &name)
{
    m_gdbServerName = name;
}

QString TrkGdbAdapter::gdbServerIP() const
{
    int pos = m_gdbServerName.indexOf(':');
    if (pos == -1)
        return m_gdbServerName;
    return m_gdbServerName.left(pos);
}

uint TrkGdbAdapter::gdbServerPort() const
{
    int pos = m_gdbServerName.indexOf(':');
    if (pos == -1)
        return 0;
    return m_gdbServerName.mid(pos + 1).toUInt();
}

QByteArray TrkGdbAdapter::trkContinueMessage()
{
    QByteArray ba;
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    return ba;
}

QByteArray TrkGdbAdapter::trkReadRegistersMessage()
{
    QByteArray ba;
    appendByte(&ba, 0); // Register set, only 0 supported
    appendShort(&ba, 0);
    appendShort(&ba, RegisterCount - 1); // last register
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    return ba;
}

QByteArray TrkGdbAdapter::trkWriteRegisterMessage(byte reg, uint value)
{
    QByteArray ba;
    appendByte(&ba, 0); // ?
    appendShort(&ba, reg);
    appendShort(&ba, reg);
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    appendInt(&ba, value);
    return ba;
}

QByteArray TrkGdbAdapter::trkReadMemoryMessage(uint from, uint len)
{
    QByteArray ba;
    ba.reserve(11);
    appendByte(&ba, 0x08); // Options, FIXME: why?
    appendShort(&ba, len);
    appendInt(&ba, from);
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    return ba;
}

QByteArray TrkGdbAdapter::trkReadMemoryMessage(const MemoryRange &range)
{
    return trkReadMemoryMessage(range.from, range.size());
}

QByteArray TrkGdbAdapter::trkWriteMemoryMessage(uint addr, const QByteArray &data)
{
    QByteArray ba;
    ba.reserve(11 + data.size());
    appendByte(&ba, 0x08); // Options, FIXME: why?
    appendShort(&ba, data.size());
    appendInt(&ba, addr);
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    ba.append(data);
    return ba;
}

QByteArray TrkGdbAdapter::trkStepRangeMessage(byte option)
{
    QByteArray ba;
    ba.reserve(17);
    appendByte(&ba, option);
    //qDebug() << "STEP ON " << hexxNumber(m_snapshot.registers[RegisterPC]);
    appendInt(&ba, m_snapshot.registers[RegisterPC]); // Start address
    appendInt(&ba, m_snapshot.registers[RegisterPC]); // End address
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    return ba;
}

QByteArray TrkGdbAdapter::trkDeleteProcessMessage()
{
    QByteArray ba;
    ba.reserve(6);
    appendByte(&ba, 0); // ?
    appendByte(&ba, 0); // Sub-command: Delete Process
    appendInt(&ba, m_session.pid);
    return ba;
}

QByteArray TrkGdbAdapter::trkInterruptMessage()
{
    QByteArray ba;
    ba.reserve(9);
    // Stop the thread (2) or the process (1) or the whole system (0).
    // We choose 2, as 1 does not seem to work.
    appendByte(&ba, 2);
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid); // threadID: 4 bytes Variable number of bytes.
    return ba;
}

void TrkGdbAdapter::emitDelayedInferiorStartFailed(const QString &msg)
{
    m_adapterFailMessage = msg;
    QTimer::singleShot(0, this, SLOT(slotEmitDelayedInferiorStartFailed()));
}

void TrkGdbAdapter::slotEmitDelayedInferiorStartFailed()
{
    emit inferiorStartFailed(m_adapterFailMessage);
}


void TrkGdbAdapter::logMessage(const QString &msg)
{
    if (m_verbose)
        debugMessage("TRK LOG: " + msg);
}

//
// Gdb
//
void TrkGdbAdapter::handleGdbConnection()
{
    logMessage("HANDLING GDB CONNECTION");
    QTC_ASSERT(m_gdbConnection == 0, /**/);
    m_gdbConnection = m_gdbServer->nextPendingConnection();
    QTC_ASSERT(m_gdbConnection, return);
    connect(m_gdbConnection, SIGNAL(disconnected()),
            m_gdbConnection, SLOT(deleteLater()));
    connect(m_gdbConnection, SIGNAL(readyRead()),
            this, SLOT(readGdbServerCommand()));
}

static inline QString msgGdbPacket(const QString &p)
{
    return QLatin1String("gdb:                              ") + p;
}

void TrkGdbAdapter::readGdbServerCommand()
{
    QTC_ASSERT(m_gdbConnection, return);
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
            // This seems too harsh.
            //emit adapterCrashed("Communication problem encountered.");
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

bool TrkGdbAdapter::sendGdbServerPacket(const QByteArray &packet, bool doFlush)
{
    if (!m_gdbConnection) {
        logMessage(_("Cannot write to gdb: No connection (%1)")
            .arg(_(packet)));
        return false;
    }
    if (m_gdbConnection->state() != QAbstractSocket::ConnectedState) {
        logMessage(_("Cannot write to gdb: Not connected (%1)")
            .arg(_(packet)));
        return false;
    }
    if (m_gdbConnection->write(packet) == -1) {
        logMessage(_("Cannot write to gdb: %1 (%2)")
            .arg(m_gdbConnection->errorString()).arg(_(packet)));
        return false;
    }
    if (doFlush)
        m_gdbConnection->flush();
    return true;
}

void TrkGdbAdapter::sendGdbServerAck()
{
    if (!m_gdbAckMode)
        return;
    logMessage("gdb: <- +");
    sendGdbServerPacket("+", false);
}

void TrkGdbAdapter::sendGdbServerMessage(const QByteArray &msg, const QByteArray &logNote)
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

void TrkGdbAdapter::sendGdbServerMessageAfterTrkResponse(const QByteArray &msg,
    const QByteArray &logNote)
{
    QByteArray ba = msg + char(1) + logNote;
    sendTrkMessage(TRK_WRITE_QUEUE_NOOP_CODE, TrkCB(reportToGdb), "", ba); // Answer gdb
}

void TrkGdbAdapter::reportToGdb(const TrkResult &result)
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

QByteArray TrkGdbAdapter::trkBreakpointMessage(uint addr, uint len, bool armMode)
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

void TrkGdbAdapter::handleGdbServerCommand(const QByteArray &cmd)
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
        uint signalNumber = cmd.mid(1).toUInt(&ok, 16);
        QByteArray ba;
        appendInt(&ba, m_session.pid);
        appendInt(&ba, m_session.tid);
        sendTrkMessage(0x18, TrkCB(handleSignalContinue), ba, signalNumber);
    }

    else if (cmd.startsWith("D")) {
        sendGdbServerAck();
        sendGdbServerMessage("OK", "shutting down");
    }

    else if (cmd == "g") {
        // Read general registers.
        if (m_snapshot.registerValid) {
            //qDebug() << "Using cached register contents";
            logMessage(msgGdbPacket(QLatin1String("Read registers")));
            sendGdbServerAck();
            reportRegisters();
        } else {
            //qDebug() << "Fetching register contents";
            sendGdbServerAck();
            sendTrkMessage(0x12,
                TrkCB(handleAndReportReadRegisters),
                trkReadRegistersMessage());
        }
    }

    else if (cmd == "gg") {
        // Force re-reading general registers for debugging purpose.
        sendGdbServerAck();
        m_snapshot.registerValid = false;
        sendTrkMessage(0x12,
            TrkCB(handleAndReportReadRegisters),
            trkReadRegistersMessage());
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
        m_session.currentThread = cmd.mid(2).toUInt(0, 16);
        sendGdbServerMessage("OK", "Set current thread "
            + QByteArray::number(m_session.currentThread));
    }

    else if (cmd == "k" || cmd.startsWith("vKill")) {
        // Kill inferior process
        logMessage(msgGdbPacket(QLatin1String("kill")));
        sendTrkMessage(0x41, TrkCB(handleDeleteProcess),
            trkDeleteProcessMessage(), "Delete process"); 
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
            readMemory(addr, len, m_bufferedMemoryRead);
        } else {
            sendGdbServerMessage("E20", "Error " + cmd);
        }
    }

    else if (cmd.startsWith("p")) {
        logMessage(msgGdbPacket(QLatin1String("read register")));
        // 0xf == current instruction pointer?
        //sendGdbServerMessage("0000", "current IP");
        sendGdbServerAck();
        bool ok = false;
        const uint registerNumber = cmd.mid(1).toUInt(&ok, 16);
        if (m_snapshot.registerValid) {
            QByteArray logMsg = "Read Register";
            if (registerNumber == RegisterPSGdb) {
                QByteArray ba;
                appendInt(&ba, m_snapshot.registers[RegisterPSTrk], LittleEndian);
                logMsg += dumpRegister(registerNumber, m_snapshot.registers[RegisterPSTrk]);
                sendGdbServerMessage(ba.toHex(), logMsg);
            } else if (registerNumber < 16) {
                QByteArray ba;
                appendInt(&ba, m_snapshot.registers[registerNumber], LittleEndian);
                logMsg += dumpRegister(registerNumber, m_snapshot.registers[registerNumber]);
                sendGdbServerMessage(ba.toHex(), logMsg);
            } else {
                sendGdbServerMessage("0000", "read single unknown register #"
                    + QByteArray::number(registerNumber));
                //sendGdbServerMessage("E01", "read single unknown register");
            }
        } else {
            //qDebug() << "Fetching single register";
            sendTrkMessage(0x12,
                TrkCB(handleAndReportReadRegister),
                trkReadRegistersMessage(), registerNumber);
        }
    }

    else if (cmd.startsWith("P")) {
        logMessage(msgGdbPacket(QLatin1String("write register")));
        // $Pe=70f96678#d3
        sendGdbServerAck();
        int pos = cmd.indexOf('=');
        QByteArray regName = cmd.mid(1, pos - 1);
        QByteArray valueName = cmd.mid(pos + 1);
        bool ok = false;
        const uint registerNumber = regName.toUInt(&ok, 16);
        const uint value = swapEndian(valueName.toUInt(&ok, 16));
        // FIXME: Assume all goes well.
        m_snapshot.registers[registerNumber] = value;
        QByteArray ba = trkWriteRegisterMessage(registerNumber, value);
        sendTrkMessage(0x13, TrkCB(handleWriteRegister), ba, "Write register");
        // Note that App TRK refuses to write registers 13 and 14
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
        sendGdbServerMessage(
            "PacketSize=7cf;"
            "QPassSignals+;"
            "QStartNoAckMode+;"
            "qXfer:libraries:read+;"
            //"qXfer:auxv:read+;"
            "qXfer:features:read+");
    }

    else if (cmd.startsWith("qThreadExtraInfo")) {
        // $qThreadExtraInfo,1f9#55
        sendGdbServerAck();
        sendGdbServerMessage(QByteArray("Nothing special").toHex());
    }

    else if (cmd == "qfDllInfo") {
        // That's the _first_ query package.
        // Happens with  gdb 6.4.50.20060226-cvs / CodeSourcery.
        // Never made it into FSF gdb that got qXfer:libraries:read instead.
        // http://sourceware.org/ml/gdb/2007-05/msg00038.html
        // Name=hexname,TextSeg=textaddr[,DataSeg=dataaddr]
        sendGdbServerAck();
        if (!m_session.libraries.isEmpty()) {
            QByteArray response = "m";
            // FIXME: Limit packet length by using qsDllInfo packages?
            for (int i = 0; i != m_session.libraries.size(); ++i) {
                if (i)
                    response += ';';
                const Library &lib = m_session.libraries.at(i);
                response += "Name=" + lib.name.toHex()
                            + ",TextSeg=" + hexNumber(lib.codeseg)
                            + ",DataSeg=" + hexNumber(lib.dataseg);
            }
            sendGdbServerMessage(response, "library information transferred");
        } else {
            sendGdbServerMessage("l", "library information transfer finished");
        }
    }

    else if (cmd == "qsDllInfo") {
        // That's a following query package
        sendGdbServerAck();
        sendGdbServerMessage("l", "library information transfer finished");
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
            sendGdbServerMessage("qSymbol:" + QByteArray("_Z7E32Mainv").toHex(),
                "ask for more");
    }

    else if (cmd.startsWith("qXfer:features:read:target.xml:")) {
        //  $qXfer:features:read:target.xml:0,7ca#46...Ack
        sendGdbServerAck();
        //sendGdbServerMessage("l<target><architecture>symbianelf</architecture></target>");
        sendGdbServerMessage("l<target><architecture>arm</architecture></target>");
        //sendGdbServerMessage("l<target><architecture>arm-none-symbianelf</architecture></target>");
    }

    else if (cmd == "qfThreadInfo") {
        // That's the _first_ query package.
        sendGdbServerAck();
        if (!m_session.threads.isEmpty()) {
            QByteArray response = "m";
            // FIXME: Limit packet length by using qsThreadInfo packages?
            qDebug()  << "CURRENT THREAD: " << m_session.tid;
            response += hexNumber(m_session.tid);
            sendGdbServerMessage(response, "thread information transferred");
        } else {
            sendGdbServerMessage("l", "thread information transfer finished");
        }
    }

    else if (cmd == "qsThreadInfo") {
        // That's a following query package
        sendGdbServerAck();
        sendGdbServerMessage("l", "thread information transfer finished");
    }

    else if (cmd.startsWith("qXfer:libraries:read")) {
        //qDebug() << "COMMAND: " << cmd;
        sendGdbServerAck();
        QByteArray response = "l<library-list>";
        for (int i = 0; i != m_session.libraries.size(); ++i) {
            const Library &lib = m_session.libraries.at(i);
            response += "<library name=\"" + lib.name + "\">";
            //response += "<segment address=\"0x" + hexNumber(lib.codeseg) + "\"/>";
            response += "<section address=\"0x" + hexNumber(lib.codeseg) + "\"/>";
            response += "<section address=\"0x" + hexNumber(lib.dataseg) + "\"/>";
            response += "<section address=\"0x" + hexNumber(lib.dataseg) + "\"/>";
            response += "</library>";
        }
        response += "</library-list>";
        sendGdbServerMessage(response, "library information transferred");
    }

    else if (cmd == "QStartNoAckMode") {
        //$qSupported#37
        logMessage("Handling 'QStartNoAckMode'");
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
        //m_snapshot.reset();
        m_running = true;
        QByteArray ba = trkStepRangeMessage(0x01);  // options "step into"
        sendTrkMessage(0x19, TrkCB(handleStepInto), ba, "Step range");
    }

    else if (cmd.startsWith('T')) {
        // FIXME: check whether thread is alive
        sendGdbServerAck();
        sendGdbServerMessage("OK"); // pretend all is well
        //sendGdbServerMessage("E nn");
    }

    else if (cmd == "vCont?") {
        // actions supported by the vCont packet
        sendGdbServerAck();
        //sendGdbServerMessage("OK"); // we don't support vCont.
        sendGdbServerMessage("vCont;c;C;s;S");
    }

    else if (cmd == "vCont;c") {
        // vCont[;action[:thread-id]]...'
        sendGdbServerAck();
        //m_snapshot.reset();
        m_running = true;
        sendTrkMessage(0x18, TrkCallback(), trkContinueMessage(), "CONTINUE");
    }

    else if (cmd.startsWith("Z0,") || cmd.startsWith("Z1,")) {
        // Insert breakpoint
        sendGdbServerAck();
        logMessage(msgGdbPacket(QLatin1String("Insert breakpoint")));
        // $Z0,786a4ccc,4#99
        const int pos = cmd.lastIndexOf(',');
        bool ok1 = false;
        bool ok2 = false;
        const uint addr = cmd.mid(3, pos - 3).toUInt(&ok1, 16);
        const uint len = cmd.mid(pos + 1).toUInt(&ok2, 16);
        if (!ok1) {
            logMessage("MISPARSED ADDRESS FROM " + cmd +
                " (" + cmd.mid(3, pos - 3) + ")");
        } else if (!ok2) {
            logMessage("MISPARSED BREAKPOINT SIZE FROM " + cmd);
        } else {
            //qDebug() << "ADDR: " << hexNumber(addr) << " LEN: " << len;
            logMessage(_("Inserting breakpoint at 0x%1, %2")
                .arg(addr, 0, 16).arg(len));
            const QByteArray ba = trkBreakpointMessage(addr, len, len == 4);
            sendTrkMessage(0x1B, TrkCB(handleAndReportSetBreakpoint), ba, addr);
        }
    }

    else if (cmd.startsWith("z0,") || cmd.startsWith("z1,")) {
        // Remove breakpoint
        sendGdbServerAck();
        logMessage(msgGdbPacket(QLatin1String("Remove breakpoint")));
        // $z0,786a4ccc,4#99
        const int pos = cmd.lastIndexOf(',');
        bool ok = false;
        const uint addr = cmd.mid(3, pos - 3).toUInt(&ok, 16);
        const uint len = cmd.mid(pos + 1).toUInt(&ok, 16);
        const uint bp = m_session.addressToBP[addr];
        if (bp == 0) {
            logMessage(_("NO RECORDED BP AT 0x%1, %2")
                .arg(addr, 0, 16).arg(len));
            sendGdbServerMessage("E00");
        } else {
            m_session.addressToBP.remove(addr);
            QByteArray ba;
            appendInt(&ba, bp);
            sendTrkMessage(0x1C, TrkCB(handleClearBreakpoint), ba, addr);
        }
    }

    else if (cmd.startsWith("qPart:") || cmd.startsWith("qXfer:"))  {
        QByteArray data = cmd.mid(1 + cmd.indexOf(':'));
        // "qPart:auxv:read::0,147": Read OS auxiliary data (see info aux)
        bool handled = false;
        if (data.startsWith("auxv:read::")) {
            const int offsetPos = data.lastIndexOf(':') + 1;
            const int commaPos = data.lastIndexOf(',');
            if (commaPos != -1) {                
                bool ok1 = false, ok2 = false;
                const int offset = data.mid(offsetPos,  commaPos - offsetPos)
                    .toUInt(&ok1, 16);
                const int length = data.mid(commaPos + 1).toUInt(&ok2, 16);
                if (ok1 && ok2) {
                    const QString msg = _("Read of OS auxiliary "
                        "vector (%1, %2) not implemented.").arg(offset).arg(length);
                    logMessage(msgGdbPacket(msg));
                    sendGdbServerMessage("E20", msg.toLatin1());
                    handled = true;
                }
            }
        } // auxv read

        if (!handled) {
            const QString msg = QLatin1String("FIXME unknown 'XFER'-request: ")
                + QString::fromAscii(cmd);
            logMessage(msgGdbPacket(msg));
            sendGdbServerMessage("E20", msg.toLatin1());
        }
    } // qPart/qXfer
    else {
        logMessage(msgGdbPacket(QLatin1String("FIXME unknown: ")
            + QString::fromAscii(cmd)));
    }
}

void TrkGdbAdapter::sendTrkMessage(byte code, TrkCallback callback,
    const QByteArray &data, const QVariant &cookie)
{
    m_trkDevice->sendTrkMessage(code, callback, data, cookie);
}

void TrkGdbAdapter::sendTrkAck(byte token)
{
    //logMessage(QString("SENDING ACKNOWLEDGEMENT FOR TOKEN %1").arg(int(token)));
    m_trkDevice->sendTrkAck(token);
}

void TrkGdbAdapter::handleTrkError(const QString &msg)
{
    logMessage("## TRK ERROR: " + msg);
}

void TrkGdbAdapter::handleTrkResult(const TrkResult &result)
{
    if (result.isDebugOutput) {
        // It looks like those messages _must not_ be acknowledged.
        // If we do so, TRK will complain about wrong sequencing.
        //sendTrkAck(result.token);
        logMessage(QLatin1String("APPLICATION OUTPUT: ") +
            QString::fromAscii(result.data));
        sendGdbServerMessage("O" + result.data.toHex());
        return;
    }
    //logMessage("READ TRK " + result.toString());
    QByteArray prefix = "READ BUF:                                       ";
    QByteArray str = result.toString().toUtf8();
    switch (result.code) {
        case 0x80: // ACK
            break;
        case 0xff: { // NAK. This mostly means transmission error, not command failed.
            QString logMsg;
            QTextStream(&logMsg) << prefix << "NAK: for token=" << result.token
                << " ERROR: " << errorMessage(result.data.at(0)) << ' ' << str;
            logMessage(logMsg);
            break;
        }
        case TrkNotifyStopped: {  // 0x90 Notified Stopped
            // 90 01   78 6a 40 40   00 00 07 23   00 00 07 24  00 00
            debugMessage(_("RESET SNAPSHOT (NOTIFY STOPPED)"));
            m_snapshot.reset();
            QString reason;
            uint addr;
            uint pid;
            uint tid;
            trk::Launcher::parseNotifyStopped(result.data, &pid, &tid, &addr, &reason);
            const QString msg = trk::Launcher::msgStopped(pid, tid, addr, reason);
            logMessage(prefix + msg);
            m_engine->manager()->showDebuggerOutput(LogMisc, msg);
            sendTrkAck(result.token);
            if (addr) {
                // Todo: Do not send off GdbMessages if a synced gdb
                // query is pending, queue instead
                if (m_running) {
                    m_running = false;
                }
            } else {
                logMessage(QLatin1String("Ignoring stop at 0"));
            }

            #if 1
            // We almost always need register values, so get them
            // now before informing gdb about the stop.s
            //qDebug() << "Auto-fetching registers";
            sendTrkMessage(0x12,
                TrkCB(handleAndReportReadRegistersAfterStop),
                trkReadRegistersMessage());
            #else
            // As a source-line step typically consists of
            // several instruction steps, better avoid the multiple
            // roundtrips through TRK in favour of an additional
            // roundtrip through gdb. But gdb will ask for all registers.
            //sendGdbServerMessage("S05", "Target stopped");
            // -- or --
            QByteArray ba = "T05";
            appendRegister(&ba, RegisterPSGdb, addr);
            sendGdbServerMessage(ba, "Registers");
            #endif
            break;
        }
        case TrkNotifyException: { // 0x91 Notify Exception (obsolete)
            debugMessage(_("RESET SNAPSHOT (NOTIFY EXCEPTION)"));
            m_snapshot.reset();
            logMessage(prefix + "NOTE: EXCEPTION  " + str);
            sendTrkAck(result.token);
            break;
        }
        case 0x92: { //
            debugMessage(_("RESET SNAPSHOT (NOTIFY INTERNAL ERROR)"));
            m_snapshot.reset();
            logMessage(prefix + "NOTE: INTERNAL ERROR: " + str);
            sendTrkAck(result.token);
            break;
        }

        // target->host OS notification
        case 0xa0: { // Notify Created
            // Sending this ACK does not seem to make a difference. Why?
            //sendTrkAck(result.token);
            debugMessage(_("RESET SNAPSHOT (NOTIFY CREATED)"));
            m_snapshot.reset();
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
            Library lib;
            lib.name = name;
            lib.codeseg = codeseg;
            lib.dataseg = dataseg;
            m_session.libraries.append(lib);
            logMessage(logMsg);
            // This lets gdb trigger a register update etc.
            // With CS gdb 6.4 we get a non-standard $qfDllInfo#7f+ request
            // afterwards, so don't use it for now.
            //sendGdbServerMessage("T05library:;");
/*
            // Causes too much "stopped" (by SIGTRAP) messages that need
            // to be answered by "continue". Auto-continuing each SIGTRAP
            // is not possible as this is also the real message for a user
            // initiated interrupt.
            sendGdbServerMessage("T05load:Name=" + lib.name.toHex()
                + ",TextSeg=" + hexNumber(lib.codeseg)
                + ",DataSeg=" + hexNumber(lib.dataseg) + ';');
*/
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
            logMessage(_("%1 %2 UNLOAD: %3")
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

void TrkGdbAdapter::handleCpuType(const TrkResult &result)
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

void TrkGdbAdapter::handleDeleteProcess(const TrkResult &result)
{
    Q_UNUSED(result);
    logMessage("Inferior process killed");
    //sendTrkMessage(0x01, TrkCB(handleDeleteProcess2)); // Ping
    sendTrkMessage(0x02, TrkCB(handleDeleteProcess2)); // Disconnect
}

void TrkGdbAdapter::handleDeleteProcess2(const TrkResult &result)
{
    Q_UNUSED(result);
    logMessage("App TRK disconnected");
    sendGdbServerAck();
    sendGdbServerMessage("", "process killed");
}

void TrkGdbAdapter::handleReadRegisters(const TrkResult &result)
{
    logMessage("       REGISTER RESULT: " + result.toString());
    // [80 0B 00   00 00 00 00   C9 24 FF BC   00 00 00 00   00
    //  60 00 00   00 00 00 00   78 67 79 70   00 00 00 00   00...]
    if (result.errorCode()) {
        logMessage("ERROR: " + result.errorString());
        return;
    }
    const char *data = result.data.data() + 1; // Skip ok byte
    for (int i = 0; i < RegisterCount; ++i)
        m_snapshot.registers[i] = extractInt(data + 4 * i);
    m_snapshot.registerValid = true;
} 

void TrkGdbAdapter::handleWriteRegister(const TrkResult &result)
{
    logMessage("       RESULT: " + result.toString() + result.cookie.toString());
    if (result.errorCode()) {
        logMessage("ERROR: " + result.errorString());
        sendGdbServerMessage("E01");
        return;
    }
    sendGdbServerMessage("OK");
} 

void TrkGdbAdapter::reportRegisters()
{
    QByteArray ba;
    for (int i = 0; i < 16; ++i) {
        const uint reg = swapEndian(m_snapshot.registers[i]);
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

void TrkGdbAdapter::handleAndReportReadRegisters(const TrkResult &result)
{
    handleReadRegisters(result);
    reportRegisters();
}

void TrkGdbAdapter::handleAndReportReadRegister(const TrkResult &result)
{
    handleReadRegisters(result);
    uint registerNumber = result.cookie.toUInt();
    QByteArray logMsg = "Read Register";
    if (registerNumber == RegisterPSGdb) {
        QByteArray ba;
        appendInt(&ba, m_snapshot.registers[RegisterPSTrk], LittleEndian);
        logMsg += dumpRegister(registerNumber, m_snapshot.registers[RegisterPSTrk]);
        sendGdbServerMessage(ba.toHex(), logMsg);
    } else if (registerNumber < 16) {
        QByteArray ba;
        appendInt(&ba, m_snapshot.registers[registerNumber], LittleEndian);
        logMsg += dumpRegister(registerNumber, m_snapshot.registers[registerNumber]);
        sendGdbServerMessage(ba.toHex(), logMsg);
    } else {
        sendGdbServerMessage("0000", "read single unknown register #"
            + QByteArray::number(registerNumber));
        //sendGdbServerMessage("E01", "read single unknown register");
    }
}

void TrkGdbAdapter::handleAndReportReadRegistersAfterStop(const TrkResult &result)
{
    handleReadRegisters(result);
    QByteArray ba = "T05";
    for (int i = 0; i < 16; ++i)
        appendRegister(&ba, i, m_snapshot.registers[i]);
    // FIXME: those are not understood by gdb 6.4
    //for (int i = 16; i < 25; ++i)
    //    appendRegister(&ba, i, 0x0);
    appendRegister(&ba, RegisterPSGdb, m_snapshot.registers[RegisterPSTrk]);
    //qDebug() << "TrkGdbAdapter::handleAndReportReadRegistersAfterStop" << ba;
    sendGdbServerMessage(ba, "Registers");
}

static QString msgMemoryReadError(int code, uint addr, uint len = 0)
{
    const QString lenS = len ? QString::number(len) : QLatin1String("<unknown>");
    return _("Memory read error %1 at: 0x%2 %3")
        .arg(code).arg(addr, 0 ,16).arg(lenS);
}

// Format log message for memory access with some smartness about registers
QByteArray TrkGdbAdapter::memoryReadLogMessage(uint addr, const QByteArray &ba) const
{
    QByteArray logMsg = "memory contents";
    if (m_verbose > 1) {
        logMsg += " addr: " + hexxNumber(addr);
        // indicate dereferencing of registers
        if (ba.size() == 4) {
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
        logMsg += QByteArray::number(ba.size());
        logMsg += " :";
        logMsg += stringFromArray(ba, 16).toAscii();
    }
    return logMsg;
}

void TrkGdbAdapter::handleReadMemoryBuffered(const TrkResult &result)
{
    if (extractShort(result.data.data() + 1) + 3 != result.data.size())
        logMessage("\n BAD MEMORY RESULT: " + result.data.toHex() + "\n");
    const MemoryRange range = result.cookie.value<MemoryRange>();
    if (const int errorCode = result.errorCode()) {
        logMessage(_("TEMPORARY: ") + msgMemoryReadError(errorCode, range.from));
        logMessage(_("RETRYING UNBUFFERED"));
        // FIXME: This does not handle large requests properly.
        sendTrkMessage(0x10, TrkCB(handleReadMemoryUnbuffered),
            trkReadMemoryMessage(range), QVariant::fromValue(range));
        return;
    }
    const QByteArray ba = result.data.mid(3);
    m_snapshot.insertMemory(range, ba);
    tryAnswerGdbMemoryRequest(true);
}

void TrkGdbAdapter::handleReadMemoryUnbuffered(const TrkResult &result)
{
    if (extractShort(result.data.data() + 1) + 3 != result.data.size())
        logMessage("\n BAD MEMORY RESULT: " + result.data.toHex() + "\n");
    const MemoryRange range = result.cookie.value<MemoryRange>();
    if (const int errorCode = result.errorCode()) {
        logMessage(_("TEMPORARY: ") + msgMemoryReadError(errorCode, range.from));
        logMessage(_("RETRYING UNBUFFERED"));
#if 1
        const QByteArray ba = "E20";
        sendGdbServerMessage(ba, msgMemoryReadError(32, range.from).toLatin1());
#else
        // emit bogus data to make Python happy
        MemoryRange wanted = m_snapshot.wantedMemory;
        qDebug() << "SENDING BOGUS DATA FOR " << wanted;
        m_snapshot.insertMemory(wanted, QByteArray(wanted.size(), 0xa5));
        tryAnswerGdbMemoryRequest(false);
#endif
        return;
    }
    const QByteArray ba = result.data.mid(3);
    m_snapshot.insertMemory(range, ba);
    tryAnswerGdbMemoryRequest(false);
}

void TrkGdbAdapter::tryAnswerGdbMemoryRequest(bool buffered)
{
    //logMessage("TRYING TO ANSWERING MEMORY REQUEST ");

    MemoryRange wanted = m_snapshot.wantedMemory;
    MemoryRange needed = m_snapshot.wantedMemory;
    MEMORY_DEBUG("WANTED: " << wanted);
    Snapshot::Memory::const_iterator it = m_snapshot.memory.begin();
    Snapshot::Memory::const_iterator et = m_snapshot.memory.end();
    for ( ; it != et; ++it) {
        MEMORY_DEBUG("   NEEDED STEP: " << needed);
        needed -= it.key(); 
    }
    MEMORY_DEBUG("NEEDED FINAL: " << needed);

    if (needed.to == 0) {
        // FIXME: need to combine chunks first.

        // All fine. Send package to gdb.
        it = m_snapshot.memory.begin();
        et = m_snapshot.memory.end();
        for ( ; it != et; ++it) {
            if (it.key().from <= wanted.from && wanted.to <= it.key().to) {
                int offset = wanted.from - it.key().from;
                int len = wanted.to - wanted.from;
                QByteArray ba = it.value().mid(offset, len);
                sendGdbServerMessage(ba.toHex(), memoryReadLogMessage(wanted.from, ba));
                return;
            }
        }
        // Happens when chunks are not comnbined
        QTC_ASSERT(false, /**/);
        debugMessage("CHUNKS NOT COMBINED");
        #ifdef MEMORY_DEBUG
        qDebug() << "CHUNKS NOT COMBINED";
        it = m_snapshot.memory.begin();
        et = m_snapshot.memory.end();
        for ( ; it != et; ++it)
            qDebug() << hexNumber(it.key().from) << hexNumber(it.key().to);
        qDebug() << "WANTED" << wanted.from << wanted.to;
        #endif
        sendGdbServerMessage("E22", "");
        return;
    }

    MEMORY_DEBUG("NEEDED AND UNSATISFIED: " << needed);
    if (buffered) {
        uint blockaddr = (needed.from / MemoryChunkSize) * MemoryChunkSize;
        logMessage(_("Requesting buffered memory %1 bytes from 0x%2")
            .arg(MemoryChunkSize).arg(blockaddr, 0, 16));
        MemoryRange range(blockaddr, blockaddr + MemoryChunkSize);
        MEMORY_DEBUG("   FETCH MEMORY : " << range);
        sendTrkMessage(0x10, TrkCB(handleReadMemoryBuffered),
            trkReadMemoryMessage(range),
            QVariant::fromValue(range));
    } else { // Unbuffered, direct requests
        int len = needed.to - needed.from;
        logMessage(_("Requesting unbuffered memory %1 bytes from 0x%2")
            .arg(len).arg(needed.from, 0, 16));
        sendTrkMessage(0x10, TrkCB(handleReadMemoryUnbuffered),
            trkReadMemoryMessage(needed),
            QVariant::fromValue(needed));
        MEMORY_DEBUG("   FETCH MEMORY : " << needed);
    }
}

/*
void TrkGdbAdapter::reportReadMemoryBuffered(const TrkResult &result)
{
    const MemoryRange range = result.cookie.value<MemoryRange>();
    // Gdb accepts less memory according to documentation.
    // Send E on complete failure.
    QByteArray ba;
    uint blockaddr = (range.from / MemoryChunkSize) * MemoryChunkSize;
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
*/

void TrkGdbAdapter::handleStepInto(const TrkResult &result)
{
    if (result.errorCode()) {
        logMessage("ERROR: " + result.errorString() + " in handleStepInto");

        // Try fallback with Step Over.
        debugMessage("FALLBACK TO 'STEP OVER'");
        QByteArray ba = trkStepRangeMessage(0x11);  // options "step over"
        sendTrkMessage(0x19, TrkCB(handleStepInto2), ba, "Step range");

        // Doing nothing as below does not work as gdb seems to insist on
        // making some progress through a 'step'.
        //sendTrkMessage(0x12,
        //    TrkCB(handleAndReportReadRegistersAfterStop),
        //    trkReadRegistersMessage());
        return;
    }
    // The gdb server response is triggered later by the Stop Reply packet
    logMessage("STEP INTO FINISHED ");
}

void TrkGdbAdapter::handleStepInto2(const TrkResult &result)
{
    if (result.errorCode()) {
        logMessage("ERROR: " + result.errorString() + " in handleStepInto2");

        // Try fallback with Continue.
        debugMessage("FALLBACK TO 'CONTINUE'");
        sendTrkMessage(0x18, TrkCallback(), trkContinueMessage(), "CONTINUE");
        //sendGdbServerMessage("S05", "Stepping finished");

        // Doing nothing as below does not work as gdb seems to insist on
        // making some progress through a 'next'.
        // sendTrkMessage(0x12,
        //     TrkCB(handleAndReportReadRegistersAfterStop),
        //     trkReadRegistersMessage());
        return;
    }
    logMessage("STEP INTO FINISHED (FALLBACK)");
}

void TrkGdbAdapter::handleStepOver(const TrkResult &result)
{
    if (result.errorCode()) {
        logMessage("ERROR: " + result.errorString() + "in handleStepOver");
        // Try fallback with Step Into
        QByteArray ba = trkStepRangeMessage(0x01);  // options "step into"
        sendTrkMessage(0x19, TrkCB(handleStepOver), ba, "Step range");
        return;
    }
    logMessage("STEP OVER FINISHED ");
}

void TrkGdbAdapter::handleStepOver2(const TrkResult &result)
{
    if (result.errorCode()) {
        logMessage("ERROR: " + result.errorString() + "in handleStepOver2");
        // Try fallback with Continue
        sendTrkMessage(0x18, TrkCallback(), trkContinueMessage(), "CONTINUE");
        //sendGdbServerMessage("S05", "Stepping finished");
        return;
    }
    logMessage("STEP OVER FINISHED (FALLBACK)");
}

void TrkGdbAdapter::handleAndReportSetBreakpoint(const TrkResult &result)
{
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 09 00 00 00 00 0A]
    if (result.errorCode()) {
        logMessage("ERROR WHEN SETTING BREAKPOINT: " + result.errorString());
        sendGdbServerMessage("E21");
        return;
    }
    uint bpnr = extractInt(result.data.data() + 1);
    uint addr = result.cookie.toUInt();
    m_session.addressToBP[addr] = bpnr;
    logMessage("SET BREAKPOINT " + hexxNumber(bpnr) + " "
         + stringFromArray(result.data.data()));
    sendGdbServerMessage("OK");
    //sendGdbServerMessage("OK");
}

void TrkGdbAdapter::handleClearBreakpoint(const TrkResult &result)
{
    logMessage("CLEAR BREAKPOINT ");
    if (result.errorCode()) {
        logMessage("ERROR: " + result.errorString());
        //return;
    } 
    sendGdbServerMessage("OK");
}

void TrkGdbAdapter::handleSignalContinue(const TrkResult &result)
{
    uint signalNumber = result.cookie.toUInt();
    logMessage("   HANDLE SIGNAL CONTINUE: " + stringFromArray(result.data));
    logMessage("NUMBER" + QString::number(signalNumber));
    sendGdbServerMessage("O" + QByteArray("Console output").toHex());
    sendGdbServerMessage("W81"); // "Process exited with result 1
}

void TrkGdbAdapter::handleSupportMask(const TrkResult &result)
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

void TrkGdbAdapter::handleTrkVersionsStartGdb(const TrkResult &result)
{
    QString logMsg;
    QTextStream str(&logMsg);
    str << "Versions: ";
    if (result.data.size() >= 5) {
        str << "App TRK version " << int(result.data.at(1)) << '.'
            << int(result.data.at(2))
            << ", TRK protocol version " << int(result.data.at(3))
             << '.' << int(result.data.at(4));
    }
    logMessage(logMsg);
    QStringList gdbArgs;
    gdbArgs.append(QLatin1String("--nx")); // Do not read .gdbinit file
    if (!m_engine->startGdb(gdbArgs, m_options->gdb, TrkOptionsPage::settingsId())) {
        cleanup();
        return;
    }
    emit adapterStarted();
}

void TrkGdbAdapter::handleDisconnect(const TrkResult & /*result*/)
{
    logMessage(QLatin1String("App TRK disconnected"));
}

void TrkGdbAdapter::readMemory(uint addr, uint len, bool buffered)
{
    Q_ASSERT(len < (2 << 16));

    // We try to get medium-sized chunks of data from the device
    if (m_verbose > 2)
        logMessage(_("readMemory %1 bytes from 0x%2 blocksize=%3")
            .arg(len).arg(addr, 0, 16).arg(MemoryChunkSize));

    m_snapshot.wantedMemory = MemoryRange(addr, addr + len);
    tryAnswerGdbMemoryRequest(buffered);

}

void TrkGdbAdapter::interruptInferior()
{
    sendTrkMessage(0x1a, TrkCallback(), trkInterruptMessage(), "Interrupting...");
}

void TrkGdbAdapter::startAdapter()
{
    // Retrieve parameters
    const DebuggerStartParameters &parameters = startParameters();
    m_overrideTrkDevice = parameters.remoteChannel;
    m_overrideTrkDeviceType = parameters.remoteChannelType;
    m_remoteExecutable = parameters.executable;
    m_remoteArguments = parameters.processArgs;
    m_symbolFile = parameters.symbolFileName;
    // FIXME: testing hack, remove!
    if (parameters.processArgs.size() == 3 && parameters.processArgs.at(0) == _("@sym@")) {
        m_remoteExecutable = parameters.processArgs.at(1);
        m_symbolFile = parameters.processArgs.at(2);
        m_remoteArguments.clear();
    }
    // Unixish gdbs accept only forward slashes
    m_symbolFile.replace(QLatin1Char('\\'), QLatin1Char('/'));
    // Start
    QTC_ASSERT(state() == EngineStarting, qDebug() << state());
    setState(AdapterStarting);
    debugMessage(_("TRYING TO START ADAPTER"));
    logMessage(QLatin1String("### Starting TrkGdbAdapter"));
    m_trkDevice->setSerialFrame(effectiveTrkDeviceType() != TrkOptions::BlueTooth);
    // Prompt the user to start communication
    QString message;        
    const trk::PromptStartCommunicationResult src =
            S60DebuggerBluetoothStarter::startCommunication(m_trkDevice,
                                                            effectiveTrkDevice(),
                                                            effectiveTrkDeviceType(),
                                                            0, &message);
    switch (src) {
    case trk::PromptStartCommunicationConnected:
        break;
    case trk::PromptStartCommunicationCanceled:
        emit adapterStartFailed(message, QString());
        return;
    case trk::PromptStartCommunicationError:
        emit adapterStartFailed(message, TrkOptionsPage::settingsId());
        return;
    }

    QTC_ASSERT(m_gdbServer == 0, delete m_gdbServer);
    QTC_ASSERT(m_gdbConnection == 0, m_gdbConnection = 0);
    m_gdbServer = new QTcpServer(this);

    if (!m_gdbServer->listen(QHostAddress(gdbServerIP()), gdbServerPort())) {
        QString msg = QString("Unable to start the gdb server at %1: %2.")
            .arg(m_gdbServerName).arg(m_gdbServer->errorString());
        logMessage(msg);
        emit adapterStartFailed(msg, TrkOptionsPage::settingsId());
        return;
    }

    logMessage(QString("Gdb server running on %1.\nLittle endian assumed.")
        .arg(m_gdbServerName));

    connect(m_gdbServer, SIGNAL(newConnection()),
        this, SLOT(handleGdbConnection()));

    m_trkDevice->sendTrkInitialPing();
    sendTrkMessage(0x02); // Disconnect, as trk might be still connected
    sendTrkMessage(0x01); // Connect
    sendTrkMessage(0x05, TrkCB(handleSupportMask));
    sendTrkMessage(0x06, TrkCB(handleCpuType));
    sendTrkMessage(0x04, TrkCB(handleTrkVersionsStartGdb)); // Versions
}

void TrkGdbAdapter::startInferior()
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    sendTrkMessage(0x40, TrkCB(handleCreateProcess),
                   trk::Launcher::startProcessMessage(m_remoteExecutable, m_remoteArguments));
}

void TrkGdbAdapter::handleCreateProcess(const TrkResult &result)
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    //  40 00 00]
    //logMessage("       RESULT: " + result.toString());
    // [80 08 00   00 00 01 B5   00 00 01 B6   78 67 40 00   00 40 00 00]
    if (result.errorCode()) {
        logMessage("ERROR: " + result.errorString());
        QString msg = _("Cannot start executable \"%1\" on the device:\n%2")
            .arg(m_remoteExecutable).arg(result.errorString());
        // Delay cleanup as not to close a trk device from its read handler,
        // which blocks.
        emitDelayedInferiorStartFailed(msg);
        return;
    }
    const char *data = result.data.data();
    m_session.pid = extractInt(data + 1);
    m_session.tid = extractInt(data + 5);
    m_session.codeseg = extractInt(data + 9);
    m_session.dataseg = extractInt(data + 13);
    const QString startMsg =
        tr("Process started, PID: 0x%1, thread id: 0x%2, "
           "code segment: 0x%3, data segment: 0x%4.")
             .arg(m_session.pid, 0, 16).arg(m_session.tid, 0, 16)
             .arg(m_session.codeseg, 0, 16).arg(m_session.dataseg, 0, 16);

    logMessage(startMsg);

    const QByteArray symbolFile = m_symbolFile.toLocal8Bit();
    if (symbolFile.isEmpty()) {
        logMessage(_("WARNING: No symbol file available."));
    } else {
        // Does not seem to be necessary anymore.
        // FIXME: Startup sequence can be streamlined now as we do not
        // have to wait for the TRK startup to learn the load address.
        //m_engine->postCommand("add-symbol-file \"" + symbolFile + "\" "
        //    + QByteArray::number(m_session.codeseg));
        m_engine->postCommand("symbol-file \"" + symbolFile + "\"");
    }
    m_engine->postCommand("set breakpoint always-inserted on");
    m_engine->postCommand("set trust-readonly-sections"); // No difference?
    m_engine->postCommand("set displaced-stepping on"); // No difference?
    m_engine->postCommand("mem 0x00400000 0x00800000 cache"); 
    m_engine->postCommand("mem 0x78000000 0x88000000 cache ro"); 
    // FIXME: replace with  stack-cache for newer gdb?
    m_engine->postCommand("set remotecache on");  // "info dcache" to check
    m_engine->postCommand("target remote " + gdbServerName().toLatin1(),
        CB(handleTargetRemote));
}

void TrkGdbAdapter::handleTargetRemote(const GdbResponse &record)
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    if (record.resultClass == GdbResultDone) {
        setState(InferiorStopped);
        emit inferiorPrepared();
    } else {
        QString msg = tr("Connecting to TRK server adapter failed:\n")
            + QString::fromLocal8Bit(record.data.findChild("msg").data());
        emit inferiorStartFailed(msg);
    }
}

void TrkGdbAdapter::startInferiorPhase2()
{
    m_engine->continueInferiorInternal();
}

//
// AbstractGdbAdapter interface implementation
//

void TrkGdbAdapter::write(const QByteArray &data)
{
    // Write magic packets directly to TRK.
    if (data.startsWith("@#")) {
        QByteArray data1 = data.mid(2);
        if (data1.endsWith(char(10)))
            data1.chop(1);
        if (data1.endsWith(char(13)))
            data1.chop(1);
        if (data1.endsWith(' '))
            data1.chop(1);
        bool ok;
        uint addr = data1.toUInt(&ok, 0);
        qDebug() << "Writing: " << quoteUnprintableLatin1(data1) << addr;
        directStep(addr);
        return;
    }
    if (data.startsWith("@$")) {
        QByteArray ba = QByteArray::fromHex(data.mid(2));
        qDebug() << "Writing: " << quoteUnprintableLatin1(ba);
        if (ba.size() >= 1)
            sendTrkMessage(ba.at(0), TrkCB(handleDirectTrk), ba.mid(1));
        return;
    }
    if (data.startsWith("@@")) {
        // Read data
        sendTrkMessage(0x10, TrkCB(handleDirectWrite1),
           trkReadMemoryMessage(m_session.dataseg, 12));
        return;
    }
    m_engine->m_gdbProc.write(data);
}

uint oldPC;
QByteArray oldMem;
uint scratch;

void TrkGdbAdapter::handleDirectWrite1(const TrkResult &response)
{
    scratch = m_session.dataseg + 512;
    logMessage("DIRECT WRITE1: " + response.toString());
    if (const int errorCode = response.errorCode()) {
        logMessage("ERROR: " + response.errorString() + "in handleDirectWrite1");
    } else {
        oldMem = response.data.mid(3);
        oldPC = m_snapshot.registers[RegisterPC];
        logMessage("READ MEM: " + oldMem.toHex());
        //qDebug("READ MEM: " + oldMem.toHex());
        QByteArray ba;
        appendByte(&ba, 0xaa);
        appendByte(&ba, 0xaa);
        appendByte(&ba, 0xaa);
        appendByte(&ba, 0xaa);

#if 0
        // Arm:
        //  0:   e51f4004        ldr     r4, [pc, #-4]   ; 4 <.text+0x4>
        appendByte(&ba, 0x04);
        appendByte(&ba, 0x50);  // R5
        appendByte(&ba, 0x1f);
        appendByte(&ba, 0xe5);
#else
        // Thumb:
        // subs  r0, #16  
        appendByte(&ba, 0x08);
        appendByte(&ba, 0x3b);
        // subs  r0, #16  
        appendByte(&ba, 0x08);
        appendByte(&ba, 0x3b);
        //
        appendByte(&ba, 0x08);
        appendByte(&ba, 0x3b);
        // subs  r0, #16  
        appendByte(&ba, 0x08);
        appendByte(&ba, 0x3b);
#endif

        // Write data
        sendTrkMessage(0x11, TrkCB(handleDirectWrite2),
            trkWriteMemoryMessage(scratch, ba));
    }
}

void TrkGdbAdapter::handleDirectWrite2(const TrkResult &response)
{
    logMessage("DIRECT WRITE2: " + response.toString());
    if (const int errorCode = response.errorCode()) {
        logMessage("ERROR: " + response.errorString() + "in handleDirectWrite1");
    } else {
        // Check
        sendTrkMessage(0x10, TrkCB(handleDirectWrite3),
            trkReadMemoryMessage(scratch, 12));
    }
}

void TrkGdbAdapter::handleDirectWrite3(const TrkResult &response)
{
    logMessage("DIRECT WRITE3: " + response.toString());
    if (const int errorCode = response.errorCode()) {
        logMessage("ERROR: " + response.errorString() + "in handleDirectWrite1");
    } else {
        // Set PC
        sendTrkMessage(0x13, TrkCB(handleDirectWrite4),
            trkWriteRegisterMessage(RegisterPC, scratch + 4));
    }
}

void TrkGdbAdapter::handleDirectWrite4(const TrkResult &response)
{
    m_snapshot.registers[RegisterPC] = scratch + 4;
return;
    logMessage("DIRECT WRITE4: " + response.toString());
    if (const int errorCode = response.errorCode()) {
        logMessage("ERROR: " + response.errorString() + "in handleDirectWrite1");
    } else {
        QByteArray ba1;
        appendByte(&ba1, 0x11); // options "step over"
        appendInt(&ba1, scratch + 4);
        appendInt(&ba1, scratch + 4);
        appendInt(&ba1, m_session.pid);
        appendInt(&ba1, m_session.tid);
        sendTrkMessage(0x19, TrkCB(handleDirectWrite5), ba1);
    }
}

void TrkGdbAdapter::handleDirectWrite5(const TrkResult &response)
{
    logMessage("DIRECT WRITE5: " + response.toString());
    if (const int errorCode = response.errorCode()) {
        logMessage("ERROR: " + response.errorString() + "in handleDirectWrite1");
    } else {
        // Restore PC
        sendTrkMessage(0x13, TrkCB(handleDirectWrite6),
            trkWriteRegisterMessage(RegisterPC, oldPC));
    }
}

void TrkGdbAdapter::handleDirectWrite6(const TrkResult &response)
{
    logMessage("DIRECT WRITE6: " + response.toString());
    if (const int errorCode = response.errorCode()) {
        logMessage("ERROR: " + response.errorString() + "in handleDirectWrite1");
    } else {
        // Restore memory
        sendTrkMessage(0x11, TrkCB(handleDirectWrite7),
            trkWriteMemoryMessage(scratch, oldMem));
    }
}

void TrkGdbAdapter::handleDirectWrite7(const TrkResult &response)
{
    logMessage("DIRECT WRITE7: " + response.toString());
    if (const int errorCode = response.errorCode()) {
        logMessage("ERROR: " + response.errorString() + "in handleDirectWrite1");
    } else {
        // Check
        sendTrkMessage(0x10, TrkCB(handleDirectWrite8),
            trkReadMemoryMessage(scratch, 8));
    }
}

void TrkGdbAdapter::handleDirectWrite8(const TrkResult &response)
{
    logMessage("DIRECT WRITE8: " + response.toString());
    if (const int errorCode = response.errorCode()) {
        logMessage("ERROR: " + response.errorString() + "in handleDirectWrite1");
    } else {
        // Re-read registers
        sendTrkMessage(0x12,
            TrkCB(handleAndReportReadRegistersAfterStop),
            trkReadRegistersMessage());
    }
}

void TrkGdbAdapter::handleDirectWrite9(const TrkResult &response)
{
    logMessage("DIRECT WRITE9: " + response.toString());
}

void TrkGdbAdapter::handleDirectTrk(const TrkResult &result)
{
    logMessage("HANDLE DIRECT TRK: " + stringFromArray(result.data));
}

void TrkGdbAdapter::directStep(uint addr)
{
    // Write PC:
    qDebug() << "ADDR: " << addr;
    oldPC = m_snapshot.registers[RegisterPC];
    m_snapshot.registers[RegisterPC] = addr;
    QByteArray ba = trkWriteRegisterMessage(RegisterPC, addr);
    sendTrkMessage(0x13, TrkCB(handleDirectStep1), ba, "Write PC");
}

void TrkGdbAdapter::handleDirectStep1(const TrkResult &result)
{
    logMessage("HANDLE DIRECT STEP1: " + stringFromArray(result.data));
    QByteArray ba;
    appendByte(&ba, 0x11); // options "step over"
    appendInt(&ba, m_snapshot.registers[RegisterPC]);
    appendInt(&ba, m_snapshot.registers[RegisterPC]);
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    sendTrkMessage(0x19, TrkCB(handleDirectStep2), ba, "Direct step");
}

void TrkGdbAdapter::handleDirectStep2(const TrkResult &result)
{
    logMessage("HANDLE DIRECT STEP2: " + stringFromArray(result.data));
    m_snapshot.registers[RegisterPC] = oldPC;
    QByteArray ba = trkWriteRegisterMessage(RegisterPC, oldPC);
    sendTrkMessage(0x13, TrkCB(handleDirectStep3), ba, "Write PC");
}

void TrkGdbAdapter::handleDirectStep3(const TrkResult &result)
{
    logMessage("HANDLE DIRECT STEP2: " + stringFromArray(result.data));
}

void TrkGdbAdapter::cleanup()
{
    m_trkDevice->close();
    delete m_gdbServer;
    m_gdbServer = 0;
}

void TrkGdbAdapter::shutdown()
{
    cleanup();
}

} // namespace Internal
} // namespace Debugger
