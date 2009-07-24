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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "trkutils.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QQueue>
#include <QtCore/QTimer>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

#if USE_NATIVE
#include <windows.h>
#endif

#ifdef Q_OS_UNIX

#include <signal.h>

void signalHandler(int)
{
    qApp->exit(1);
}

#endif

using namespace trk;

enum { TRK_SYNC = 0x7f };

#define CB(s) &Adapter::s

class Adapter : public QObject
{
    Q_OBJECT

public:
    Adapter();
    ~Adapter();
    void setGdbServerName(const QString &name);
    void setTrkServerName(const QString &name) { m_trkServerName = name; }
    bool startServer();

private:
    //
    // TRK
    //
    Q_SLOT void readFromTrk();

    typedef void (Adapter::*TrkCallBack)(const TrkResult &);

    struct TrkMessage 
    {
        TrkMessage() { code = token = 0; callBack = 0; }
        byte code;
        byte token;
        QByteArray data;
        QVariant cookie;
        TrkCallBack callBack;
    };

    bool openTrkPort(const QString &port); // or server name for local server
    void sendTrkMessage(byte code,
        TrkCallBack callBack = 0,
        const QByteArray &data = QByteArray(),
        const QVariant &cookie = QVariant());
    // adds message to 'send' queue
    void queueTrkMessage(const TrkMessage &msg);
    void tryTrkWrite();
    void tryTrkRead();
    // actually writes a message to the device
    void trkWrite(const TrkMessage &msg);
    // convienience messages
    void sendTrkInitialPing();
    void waitForTrkFinished();
    void sendTrkAck(byte token);

    // kill process and breakpoints
    void cleanUp();

    void timerEvent(QTimerEvent *ev);
    byte nextTrkWriteToken();

    void handleCpuType(const TrkResult &result);
    void handleCreateProcess(const TrkResult &result);
    void handleSetBreakpoint(const TrkResult &result);
    void handleClearBreakpoint(const TrkResult &result);
    void handleSignalContinue(const TrkResult &result);
    void handleWaitForFinished(const TrkResult &result);
    void handleStop(const TrkResult &result);
    void handleSupportMask(const TrkResult &result);

    void handleAndReportCreateProcess(const TrkResult &result);
    void handleAndReportReadRegisters(const TrkResult &result);
    void handleReadMemory(const TrkResult &result);
    void reportReadMemory(const TrkResult &result);
    void reportToGdb(const TrkResult &result);

    void setTrkBreakpoint(const Breakpoint &bp);
    void clearTrkBreakpoint(const Breakpoint &bp);
    void handleResult(const TrkResult &data);
    void readMemory(uint addr, uint len);
    void startInferiorIfNeeded();
    void interruptInferior();

#if USE_NATIVE
    HANDLE m_hdevice;
#else
     QLocalSocket *m_trkDevice;
#endif

    QString m_trkServerName;
    QByteArray m_trkReadBuffer;

    unsigned char m_trkWriteToken;
    QQueue<TrkMessage> m_trkWriteQueue;
    QHash<byte, TrkMessage> m_writtenTrkMessages;
    QByteArray m_trkReadQueue;
    bool m_trkWriteBusy;

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

    //
    void logMessage(const QString &msg);

    QTcpServer m_gdbServer;
    QTcpSocket *m_gdbConnection;
    QString m_gdbServerName;
    quint16 m_gdbServerPort;
    QByteArray m_gdbReadBuffer;
    bool m_gdbAckMode;

    // Debuggee state
    Session m_session; // global-ish data (process id, target information)
    Snapshot m_snapshot; // local-ish data (memory and registers)
};

Adapter::Adapter()
{
    // Trk
#if USE_NATIVE
    m_hdevice = NULL;
#else
    m_trkDevice = 0;
#endif
    m_trkWriteToken = 0;
    m_trkWriteBusy = false;
    //m_breakpoints.append(Breakpoint(0x0370));
    //m_breakpoints.append(Breakpoint(0x0340));
    //m_breakpoints.append(Breakpoint(0x0040)); // E32Main
    startTimer(100);

    // Gdb
    m_gdbConnection = 0;
    m_gdbAckMode = true;
}

Adapter::~Adapter()
{
    // Trk
#if USE_NATIVE
    CloseHandle(m_hdevice);
#else
    m_trkDevice->abort();
    delete m_trkDevice;
#endif

    // Gdb
    m_gdbServer.close();
    //>disconnectFromServer();
    logMessage("Shutting down.\n");
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

bool Adapter::startServer()
{
    if (!openTrkPort(m_trkServerName)) {
        logMessage("Unable to connect to TRK server");
        return false;
    }

    sendTrkInitialPing();
    sendTrkMessage(0x01); // Connect
    sendTrkMessage(0x05, CB(handleSupportMask));
    sendTrkMessage(0x06, CB(handleCpuType));
    sendTrkMessage(0x04); // Versions
    sendTrkMessage(0x09); // Unrecognized command
    //sendTrkMessage(0x4a, 0,
    //    "10 " + formatString("C:\\data\\usingdlls.sisx")); // Open File
    //sendTrkMessage(0x4B, 0, "00 00 00 01 73 1C 3A C8"); // Close File

    logMessage("Connected to TRK server");

    if (!m_gdbServer.listen(QHostAddress(m_gdbServerName), m_gdbServerPort)) {
        logMessage(QString("Unable to start the gdb server at %1:%2: %3.")
            .arg(m_gdbServerName).arg(m_gdbServerPort)
            .arg(m_gdbServer.errorString()));
        return false;
    }

    logMessage(QString("Gdb server running on port %1. Run arm-gdb now.")
        .arg(m_gdbServer.serverPort()));

    connect(&m_gdbServer, SIGNAL(newConnection()),
        this, SLOT(handleGdbConnection()));
    return true;
}

void Adapter::logMessage(const QString &msg)
{
    qDebug() << "ADAPTER: " << qPrintable(msg);
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
}

void Adapter::readFromGdb()
{
    QByteArray packet = m_gdbConnection->readAll();
    m_gdbReadBuffer.append(packet);

    logMessage("gdb: -> " + packet);
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
        ba = ba.mid(pos + 3);
        handleGdbResponse(response);
    }
}

void Adapter::sendGdbAckMessage()
{
    if (!m_gdbAckMode)
        return;
    QByteArray packet = "+";
    logMessage("gdb: <- " + packet);
    m_gdbConnection->write(packet);
    //m_gdbConnection->flush();
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
    m_gdbConnection->write(packet);
    m_gdbConnection->flush();
}


void Adapter::sendGdbMessageAfterSync(const QByteArray &msg, const QByteArray &logNote)
{
    QByteArray ba = msg + char(1) + logNote;
    sendTrkMessage(TRK_SYNC, CB(reportToGdb), "", ba); // Answer gdb
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
        // Indicate the reason the target halted.
        // The reply is the same as for step and continue.
        sendGdbAckMessage();
        startInferiorIfNeeded();
        //sendGdbMessageAfterSync("T05thread:p@PID@.@TID@;", "current thread Id");
        //sendGdbMessageAfterSync("T05thread:@TID@;", "current thread Id");
        //sendGdbMessage("T0505:00000000;04:e0c1ddbf;08:10f80bb8;thread:1f43;",
        //    sendGdbMessage("T0505:00000000;thread:" + QByteArray(buf),
        sendGdbMessage("S05", "target halted");
    }

    else if (response == "c") {
        sendGdbAckMessage();
        QByteArray ba;
        appendByte(&ba, 0); // options
        appendInt(&ba, 0); // start address
        appendInt(&ba, 0); // end address
        appendInt(&ba, m_session.pid);
        appendInt(&ba, m_session.tid);
        sendTrkMessage(0x18, 0, ba);
        // FIXME: should be triggered by real stop
        //sendGdbMessageAfterSync("S11", "target stopped");
    }

    else if (response.startsWith("C")) {
        // C sig[;addr] Continue with signal sig (hex signal number)
        //Reply: See section D.3 Stop Reply Packets, for the reply specifications. 
        sendGdbAckMessage();
        bool ok = false;
        uint signalNumber = response.mid(1).toInt(&ok, 16);
        QByteArray ba;
        appendInt(&ba, m_session.pid);
        appendInt(&ba, m_session.tid);
        sendTrkMessage(0x18, CB(handleSignalContinue), ba, signalNumber); // Continue
    }

    else if (response.startsWith("D")) {
        sendGdbAckMessage();
        sendGdbMessage("OK", "shutting down");
        qApp->exit(1);
    }

    else if (response == "g") {
        // Read general registers.
        //sendGdbMessage("00000000", "read registers");
        sendGdbAckMessage();
        QByteArray ba;
        appendByte(&ba, 0); // ?
        appendByte(&ba, 0); // ?
        appendByte(&ba, 0); // ?

        appendByte(&ba, 0); // first register
        // FIXME: off by one?
        appendByte(&ba, RegisterCount - 1); // last register
        appendInt(&ba, m_session.pid);
        appendInt(&ba, m_session.tid);
        
        sendTrkMessage(0x12, CB(handleAndReportReadRegisters), ba);
    }

    else if (response.startsWith("Hc")) {
        // Set thread for subsequent operations (`m', `M', `g', `G', et.al.).
        // for step and continue operations
        //$Hc-1#09
        sendGdbAckMessage();
        sendGdbMessage("OK", "set current thread for step & continue");
    }

    else if (response.startsWith("Hg")) {
        // Set thread for subsequent operations (`m', `M', `g', `G', et.al.).
        // for 'other operations.  0 - any thread
        //$Hg0#df
        sendGdbAckMessage();
        m_session.currentThread = response.mid(2).toInt();
        sendGdbMessage("OK", "set current thread "
            + QByteArray::number(m_session.currentThread));
    }

    else if (response == "k") {
        // kill
        sendGdbAckMessage();
        QByteArray ba;
        appendByte(&ba, 0); // Sub-command: Delete Process
        appendInt(&ba, m_session.pid);
        sendTrkMessage(0x41, 0, ba, "Delete process"); // Delete Item
        sendGdbMessageAfterSync("", "process killed");
    }

    else if (response.startsWith("m")) {
        // m addr,length
        sendGdbAckMessage();
        int pos = response.indexOf(',');
        bool ok = false;
        uint addr = response.mid(1, pos - 1).toInt(&ok, 16);
        uint len = response.mid(pos + 1).toInt(&ok, 16);
        //qDebug() << "GDB ADDR: " << hexNumber(addr) << " " << hexNumber(len);
        readMemory(addr, len);
    }

    else if (response.startsWith("p")) {
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
        uint registerNumber = response.mid(1).toInt(&ok, 16);
        if (registerNumber == RegisterPSGdb) {
            QByteArray ba;
            appendInt(&ba, m_snapshot.registers[RegisterPSTrk]);
            sendGdbMessage(ba.toHex(), "read processor status register");
        } else if (registerNumber < RegisterCount) {
            QByteArray ba;
            appendInt(&ba, m_snapshot.registers[registerNumber]);
            sendGdbMessage(ba.toHex(), "read single known register");
        } else {
            sendGdbMessage("0000", "read single unknown register");
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
                "QPassSignals+;"
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

    else if (response == "s") {
        sendGdbAckMessage();
        QByteArray ba;
        appendByte(&ba, 0); // options
        appendInt(&ba, m_snapshot.registers[RegisterPC]); // start address
        appendInt(&ba, m_snapshot.registers[RegisterPC] + 4); // end address
        appendInt(&ba, m_session.pid);
        appendInt(&ba, m_session.tid);
        sendTrkMessage(0x19, 0, ba, "Step range");
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
        sendTrkMessage(0x41, 0, ba, "Delete process"); // Delete Item
        sendGdbMessageAfterSync("", "process killed");
    }

    else {
        logMessage("FIXME unknown: " + response);
    }
}

void Adapter::readFromTrk()
{
    //QByteArray ba = m_gdbConnection->readAll();
    //logMessage("Read from gdb: " + ba);
}

bool Adapter::openTrkPort(const QString &port)
{
#if USE_NATIVE
    m_hdevice = CreateFile(port.toStdWString().c_str(),
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

    if (INVALID_HANDLE_VALUE == m_hdevice){
        logMessage("Could not open device " + port);
        return false;
     }
    return true;
#else
#if 0
    m_trkDevice = new Win_QextSerialPort(port);
    m_trkDevice->setBaudRate(BAUD115200);
    m_trkDevice->setDataBits(DATA_8);
    m_trkDevice->setParity(PAR_NONE);
    //m_trkDevice->setStopBits(STO);
    m_trkDevice->setFlowControl(FLOW_OFF);
    m_trkDevice->setTimeout(0, 500);

    if (!m_trkDevice->open(QIODevice::ReadWrite)) {
        QByteArray ba = m_trkDevice->errorString().toLatin1();
        logMessage("Could not open device " << ba);
        return false;
    }
    return true
#else
    m_trkDevice = new QLocalSocket(this);
    m_trkDevice->connectToServer(port);
    return m_trkDevice->waitForConnected();
#endif
#endif
}

void Adapter::timerEvent(QTimerEvent *)
{
    //qDebug(".");
    tryTrkWrite();
    tryTrkRead();
}

byte Adapter::nextTrkWriteToken()
{
    ++m_trkWriteToken;
    if (m_trkWriteToken == 0)
        ++m_trkWriteToken;
    return m_trkWriteToken;
}

void Adapter::sendTrkMessage(byte code, TrkCallBack callBack,
    const QByteArray &data, const QVariant &cookie)
{
    TrkMessage msg;
    msg.code = code;
    msg.token = nextTrkWriteToken();
    msg.callBack = callBack;
    msg.data = data;
    msg.cookie = cookie;
    queueTrkMessage(msg);
}

void Adapter::sendTrkInitialPing()
{
    TrkMessage msg;
    msg.code = 0x00; // Ping
    msg.token = 0; // reset sequence count
    queueTrkMessage(msg);
}

void Adapter::waitForTrkFinished()
{
    TrkMessage msg;
    // initiate one last roundtrip to ensure all is flushed
    msg.code = 0x00; // Ping
    msg.token = nextTrkWriteToken();
    msg.callBack = CB(handleWaitForFinished);
    queueTrkMessage(msg);
}

void Adapter::sendTrkAck(byte token)
{
    logMessage(QString("SENDING ACKNOWLEDGEMENT FOR TOKEN %1").arg(int(token)));
    TrkMessage msg;
    msg.code = 0x80;
    msg.token = token;
    msg.data.append('\0');
    // The acknowledgement must not be queued!
    //queueMessage(msg);
    trkWrite(msg);
    // 01 90 00 07 7e 80 01 00 7d 5e 7e
}

void Adapter::queueTrkMessage(const TrkMessage &msg)
{
    m_trkWriteQueue.append(msg);
}

void Adapter::tryTrkWrite()
{
    if (m_trkWriteBusy)
        return;
    if (m_trkWriteQueue.isEmpty())
        return;

    TrkMessage msg = m_trkWriteQueue.dequeue();
    if (msg.code == TRK_SYNC) {
        //logMessage("TRK SYNC");
        TrkResult result;
        result.code = msg.code;
        result.token = msg.token;
        result.data = msg.data;
        result.cookie = msg.cookie;
        TrkCallBack cb = msg.callBack;
        if (cb)
            (this->*cb)(result);
    } else {
        trkWrite(msg);
    }
}

void Adapter::trkWrite(const TrkMessage &msg)
{
    QByteArray ba = frameMessage(msg.code, msg.token, msg.data);

    m_writtenTrkMessages.insert(msg.token, msg);
    m_trkWriteBusy = true;

    logMessage("WRITE: " + stringFromArray(ba));

#if USE_NATIVE
    DWORD charsWritten;
    if (!WriteFile(m_hdevice, ba.data(), ba.size(), &charsWritten, NULL))
        logMessage("WRITE ERROR: ");

    FlushFileBuffers(m_hdevice);
#else
    if (!m_trkDevice->write(ba))
        logMessage("WRITE ERROR: " + m_trkDevice->errorString());
    m_trkDevice->flush();

#endif
}

void Adapter::tryTrkRead()
{
    //logMessage("TRY READ: " << m_trkDevice->bytesAvailable()
    //        << stringFromArray(m_trkReadQueue);

#if USE_NATIVE
    const DWORD BUFFERSIZE = 1;
    char buffer[BUFFERSIZE];
    DWORD charsRead;

    while (ReadFile(m_hdevice, buffer, BUFFERSIZE, &charsRead, NULL)) {
        m_trkReadQueue.append(buffer, charsRead);
        if (isValidTrkResult(m_trkReadQueue))
            break;
    }
#else // USE_NATIVE

    if (m_trkDevice->bytesAvailable() == 0 && m_trkReadQueue.isEmpty())
        return;

    QByteArray res = m_trkDevice->readAll();
    m_trkReadQueue.append(res);


#endif // USE_NATIVE

    if (m_trkReadQueue.size() < 9) {
        logMessage("ERROR READBUFFER INVALID (1): "
            + stringFromArray(m_trkReadQueue));
        m_trkReadQueue.clear();
        return;
    }

    while (!m_trkReadQueue.isEmpty())
        handleResult(extractResult(&m_trkReadQueue));

    m_trkWriteBusy = false;
}


void Adapter::handleResult(const TrkResult &result)
{
    QByteArray prefix = "READ BUF:                                       ";
    QByteArray str = result.toString().toUtf8();
    switch (result.code) {
        case 0x80: { // ACK
            //logMessage(prefix + "ACK: " + str);
            if (!result.data.isEmpty() && result.data.at(0))
                logMessage(prefix + "ERR: " +QByteArray::number(result.data.at(0)));
            //logMessage("READ RESULT FOR TOKEN: " << token);
            if (!m_writtenTrkMessages.contains(result.token)) {
                logMessage("NO ENTRY FOUND!");
            }
            TrkMessage msg = m_writtenTrkMessages.take(result.token);
            TrkResult result1 = result;
            result1.cookie = msg.cookie;
            TrkCallBack cb = msg.callBack;
            if (cb) {
                //logMessage("HANDLE: " << stringFromArray(result.data));
                (this->*cb)(result1);
            } else {
                QString msg = result.cookie.toString();
                if (!msg.isEmpty())
                    logMessage("HANDLE: " + msg + stringFromArray(result.data));
            }
            break;
        }
        case 0xff: { // NAK
            logMessage(prefix + "NAK: " + str);
            //logMessage(prefix << "TOKEN: " << result.token);
            logMessage(prefix + "ERROR: " + errorMessage(result.data.at(0)));
            break;
        }
        case 0x90: { // Notified Stopped
            logMessage(prefix + "NOTE: STOPPED  " + str);
            // 90 01   78 6a 40 40   00 00 07 23   00 00 07 24  00 00
            //const char *data = result.data.data();
//            uint addr = extractInt(data); //code address: 4 bytes; code base address for the library
//            uint pid = extractInt(data + 4); // ProcessID: 4 bytes;
//            uint tid = extractInt(data + 8); // ThreadID: 4 bytes
            //logMessage(prefix << "      ADDR: " << addr << " PID: " << pid << " TID: " << tid);
            sendTrkAck(result.token);
            //sendGdbMessage("S11", "Target stopped");
            sendGdbMessage("S05", "Target stopped");
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
            /*
            const char *data = result.data.data();
            byte error = result.data.at(0);
            byte type = result.data.at(1); // type: 1 byte; for dll item, this value is 2.
            uint pid = extractInt(data + 2); //  ProcessID: 4 bytes;
            uint tid = extractInt(data + 6); //threadID: 4 bytes
            uint codeseg = extractInt(data + 10); //code address: 4 bytes; code base address for the library
            uint dataseg = extractInt(data + 14); //data address: 4 bytes; data base address for the library
            uint len = extractShort(data + 18); //length: 2 bytes; length of the library name string to follow
            QByteArray name = result.data.mid(20, len); // name: library name

            logMessage(prefix + "NOTE: LIBRARY LOAD: " + str);
            logMessage(prefix + "TOKEN: " + result.token);
            logMessage(prefix + "ERROR: " + int(error));
            logMessage(prefix + "TYPE:  " + int(type));
            logMessage(prefix + "PID:   " + pid);
            logMessage(prefix + "TID:   " + tid);
            logMessage(prefix + "CODE:  " + codeseg);
            logMessage(prefix + "DATA:  " + dataseg);
            logMessage(prefix + "LEN:   " + len);
            logMessage(prefix + "NAME:  " + name);
            */

            QByteArray ba;
            appendInt(&ba, m_session.pid);
            appendInt(&ba, m_session.tid);
            sendTrkMessage(0x18, 0, ba, "CONTINUE");
            //sendTrkAck(result.token)
            break;
        }
        case 0xa1: { // NotifyDeleted
            logMessage(prefix + "NOTE: LIBRARY UNLOAD: " + str);
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
    logMessage("HANDLE CPU TYPE: " + result.toString());
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 03 00  04 00 00 04 00 00 00]
    m_session.cpuMajor = result.data[0];
    m_session.cpuMinor = result.data[1];
    m_session.bigEndian = result.data[2];
    m_session.defaultTypeSize = result.data[3];
    m_session.fpTypeSize = result.data[4];
    m_session.extended1TypeSize = result.data[5];
    //m_session.extended2TypeSize = result.data[6];
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
    qDebug() << "    READ PID: " << m_session.pid;
    qDebug() << "    READ TID: " << m_session.tid;
    qDebug() << "    READ CODE: " << m_session.codeseg;
    qDebug() << "    READ DATA: " << m_session.dataseg;

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
    foreach (const Breakpoint &bp, m_breakpoints)
        setTrkBreakpoint(bp);
#endif

#if 1
    //---IDE------------------------------------------------------
    //  Command: 0x42 Read Info
    //          [42 0C 00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F
    //  72 70 68 69 63 44 4C 4C 32 2E 64 6C 6C 00]
    sendTrkMessage(0x42, CB(handleReadInfo),
        "00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F "
        "72 70 68 69 63 44 4C 4C 32 2E 64 6C 6C 00");
    //sendTrkMessage(0x42, CB(handleReadInfo),
    //        "00 01 00 00 00 00");
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x20 Unspecified general OS-related error
    // [80 0C 20]


    //---IDE------------------------------------------------------
    //  Command: 0x42 Read Info
    // [42 0D 00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F
    //  72 70 68 69 63 44 4C 4C 31 2E 64 6C 6C 00]
    sendTrkMessage(0x42, CB(handleReadInfo),
        "00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F "
        "72 70 68 69 63 44 4C 4C 31 2E 64 6C 6C 00");
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x20 Unspecified general OS-related error
    // [80 0D 20]
#endif

    //sendTrkMessage(0x18, CB(handleStop),
    //    "01 " + formatInt(m_session.pid) + formatInt(m_session.tid));

    //---IDE------------------------------------------------------
    //  Command: 0x18 Continue
    //ProcessID: 0x000001B5 (437)
    // ThreadID: 0x000001B6 (438)
    // [18 0E 00 00 01 B5 00 00 01 B6]
    QByteArray ba;
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    sendTrkMessage(0x18, CB(handleContinue), ba);
    //sendTrkMessage(0x18, CB(handleContinue),
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
    const char *data = result.data.data();
    for (int i = 0; i < RegisterCount; ++i) {
        m_snapshot.registers[i] = extractInt(data + 4 * i);
        //qDebug() << i << hexNumber(m_snapshot.registers[i], 8);
    }
    //QByteArray ba = result.data.toHex();
    QByteArray ba;
    for (int i = 0; i < 16; ++i)
        ba += hexNumber(m_snapshot.registers[i], 8);
    sendGdbMessage(ba, "register contents");
}

void Adapter::handleReadMemory(const TrkResult &result)
{
    //logMessage("       RESULT READ MEMORY: " + result.data.toHex());
    QByteArray ba = result.data.mid(1);
    uint blockaddr = result.cookie.toInt();
    //qDebug() << "READING " << ba.size() << " BYTES: "
    //    << quoteUnprintableLatin1(ba)
    //    << "ADDR: " << hexNumber(blockaddr)
    //    << "COOKIE: " << result.cookie;
    m_snapshot.memory[blockaddr] = ba;
}

void Adapter::reportReadMemory(const TrkResult &result)
{
    qulonglong cookie = result.cookie.toLongLong();
    uint addr = cookie >> 32;
    uint len = uint(cookie);

    QByteArray ba;
    uint blockaddr = (addr / MemoryChunkSize) * MemoryChunkSize;
    for (; blockaddr < addr + len; blockaddr += MemoryChunkSize) {
        QByteArray blockdata = m_snapshot.memory[blockaddr];
        Q_ASSERT(!blockdata.isEmpty());
        ba.append(blockdata);
    }

    ba = ba.mid(addr % MemoryChunkSize, len);
    // qDebug() << "REPORTING MEMORY " << ba.size()
    //     << " ADDR: " << hexNumber(blockaddr) << " LEN: " << len
    //     << " BYTES: " << quoteUnprintableLatin1(ba);

    sendGdbMessage(ba.toHex(), "memory contents");
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
    QByteArray ba;
    appendByte(&ba, 0x82);
    appendByte(&ba, bp.mode == ArmMode ? 0x00 : 0x01);
    appendInt(&ba, m_session.codeseg + bp.offset);
    appendInt(&ba, 0x00000001);
    appendInt(&ba, 0x00000001);
    appendInt(&ba, m_session.pid);
    appendInt(&ba, 0xFFFFFFFF);

    sendTrkMessage(0x1B, CB(handleSetBreakpoint), ba);
    //m_session.toekn

    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 09 00 00 00 00 0A]
}

void Adapter::handleSetBreakpoint(const TrkResult &result)
{
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 09 00 00 00 00 0A]
    uint bpnr = extractInt(result.data.data());
    logMessage("SET BREAKPOINT " + bpnr
        + stringFromArray(result.data.data()));
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
    sendTrkMessage(0x1C, CB(handleClearBreakpoint), ba);
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


void Adapter::cleanUp()
{
    //
    //---IDE------------------------------------------------------
    //  Command: 0x41 Delete Item
    //  Sub Cmd: Delete Process
    //ProcessID: 0x0000071F (1823)
    // [41 24 00 00 00 00 07 1F]
    QByteArray ba;
    appendByte(&ba, 0x00);
    appendByte(&ba, 0x00);
    appendInt(&ba, m_session.pid);
    sendTrkMessage(0x41, 0, ba, "Delete process");

    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 24 00]

    foreach (const Breakpoint &bp, m_breakpoints)
        clearTrkBreakpoint(bp);

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
//    sendTrkMessage(0x02, CB(handleDisconnect));
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    // Error: 0x00
}

void Adapter::readMemory(uint addr, uint len)
{
    Q_ASSERT(len < (2 << 16));

    // We try to get medium-sized chunks of data from the device

    QList <uint> blocksToFetch;
    uint blockaddr = (addr / MemoryChunkSize) * MemoryChunkSize;
    for (; blockaddr < addr + len; blockaddr += MemoryChunkSize) {
        QByteArray blockdata = m_snapshot.memory[blockaddr];
        if (blockdata.isEmpty()) {
            // fetch it
            QByteArray ba;
            appendByte(&ba, 0x08); // Options, FIXME: why?
            appendShort(&ba, MemoryChunkSize);
            appendInt(&ba, blockaddr);
            appendInt(&ba, m_session.pid);
            appendInt(&ba, m_session.tid);
            // Read Memory
            sendTrkMessage(0x10, CB(handleReadMemory), ba, QVariant(blockaddr));
        }
    }
    qulonglong cookie = (qulonglong(addr) << 32) + len;
    sendTrkMessage(TRK_SYNC, CB(reportReadMemory), QByteArray(), cookie);
}

void Adapter::startInferiorIfNeeded()
{
    if (m_session.pid != 0) {
        qDebug() << "Process already 'started'";
        return;
    }
    // It's not started yet
    QByteArray ba;
    appendByte(&ba, 0); // ?
    appendByte(&ba, 0); // ?
    appendByte(&ba, 0); // ?

    QByteArray file("C:\\sys\\bin\\filebrowseapp.exe");
    file.append('\0');
    file.append('\0');
    appendString(&ba, file, TargetByteOrder);
    sendTrkMessage(0x40, CB(handleCreateProcess), ba); // Create Item
}

void Adapter::interruptInferior()
{
    QByteArray ba;
    // stop the thread (2) or the process (1) or the whole system (0)
    appendByte(&ba, 1);
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid); // threadID: 4 bytes Variable number of bytes.
    sendTrkMessage(0x1A, 0, ba, "Interrupting...");
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        qDebug() << "Usage: " << argv[0] << " <trkservername> <gdbserverport>";
        return 1;
    }

#ifdef Q_OS_UNIX
    signal(SIGUSR1, signalHandler);
#endif

    QCoreApplication app(argc, argv);

    Adapter adapter;
    adapter.setTrkServerName(argv[1]);
    adapter.setGdbServerName(argv[2]);
    if (adapter.startServer())
        return app.exec();
    return 4;
}

#include "adapter.moc"
