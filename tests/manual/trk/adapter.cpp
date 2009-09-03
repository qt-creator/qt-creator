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
#include "trkdevice.h"

#include <QtCore/QPointer>
#include <QtCore/QCoreApplication>
#include <QtCore/QQueue>
#include <QtCore/QTimer>
#include <QtCore/QStringList>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

#ifdef Q_OS_UNIX

#include <signal.h>

void signalHandler(int)
{
    qApp->exit(1);
}

#endif

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

static inline void dumpRegister(int n, uint value, QByteArray &a)
{
    a += ' ';
    if (n < KnownRegisters && registerNames[n]) {
        a += registerNames[n];
    } else {
        a += '#';
        a += QByteArray::number(n);
    }
    a += "=0x";
    a += QByteArray::number(value, 16);
}

struct AdapterOptions
{
    AdapterOptions()
      : verbose(1),
        serialFrame(true),
        registerEndianness(LittleEndian),
        useSocket(false),
        bufferedMemoryRead(true)
    {}

    int verbose;
    bool serialFrame;
    Endianness registerEndianness;
    bool useSocket;
    bool bufferedMemoryRead;
    QString gdbServer;
    QString trkServer;
};

class Adapter : public QObject
{
    Q_OBJECT

public:
    typedef TrkFunctor1<const TrkResult &> Callback;

    Adapter();
    ~Adapter();
    void setGdbServerName(const QString &name);
    void setTrkServerName(const QString &name) { m_trkServerName = name; }
    void setVerbose(int verbose) { m_verbose = verbose; }
    void setSerialFrame(bool b) { m_serialFrame = b; }
    void setRegisterEndianness(Endianness r) { m_registerEndianness = r; }
    void setUseSocket(bool s) { m_useSocket = s; }
    void setBufferedMemoryRead(bool b) { qDebug() << "Buffered=" << b; m_bufferedMemoryRead = b; }

public slots:
    void startServer();

private slots:
    void handleResult(const trk::TrkResult &data);

private:
    //
    // TRK
    //

    bool openTrkPort(const QString &port, QString *errorMessage); // or server name for local server
    void sendTrkMessage(byte code,
        Callback callBack = Callback(),
        const QByteArray &data = QByteArray(),
        const QVariant &cookie = QVariant(),
        bool invokeOnFailure = false);

    // convenience messages
    void sendTrkInitialPing();
    void sendTrkContinue();
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
    void handleAndReportSetBreakpoint(const TrkResult &result);
    void handleReadMemoryBuffered(const TrkResult &result);
    void handleReadMemoryUnbuffered(const TrkResult &result);
    void reportReadMemoryBuffered(const TrkResult &result);
    void reportToGdb(const TrkResult &result);

    void clearTrkBreakpoint(const Breakpoint &bp);

    void handleSetTrkBreakpoint(const TrkResult &result);
    void setTrkBreakpoint(const Breakpoint &bp);

    void readMemory(uint addr, uint len);
    void startInferiorIfNeeded();
    void interruptInferior();

    QSharedPointer<TrkWriteQueueDevice> m_trkDevice;
    QSharedPointer<QIODevice> m_socket;
    QSharedPointer<TrkWriteQueueIODevice> m_socketDevice;

    QString m_trkServerName;
    QByteArray m_trkReadBuffer;

    QList<Breakpoint> m_breakpoints;

    //
    // Gdb
    //
    Q_SLOT void handleGdbConnection();
    Q_SLOT void readFromGdb();
    void handleGdbResponse(const QByteArray &ba);
    void sendGdbMessage(const QByteArray &msg, const QByteArray &logNote = QByteArray());
    void sendGdbMessageAfterSync(const QByteArray &msg, const QByteArray &logNote = QByteArray());
    void sendGdbAckMessage();
    bool sendGdbPacket(const QByteArray &packet, bool doFlush);

    void logMessage(const QString &msg, bool force = false);

    QTcpServer m_gdbServer;
    QPointer<QTcpSocket> m_gdbConnection;
    QString m_gdbServerName;
    quint16 m_gdbServerPort;
    QByteArray m_gdbReadBuffer;
    bool m_gdbAckMode;

    // Debuggee state
    Session m_session; // global-ish data (process id, target information)
    Snapshot m_snapshot; // local-ish data (memory and registers)
    int m_verbose;
    Endianness m_registerEndianness;
    bool m_useSocket;
    bool m_serialFrame;
    bool m_startInferiorTriggered;
    bool m_bufferedMemoryRead;
};

Adapter::Adapter() :
    m_gdbConnection(0),
    m_gdbServerPort(0),
    m_gdbAckMode(true),
    m_verbose(1),
    m_registerEndianness(LittleEndian),
    m_useSocket(false),
    m_serialFrame(true),
    m_startInferiorTriggered(false),
    m_bufferedMemoryRead(true)
{
    // m_breakpoints.append(Breakpoint(0x0040)); // E32Main
}

Adapter::~Adapter()
{
    // Trk
    if (!m_socket.isNull())
        if (QLocalSocket *sock = qobject_cast<QLocalSocket *>(m_socket.data()))
            sock->abort();

    // Gdb
    m_gdbServer.close();
    logMessage("Shutting down.\n", true);
}

void Adapter::setGdbServerName(const QString &name)
{
    int pos = name.indexOf(':');
    if (pos == -1) {
        m_gdbServerPort = 0;
        m_gdbServerName = name;
    } else {
        m_gdbServerPort = name.mid(pos + 1).toUInt();
        m_gdbServerName = name.left(pos);
    }
}

void Adapter::startServer()
{
    QString errorMessage;
    if (!openTrkPort(m_trkServerName, &errorMessage)) {
        logMessage(errorMessage, true);
        logMessage("LOOPING");
        QTimer::singleShot(1000, this, SLOT(startServer()));
        return;
    }

    sendTrkInitialPing();
    sendTrkMessage(0x01); // Connect
    sendTrkMessage(0x05, Callback(this, &Adapter::handleSupportMask));
    sendTrkMessage(0x06, Callback(this, &Adapter::handleCpuType));
    sendTrkMessage(0x04, Callback(this, &Adapter::handleTrkVersions)); // Versions
    //sendTrkMessage(0x09); // Unrecognized command
    //sendTrkMessage(0x4a, 0,
    //    "10 " + formatString("C:\\data\\usingdlls.sisx")); // Open File
    //sendTrkMessage(0x4B, 0, "00 00 00 01 73 1C 3A C8"); // Close File

    logMessage("Connected to TRK server");

    if (!m_gdbServer.listen(QHostAddress(m_gdbServerName), m_gdbServerPort)) {
        logMessage(QString("Unable to start the gdb server at %1:%2: %3.")
            .arg(m_gdbServerName).arg(m_gdbServerPort)
            .arg(m_gdbServer.errorString()), true);
        QCoreApplication::exit(5);
        return;
    }

    logMessage(QString("Gdb server running on %1:%2.\nRegister endianness: %3\nRun arm-gdb now.")
        .arg(m_gdbServerName).arg(m_gdbServer.serverPort()).arg(m_registerEndianness), true);

    connect(&m_gdbServer, SIGNAL(newConnection()),
        this, SLOT(handleGdbConnection()));
}

void Adapter::logMessage(const QString &msg, bool force)
{
    if (m_verbose || force)
        qDebug("ADAPTER: %s ", qPrintable(msg));
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
            this, SLOT(readFromGdb()));
    m_startInferiorTriggered = false;
}

static inline QString msgGdbPacket(const QString &p)
{
    return QLatin1String("gdb: -> ") + p;
}

void Adapter::readFromGdb()
{
    QByteArray packet = m_gdbConnection->readAll();
    m_gdbReadBuffer.append(packet);

    logMessage(msgGdbPacket(QString::fromAscii(packet)));
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

        QByteArray response = ba.left(pos);
        ba.remove(0, pos + 3);
        handleGdbResponse(response);
    }
}

bool Adapter::sendGdbPacket(const QByteArray &packet, bool doFlush)
{
    if (!m_gdbConnection || m_gdbConnection->state() != QAbstractSocket::ConnectedState) {
        logMessage(QString::fromLatin1("Cannot write to gdb: Not connected (%1)").arg(QString::fromLatin1(packet)), true);
        return false;
    }
    if (m_gdbConnection->write(packet) == -1) {
        logMessage(QString::fromLatin1("Cannot write to gdb: %1 (%2)").arg(m_gdbConnection->errorString()).arg(QString::fromLatin1(packet)), true);
        return false;
    }
    if (doFlush)
        m_gdbConnection->flush();
    return true;
}

void Adapter::sendGdbAckMessage()
{
    if (!m_gdbAckMode)
        return;
    QByteArray packet = "+";
    logMessage("gdb: <- " + packet);
    sendGdbPacket(packet, false);
}

void Adapter::sendGdbMessage(const QByteArray &msg, const QByteArray &logNote)
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
    sendGdbPacket(packet, true);
}

void Adapter::sendGdbMessageAfterSync(const QByteArray &msg, const QByteArray &logNote)
{
    QByteArray ba = msg + char(1) + logNote;
    sendTrkMessage(TRK_WRITE_QUEUE_NOOP_CODE, Callback(this, &Adapter::reportToGdb), "", ba); // Answer gdb
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
    sendGdbMessage(message, note);
}

static QByteArray breakpointTrkMessage(uint addr, int len, int pid, bool armMode = true)
{
    QByteArray ba;
    appendByte(&ba, 0x82);  // unused option
    appendByte(&ba, armMode /*bp.mode == ArmMode*/ ? 0x00 : 0x01);
    appendInt(&ba, addr);
    appendInt(&ba, len);
    appendInt(&ba, 0x00000001);
    appendInt(&ba, pid);
    appendInt(&ba, 0xFFFFFFFF);
    return ba;
}

void Adapter::handleGdbResponse(const QByteArray &response)
{
    // http://sourceware.org/gdb/current/onlinedocs/gdb_34.html
    if (0) {}

    else if (response == "!") {
        sendGdbAckMessage();
        sendGdbMessage("", "extended mode not enabled");
        //sendGdbMessage("OK", "extended mode enabled");
    }

    else if (response.startsWith("?")) {
        if (m_verbose)
            logMessage(msgGdbPacket(QLatin1String("Query halted")));
        // Indicate the reason the target halted.
        // The reply is the same as for step and continue.
        sendGdbAckMessage();
        startInferiorIfNeeded();        
        sendGdbMessage("T05library:r;", "target halted (library load)");
        // trap 05
        // sendGdbMessage("S05", "target halted (trap)");
    }

    else if (response == "c") {
        if (m_verbose)
            logMessage(msgGdbPacket(QLatin1String("continue")));
        sendGdbAckMessage();
        QByteArray ba;
        appendByte(&ba, 0); // options
        appendInt(&ba, 0); // start address
        appendInt(&ba, 0); // end address
        appendInt(&ba, m_session.pid);
        appendInt(&ba, m_session.tid);
        sendTrkMessage(0x18, Callback(), ba);
        // FIXME: should be triggered by real stop
        //sendGdbMessageAfterSync("S11", "target stopped");
    }

    else if (response.startsWith("C")) {
        if (m_verbose)
            logMessage(msgGdbPacket(QLatin1String("continue with signal")));
        // C sig[;addr] Continue with signal sig (hex signal number)
        //Reply: See section D.3 Stop Reply Packets, for the reply specifications.
        sendGdbAckMessage();
        bool ok = false;
        uint signalNumber = response.mid(1).toInt(&ok, 16);
        QByteArray ba;
        appendInt(&ba, m_session.pid);
        appendInt(&ba, m_session.tid);
        sendTrkMessage(0x18, Callback(this, &Adapter::handleSignalContinue), ba, signalNumber); // Continue
    }

    else if (response.startsWith("D")) {
        sendGdbAckMessage();
        sendGdbMessage("OK", "shutting down");
        qApp->quit();
    }

    else if (response == "g") {
        if (m_verbose)
            logMessage(msgGdbPacket(QLatin1String("read registers")));
        // Read general registers.
        //sendGdbMessage("00000000", "read registers");
        sendGdbAckMessage();
        QByteArray ba;
        appendByte(&ba, 0); // Register set, only 0 supported
        appendShort(&ba, 0);
        appendShort(&ba, RegisterCount - 1); // last register
        appendInt(&ba, m_session.pid);
        appendInt(&ba, m_session.tid);
        sendTrkMessage(0x12, Callback(this, &Adapter::handleAndReportReadRegisters), ba, QVariant(), true);
    }

    else if (response.startsWith("Hc")) {
        if (m_verbose)
            logMessage(msgGdbPacket(QLatin1String("Set thread & continue")));
        // Set thread for subsequent operations (`m', `M', `g', `G', et.al.).
        // for step and continue operations
        //$Hc-1#09
        sendGdbAckMessage();
        sendGdbMessage("OK", "set current thread for step & continue");
    }

    else if (response.startsWith("Hg")) {
        if (m_verbose)
            logMessage(msgGdbPacket(QLatin1String("Set thread")));
        // Set thread for subsequent operations (`m', `M', `g', `G', et.al.).
        // for 'other operations.  0 - any thread
        //$Hg0#df
        sendGdbAckMessage();
        m_session.currentThread = response.mid(2).toInt(0, 16);
        sendGdbMessage("OK", "set current thread "
            + QByteArray::number(m_session.currentThread));
    }

    else if (response == "k") {
        if (m_verbose)
            logMessage(msgGdbPacket(QLatin1String("kill")));
        // kill
        sendGdbAckMessage();
        QByteArray ba;
        appendByte(&ba, 0); // Sub-command: Delete Process
        appendInt(&ba, m_session.pid);
        sendTrkMessage(0x41, Callback(), ba, "Delete process"); // Delete Item
        sendGdbMessageAfterSync("", "process killed");
    }

    else if (response.startsWith("m")) {
        if (m_verbose)
            logMessage(msgGdbPacket(QLatin1String("read memory")));
        // m addr,length
        sendGdbAckMessage();
        uint addr = 0, len = 0;
        do {
            const int pos = response.indexOf(',');
            if (pos == -1)
                break;
            bool ok;
            addr = response.mid(1, pos - 1).toUInt(&ok, 16);
            if (!ok)
                break;
            len = response.mid(pos + 1).toUInt(&ok, 16);
            if (!ok)
                break;
        } while (false);
        if (len) {
            readMemory(addr, len);
        } else {
            sendGdbMessage("E20", "Error " + response);
        }
    }
    else if (response.startsWith("p")) {
        if (m_verbose)
            logMessage(msgGdbPacket(QLatin1String("read register")));
        // 0xf == current instruction pointer?
        //sendGdbMessage("0000", "current IP");
        sendGdbAckMessage();
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
        const uint registerNumber = response.mid(1).toInt(&ok, 16);
        QByteArray logMsg = "read register";
        if (registerNumber == RegisterPSGdb) {
            QByteArray ba;
            appendInt(&ba, m_snapshot.registers[RegisterPSTrk], m_registerEndianness);
            dumpRegister(registerNumber, m_snapshot.registers[RegisterPSTrk], logMsg);
            sendGdbMessage(ba.toHex(), logMsg);
        } else if (registerNumber < RegisterCount) {
            QByteArray ba;
            appendInt(&ba, m_snapshot.registers[registerNumber], m_registerEndianness);
            dumpRegister(registerNumber, m_snapshot.registers[registerNumber], logMsg);
            sendGdbMessage(ba.toHex(), logMsg);
        } else {
            sendGdbMessage("0000", "read single unknown register #" + QByteArray::number(registerNumber));
            //sendGdbMessage("E01", "read single unknown register");
        }
    }

    else if (response == "qAttached") {
        //$qAttached#8f
        // 1: attached to an existing process
        // 0: created a new process
        sendGdbAckMessage();
        sendGdbMessage("0", "new process created");
        //sendGdbMessage("1", "attached to existing process");
        //sendGdbMessage("E01", "new process created");
    }

    else if (response.startsWith("qC")) {
        if (m_verbose)
            logMessage(msgGdbPacket(QLatin1String("query thread id")));
        // Return the current thread ID
        //$qC#b4
        sendGdbAckMessage();
        startInferiorIfNeeded();
        sendGdbMessageAfterSync("QC@TID@");
    }

    else if (response.startsWith("qSupported")) {
        //$qSupported#37
        //$qSupported:multiprocess+#c6
        //logMessage("Handling 'qSupported'");
        sendGdbAckMessage();
        if (0)
            sendGdbMessage(QByteArray(), "nothing supported");
        else
            sendGdbMessage(
                "PacketSize=7cf;"
                //"QPassSignals+;"
                "qXfer:libraries:read+;"
                //"qXfer:auxv:read+;"
                "qXfer:features:read+");
    }

    else if (response == "qPacketInfo") {
        // happens with  gdb 6.4.50.20060226-cvs / CodeSourcery
        // deprecated by qSupported?
        sendGdbAckMessage();
        sendGdbMessage("", "FIXME: nothing?");
    }

    else if (response == "qOffsets") {
        sendGdbAckMessage();
        startInferiorIfNeeded();
        sendGdbMessageAfterSync("TextSeg=@CODESEG@;DataSeg=@DATASEG@");
    }

    else if (response == "qSymbol::") {
        if (m_verbose)
            logMessage(msgGdbPacket(QLatin1String("notify can handle symbol lookup")));
        // Notify the target that GDB is prepared to serve symbol lookup requests.
        sendGdbAckMessage();
        if (1)
            sendGdbMessage("OK", "no further symbols needed");
        else
            sendGdbMessage("qSymbol:" + QByteArray("_Z7E32Mainv").toHex(), "ask for more");
    }

    else if (response.startsWith("qXfer:features:read:target.xml:")) {
        //  $qXfer:features:read:target.xml:0,7ca#46...Ack
        sendGdbAckMessage();
        sendGdbMessage("l<target><architecture>symbianelf</architecture></target>");
    }

    else if (response == "QStartNoAckMode") {
        //$qSupported#37
        //logMessage("Handling 'QStartNoAckMode'");
        sendGdbAckMessage();
        sendGdbMessage("OK", "ack no-ack mode");
        m_gdbAckMode = false;
    }

    else if (response.startsWith("QPassSignals")) {
        // list of signals to pass directly to inferior
        // $QPassSignals:e;10;14;17;1a;1b;1c;21;24;25;4c;#8f
        // happens only if "QPassSignals+;" is qSupported
        sendGdbAckMessage();
        // FIXME: use the parameters
        sendGdbMessage("OK", "passing signals accepted");
    }

    else if (response == "s") {
        if (m_verbose)
            logMessage(msgGdbPacket(QLatin1String("Step range")));
        sendGdbAckMessage();
        QByteArray ba;
        appendByte(&ba, 0); // options
        appendInt(&ba, m_snapshot.registers[RegisterPC]); // start address
        appendInt(&ba, m_snapshot.registers[RegisterPC] + 4); // end address
        appendInt(&ba, m_session.pid);
        appendInt(&ba, m_session.tid);
        sendTrkMessage(0x19, Callback(), ba, "Step range");
        // FIXME: should be triggered by "real" stop"
        //sendGdbMessageAfterSync("S05", "target halted");
    }

    else if (response == "vCont?") {
        // actions supported by the vCont packet
        sendGdbAckMessage();
        sendGdbMessage(""); // we don't support vCont.
        //sendGdbMessage("vCont;c");
    }

    //else if (response.startsWith("vCont")) {
    //    // vCont[;action[:thread-id]]...'
    //}

    else if (response.startsWith("vKill")) {
        // kill
        sendGdbAckMessage();
        QByteArray ba;
        appendByte(&ba, 0); // Sub-command: Delete Process
        appendInt(&ba, m_session.pid);
        sendTrkMessage(0x41, Callback(), ba, "Delete process"); // Delete Item
        sendGdbMessageAfterSync("", "process killed");
    }

    else if (response.startsWith("Z0,")) { // Insert breakpoint
        if (m_verbose)
            logMessage(msgGdbPacket(QLatin1String("Insert breakpoint")));
        // $z0,786a4ccc,4#99
        const int pos = response.lastIndexOf(',');
        bool ok = false;
        const uint addr = response.mid(3, pos - 1).toInt(&ok, 16);
        const uint len = response.mid(pos + 1).toInt(&ok, 16);
        if (m_verbose)
            logMessage(QString::fromLatin1("Inserting breakpoint at 0x%1, %2").arg(addr,0 ,16).arg(len));

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
        const QByteArray ba = breakpointTrkMessage(addr, len, m_session.pid);
        sendTrkMessage(0x1B, Callback(this, &Adapter::handleAndReportSetBreakpoint), ba);
        //m_session.toekn

        //---TRK------------------------------------------------------
        //  Command: 0x80 Acknowledge
        //    Error: 0x00
        // [80 09 00 00 00 00 0A]
    } else if (response.startsWith("qPart:") || response.startsWith("qXfer:"))  {
        QByteArray data  = response.mid(1 + response.indexOf(':'));
        // "qPart:auxv:read::0,147": Read OS auxiliary data (see info aux)
        bool handled = false;
        if (data.startsWith("auxv:read::")) {
            const int offsetPos = data.lastIndexOf(':') + 1;
            const int commaPos = data.lastIndexOf(',');
            if (commaPos != -1) {                
                bool ok1 = false, ok2 = false;
                const int offset = data.mid(offsetPos,  commaPos - offsetPos).toInt(&ok1, 16);
                const int length = data.mid(commaPos + 1).toInt(&ok2, 16);
                if (ok1 && ok2) {
                    const QString msg = QString::fromLatin1("Read of OS auxilary vector (%1, %2) not implemented.").arg(offset).arg(length);
                    logMessage(msgGdbPacket(msg), true);
                    sendGdbMessage("E20", msg.toLatin1());
                    handled = true;
                }
            }
        } // auxv read
        if (!handled) {
            const QString msg = QLatin1String("FIXME unknown 'XFER'-request: ") + QString::fromAscii(response);
            logMessage(msgGdbPacket(msg), true);
            sendGdbMessage("E20", msg.toLatin1());
        }
    } // qPart/qXfer
    else {
        logMessage(msgGdbPacket(QLatin1String("FIXME unknown: ") + QString::fromAscii(response)));
    }
}

bool Adapter::openTrkPort(const QString &port, QString *errorMessage)
{
    if (m_useSocket) {
        QLocalSocket *socket = new QLocalSocket;
        m_socket = QSharedPointer<QIODevice>(new QLocalSocket);
        socket->connectToServer(port);
        if (!socket->waitForConnected()) {
            *errorMessage = "Unable to connect to TRK server " + m_trkServerName + ' ' + socket->errorString();
            delete socket;
            return false;
        }
        m_socketDevice = QSharedPointer<TrkWriteQueueIODevice>(new TrkWriteQueueIODevice(m_socket));
        connect(m_socketDevice.data(), SIGNAL(messageReceived(trk::TrkResult)), this, SLOT(handleResult(trk::TrkResult)));
        if (m_verbose > 1)
            m_socketDevice->setVerbose(true);
        m_socketDevice->setSerialFrame(m_serialFrame);
        return true;
    }
    m_trkDevice = QSharedPointer<TrkWriteQueueDevice>(new TrkWriteQueueDevice);
    connect(m_trkDevice.data(), SIGNAL(messageReceived(trk::TrkResult)), this, SLOT(handleResult(trk::TrkResult)));
    if (m_verbose > 1)
        m_trkDevice->setVerbose(true);
    m_trkDevice->setSerialFrame(m_serialFrame);
    return m_trkDevice->open(port, errorMessage);
}

void Adapter::sendTrkMessage(byte code, Callback callBack,
    const QByteArray &data, const QVariant &cookie, bool invokeOnFailure)
{
    if (m_useSocket)
        m_socketDevice->sendTrkMessage(code, callBack, data, cookie, invokeOnFailure);
    else
        m_trkDevice->sendTrkMessage(code, callBack, data, cookie, invokeOnFailure);
}

void Adapter::sendTrkInitialPing()
{
    if (m_useSocket)
        m_socketDevice->sendTrkInitialPing();
    else
        m_trkDevice->sendTrkInitialPing();
}

void Adapter::sendTrkContinue()
{
    QByteArray ba;
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    sendTrkMessage(0x18, Callback(), ba, "CONTINUE");
}

void Adapter::waitForTrkFinished()
{
    // initiate one last roundtrip to ensure all is flushed
    sendTrkMessage(0x0, Callback(this, &Adapter::handleWaitForFinished));
}

void Adapter::sendTrkAck(byte token)
{
    logMessage(QString("SENDING ACKNOWLEDGEMENT FOR TOKEN %1").arg(int(token)));
    if (m_useSocket)
        m_socketDevice->sendTrkAck(token);
    else
        m_trkDevice->sendTrkAck(token);
}

void Adapter::handleResult(const TrkResult &result)
{
    if (result.isDebugOutput) {
        logMessage(QLatin1String("APPLICATION OUTPUT: ") + QString::fromAscii(result.data));
        return;
    }
    QByteArray prefix = "READ BUF:                                       ";
    QByteArray str = result.toString().toUtf8();
    switch (result.code) {
        case 0x80: // ACK
            break;
        case 0xff: { // NAK. This mostly means transmission error, not command failed.
            QString logMsg;
            QTextStream(&logMsg) << prefix << "NAK: for token=" << result.token << " ERROR: " << errorMessage(result.data.at(0)) << ' ' << str;
            logMessage(logMsg, true);
            break;
        }
        case 0x90: { // Notified Stopped
            // 90 01   78 6a 40 40   00 00 07 23   00 00 07 24  00 00
            const char *data = result.data.data();
            const uint addr = extractInt(data); //code address: 4 bytes; code base address for the library
            const uint pid = extractInt(data + 4); // ProcessID: 4 bytes;
            const uint tid = extractInt(data + 8); // ThreadID: 4 bytes
            logMessage(prefix + QString::fromLatin1("NOTE: PID %1/TID %2 STOPPED at 0x%3").arg(pid).arg(tid).arg(addr, 0, 16));
            sendTrkAck(result.token);
            if (addr) {
                // Todo: Do not send off GdbMessages if a synced gdb query is pending, queue instead
                sendGdbMessage("S05", "Target stopped");
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
            const byte type = result.data.at(1); // type: 1 byte; for dll item, this value is 2.
            const uint pid = extractInt(data + 2); //  ProcessID: 4 bytes;
            const uint tid = extractInt(data + 6); //threadID: 4 bytes
            const uint codeseg = extractInt(data + 10); //code address: 4 bytes; code base address for the library
            const uint dataseg = extractInt(data + 14); //data address: 4 bytes; data base address for the library
            const uint len = extractShort(data + 18); //length: 2 bytes; length of the library name string to follow
            const QByteArray name = result.data.mid(20, len); // name: library name
            m_session.modules += QString::fromAscii(name);
            QString logMsg;
            QTextStream str(&logMsg);
            str<< prefix << " NOTE: LIBRARY LOAD: token=" << result.token;
            if (error)
                str<< " ERROR: " << int(error);
            str << " TYPE: " << int(type) << " PID: " << pid << " TID:   " <<  tid;
            str.setIntegerBase(16);
            str << " CODE: 0x" << codeseg << " DATA: 0x" << dataseg;
            str.setIntegerBase(10);
            str << " NAME: '" << name << '\'';
            logMessage(logMsg);
            sendTrkContinue();
            break;
        }
        case 0xa1: { // NotifyDeleted
            const ushort itemType = (unsigned char)result.data.at(1);
            const ushort len = result.data.size() > 12 ? extractShort(result.data.data() + 10) : ushort(0);
            const QString name = len ? QString::fromAscii(result.data.mid(12, len)) : QString();
            if (!name.isEmpty())
                m_session.modules.removeAll(name);
            logMessage(QString::fromLatin1("%1 %2 UNLOAD: %3").
                       arg(QString::fromAscii(prefix)).arg(itemType ? QLatin1String("LIB") : QLatin1String("PROCESS")).
                       arg(name));
            sendTrkAck(result.token);
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
    QTextStream(&logMsg) << "HANDLE CPU TYPE: " << " CPU=" << m_session.cpuMajor << '.'
            << m_session.cpuMinor << " bigEndian=" << m_session.bigEndian
            << " defaultTypeSize=" << m_session.defaultTypeSize
            << " fpTypeSize=" << m_session.fpTypeSize << " extended1TypeSize=" <<  m_session.extended1TypeSize;
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
    const QByteArray ba = breakpointTrkMessage(m_session.codeseg + bp.offset, 1, m_session.pid);
    sendTrkMessage(0x1B, Callback(this, &Adapter::handleSetTrkBreakpoint), ba);

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
    const  uint bpnr = extractInt(result.data.data());
    if (m_verbose)
        logMessage(QString::fromLatin1("SET BREAKPOINT %1 %2").arg(bpnr).arg(stringFromArray(result.data.data())));
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
    QString logMsg = QString::fromLatin1("handleCreateProcess PID=%1 TID=%2 CODE=0x%3 (%4) DATA=0x%5 (%6)")
                     .arg(m_session.pid).arg(m_session.tid).arg(m_session.codeseg, 0 ,16).arg(m_session.codeseg).arg(m_session.dataseg, 0, 16).arg(m_session.dataseg);
    logMessage(logMsg);
    foreach (const Breakpoint &bp, m_breakpoints)
        setTrkBreakpoint(bp);

    sendTrkContinue();
#if 0
    /*
    logMessage("PID: " + formatInt(m_session.pid) + m_session.pid);
    logMessage("TID: " + formatInt(m_session.tid) + m_session.tid);
    logMessage("COD: " + formatInt(m_session.codeseg) + m_session.codeseg);
    logMessage("DAT: " + formatInt(m_session.dataseg) + m_session.dataseg);
    */


    //setTrkBreakpoint(0x0000, ArmMode);
    //clearTrkBreakpoint(0x0000);




#if 1
    //---IDE------------------------------------------------------
    //  Command: 0x42 Read Info
    //          [42 0C 00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F
    //  72 70 68 69 63 44 4C 4C 32 2E 64 6C 6C 00]
    sendTrkMessage(0x42, Callback(this, &Adapter::handleReadInfo),
        "00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F "
        "72 70 68 69 63 44 4C 4C 32 2E 64 6C 6C 00");
    //sendTrkMessage(0x42, Callback(this, &Adapter::handleReadInfo),
    //        "00 01 00 00 00 00");
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x20 Unspecified general OS-related error
    // [80 0C 20]


    //---IDE------------------------------------------------------
    //  Command: 0x42 Read Info
    // [42 0D 00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F
    //  72 70 68 69 63 44 4C 4C 31 2E 64 6C 6C 00]
    sendTrkMessage(0x42, Callback(this, &Adapter::handleReadInfo),
        "00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F "
        "72 70 68 69 63 44 4C 4C 31 2E 64 6C 6C 00");
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x20 Unspecified general OS-related error
    // [80 0D 20]
#endif

    //sendTrkMessage(0x18, Callback(this, &Adapter::handleStop),
    //    "01 " + formatInt(m_session.pid) + formatInt(m_session.tid));

    //---IDE------------------------------------------------------
    //  Command: 0x18 Continue
    //ProcessID: 0x000001B5 (437)
    // ThreadID: 0x000001B6 (438)
    // [18 0E 00 00 01 B5 00 00 01 B6]
    QByteArray ba;
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    sendTrkMessage(0x18, Callback(this, &Adapter::handleContinue), ba);
    //sendTrkMessage(0x18, Callback(this, &Adapter::handleContinue),
    //    formatInt(m_session.pid) + "ff ff ff ff");
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 0E 00]
#endif
}

void Adapter::handleAndReportReadRegisters(const TrkResult &result)
{
    //logMessage("       RESULT: " + result.toString());
    // [80 0B 00   00 00 00 00   C9 24 FF BC   00 00 00 00   00
    //  60 00 00   00 00 00 00   78 67 79 70   00 00 00 00   00...]

    const char *data = result.data.data() + 1; // Skip ok byte
    for (int i = 0; i < RegisterCount; ++i) {
        m_snapshot.registers[i] = extractInt(data + 4 * i);
    }
    QByteArray ba;
    for (int i = 0; i < 16; ++i) {
        const uint reg = m_registerEndianness == LittleEndian ? swapEndian(m_snapshot.registers[i]) : m_snapshot.registers[i];
        ba += hexNumber(reg, 8);
    }
    QByteArray logMsg = "register contents";
    if (m_verbose > 1) {
        for (int i = 0; i < RegisterCount; ++i)
            dumpRegister(i, m_snapshot.registers[i], logMsg);
    }
    sendGdbMessage(ba, logMsg);
}

static inline QString msgMemoryReadError(int code, uint addr, uint len = 0)
{
    const QString lenS = len ? QString::number(len) : QLatin1String("<unknown>");
    return QString::fromLatin1("Memory read error %1 at: 0x%2 %3").arg(code).arg(addr, 0 ,16).arg(lenS);
}

void Adapter::handleReadMemoryBuffered(const TrkResult &result)
{
    const uint blockaddr = result.cookie.toUInt();
    if (const int errorCode = result.errorCode()) {
        logMessage(msgMemoryReadError(errorCode, blockaddr));
        return;
    }
    const QByteArray ba = result.data.mid(1);
    m_snapshot.memory.insert(blockaddr , ba);
}

// Format log message for memory access with some smartness about registers
QByteArray Adapter::memoryReadLogMessage(uint addr, uint len, const QByteArray &ba) const
{
    QByteArray logMsg = "memory contents";
    if (m_verbose > 1) {
        logMsg += " addr: 0x";
        logMsg += QByteArray::number(addr, 16);
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
            } else if (addr > m_snapshot.registers[RegisterSP] && (addr - m_snapshot.registers[RegisterSP]) < 10240) {
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
        sendGdbMessage(ba, msgMemoryReadError(32, addr, len).toLatin1());
    } else {
        sendGdbMessage(ba.toHex(), memoryReadLogMessage(addr, len, ba));
    }
}

void Adapter::handleReadMemoryUnbuffered(const TrkResult &result)
{
    const uint blockaddr = result.cookie.toUInt();
    if (const int errorCode = result.errorCode()) {
        const QByteArray ba = "E20";
        sendGdbMessage(ba, msgMemoryReadError(32, blockaddr).toLatin1());
    } else {
        const QByteArray ba = result.data.mid(1);
        sendGdbMessage(ba.toHex(), memoryReadLogMessage(blockaddr, ba.size(), ba));
    }
}

void Adapter::handleAndReportSetBreakpoint(const TrkResult &result)
{
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 09 00 00 00 00 0A]
    uint bpnr = extractInt(result.data.data());
    logMessage("SET BREAKPOINT " + bpnr
        + stringFromArray(result.data.data()));
    sendGdbMessage("OK");
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
    sendTrkMessage(0x1C, Callback(this, &Adapter::handleClearBreakpoint), ba);
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
    qDebug() << "NUMBER" << signalNumber;
    sendGdbMessage("O" + QByteArray("Console output").toHex());
    sendGdbMessage("W81"); // "Process exited with result 1
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
        str << "Trk version " << int(result.data.at(1)) << '.' << int(result.data.at(2))
                << ", Protocol version " << int(result.data.at(3)) << '.' << int(result.data.at(4));
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
    sendTrkMessage(0x41, Callback(), ba, "Delete process");

    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 24 00]

    foreach (const Breakpoint &bp, m_breakpoints)
        clearTrkBreakpoint(bp);

    sendTrkMessage(0x02, Callback(this, &Adapter::handleDisconnect));
    m_startInferiorTriggered = false;
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
//    sendTrkMessage(0x02, Callback(this, &Adapter::handleDisconnect));
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    // Error: 0x00
}

static inline QByteArray memoryRequestTrkMessage(uint addr, uint len, int pid, int tid)
{
    QByteArray ba;
    appendByte(&ba, 0x08); // Options, FIXME: why?
    appendShort(&ba, len);
    appendInt(&ba, addr);
    appendInt(&ba, pid);
    appendInt(&ba, tid);
    return ba;
}

void Adapter::readMemory(uint addr, uint len)
{
    Q_ASSERT(len < (2 << 16));

    // We try to get medium-sized chunks of data from the device
    if (m_verbose > 2)
        logMessage(QString::fromLatin1("readMemory %1 bytes from 0x%2 blocksize=%3").arg(len).arg(addr, 0, 16).arg(MemoryChunkSize));

    if (m_bufferedMemoryRead) {
        uint blockaddr = (addr / MemoryChunkSize) * MemoryChunkSize;
        for (; blockaddr < addr + len; blockaddr += MemoryChunkSize) {
            if (!m_snapshot.memory.contains(blockaddr)) {
                if (m_verbose)
                    logMessage(QString::fromLatin1("Requesting buffered memory %1 bytes from 0x%2").arg(MemoryChunkSize).arg(blockaddr, 0, 16));
                sendTrkMessage(0x10, Callback(this, &Adapter::handleReadMemoryBuffered),
                               memoryRequestTrkMessage(blockaddr, MemoryChunkSize, m_session.pid, m_session.tid),
                               QVariant(blockaddr), true);
            }
        }
        const qulonglong cookie = (qulonglong(addr) << 32) + len;
        sendTrkMessage(TRK_WRITE_QUEUE_NOOP_CODE, Callback(this, &Adapter::reportReadMemoryBuffered), QByteArray(), cookie);
    } else {
        if (m_verbose)
            logMessage(QString::fromLatin1("Requesting unbuffered memory %1 bytes from 0x%2").arg(len).arg(addr, 0, 16));
        sendTrkMessage(0x10, Callback(this, &Adapter::handleReadMemoryUnbuffered),
                       memoryRequestTrkMessage(addr, len, m_session.pid, m_session.tid),
                       QVariant(addr), true);
    }
}

void Adapter::startInferiorIfNeeded()
{
    if (m_startInferiorTriggered)
        return;
    if (m_session.pid != 0) {
        qDebug() << "Process already 'started'";
        return;
    }
    // It's not started yet
    m_startInferiorTriggered = true;
    QByteArray ba;
    appendByte(&ba, 0); // ?
    appendByte(&ba, 0); // ?
    appendByte(&ba, 0); // ?

    QByteArray file("C:\\sys\\bin\\filebrowseapp.exe");
    appendString(&ba, file, TargetByteOrder);
    sendTrkMessage(0x40, Callback(this, &Adapter::handleCreateProcess), ba); // Create Item
}

void Adapter::interruptInferior()
{
    QByteArray ba;
    // stop the thread (2) or the process (1) or the whole system (0)
    appendByte(&ba, 1);
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid); // threadID: 4 bytes Variable number of bytes.
    sendTrkMessage(0x1A, Callback(), ba, "Interrupting...");
}

static bool readAdapterArgs(const QStringList &args, AdapterOptions *o)
{
    int argNumber = 0;
    const QStringList::const_iterator cend = args.constEnd();
    QStringList::const_iterator it = args.constBegin();
    for (++it; it != cend; ++it) {
        if (it->startsWith(QLatin1Char('-'))) {
            if (*it == QLatin1String("-v")) {
                o->verbose++;
            } else if (*it == QLatin1String("-q")) {
                o->verbose = 0;
            } else if (*it == QLatin1String("-b")) {
                o->registerEndianness = BigEndian;
            } else if (*it == QLatin1String("-s")) {
                o->useSocket = true;
            } else if (*it == QLatin1String("-f")) {
                o->serialFrame = false;
            } else if (*it == QLatin1String("-u")) {
                o->bufferedMemoryRead = false;
            } else {
                return false;
            }
        } else {
            switch (argNumber++) {
            case 0:
                o->trkServer = *it;
                break;
            case 1:
                o->gdbServer = *it;
                break;
            }
        }
      }
    return !o->gdbServer.isEmpty();
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_UNIX
    signal(SIGUSR1, signalHandler);
#endif

    QCoreApplication app(argc, argv);
    AdapterOptions options;

    if (!readAdapterArgs(app.arguments(), &options)) {
        qDebug("Usage: %s [-v|-q] [-b][-s][-l][-u] <trk com/trkservername> <gdbserverport>\n"
               "Options: -v verbose\n"
               "         -f Turn serial message frame off\n"
               "         -u Turn buffered memory read off\n"
               "         -q quiet\n"
               "         -s Use socket (simulation)\n"
               "         -b Set register endianness to big\n", argv[0]);
        return 1;
    }

    qDebug() << "Adapter args: " << app.arguments();

    Adapter adapter;
    adapter.setTrkServerName(options.trkServer);
    adapter.setGdbServerName(options.gdbServer);
    adapter.setVerbose(options.verbose);
    adapter.setBufferedMemoryRead(options.bufferedMemoryRead);
    adapter.setRegisterEndianness(options.registerEndianness);
    adapter.setUseSocket(options.useSocket);
    adapter.setSerialFrame(options.serialFrame);
    QTimer::singleShot(0, &adapter, SLOT(startServer()));
    return app.exec();
}

#include "adapter.moc"
