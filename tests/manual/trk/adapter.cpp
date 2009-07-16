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

#ifdef Q_OS_UNIX

#include <signal.h>

void signalHandler(int)
{
    qApp->exit(1);
}

#endif

using namespace trk;

#define CB(s) &Adapter::s


class Adapter : public QObject
{
    Q_OBJECT

public:
    Adapter();
    ~Adapter();
    void setGdbServerName(const QString &name);
    void setTrkServerName(const QString &name) { m_trkServerName = name; }
    void startServer();


private:
    //
    // TRK
    //
    Q_SLOT void readFromTrk();

    typedef void (Adapter::*TrkCallBack)(const TrkResult &);

    struct TrkMessage 
    {
        TrkMessage() { token = 0; callBack = 0; }
        byte command;
        byte token;
        QByteArray data;
        TrkCallBack callBack;
    };

    bool openTrkPort(const QString &port); // or server name for local server
    void sendTrkMessage(byte command, TrkCallBack callBack = 0,
        const QByteArray &lit = QByteArray());
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

    void handleTrkCpuType(const TrkResult &result);
    void handleTrkCreateProcess(const TrkResult &result);
    void handleTrkDeleteProcess(const TrkResult &result);
    void handleTrkSetBreakpoint(const TrkResult &result);
    void handleTrkClearBreakpoint(const TrkResult &result);
    void handleTrkContinue(const TrkResult &result);
    void handleTrkReadInfo(const TrkResult &result);
    void handleTrkWaitForFinished(const TrkResult &result);
    void handleTrkStep(const TrkResult &result);
    void handleTrkStop(const TrkResult &result);
    void handleTrkReadRegisters(const TrkResult &result);
    void handleTrkWriteRegisters(const TrkResult &result);
    void handleTrkReadMemory(const TrkResult &result);
    void handleTrkWriteMemory(const TrkResult &result);
    void handleTrkSupportMask(const TrkResult &result);
    void handleTrkDisconnect(const TrkResult &result);


    void setTrkBreakpoint(const Breakpoint &bp);
    void clearTrkBreakpoint(const Breakpoint &bp);
    void handleTrkResult(const TrkResult &data);

    QLocalSocket *m_trkDevice;

    QString m_trkServerName;
    QByteArray m_trkReadBuffer;

    unsigned char m_trkWriteToken;
    QQueue<TrkMessage> m_trkWriteQueue;
    QHash<byte, TrkMessage> m_writtenTrkMessages;
    QByteArray m_trkReadQueue;
    bool m_trkWriteBusy;

    QList<Breakpoint> m_breakpoints;
    Session m_session;

    //
    // Gdb
    //
    Q_SLOT void handleGdbConnection();
    Q_SLOT void readFromGdb();
    void handleGdbResponse(const QByteArray &ba);
    void writeToGdb(const QByteArray &msg, bool addAck = true);
    void writeAckToGdb();

    //
    void logMessage(const QString &msg);

    QTcpServer m_gdbServer;
    QTcpSocket *m_gdbConnection;
    QString m_gdbServerName;
    quint16 m_gdbServerPort;
    QByteArray m_gdbReadBuffer;
    bool m_gdbAckMode;

    uint m_registers[100];
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
    delete m_trkDevice;
#endif

    // Gdb
    m_gdbServer.close();
    //>disconnectFromServer();
    m_trkDevice->abort();
    logMessage("Shutting down.\n");
}

void Adapter::setGdbServerName(const QString &name)
{
    int pos = name.indexOf(':');
    if (pos == -1) {
        m_gdbServerPort = 0;
        m_gdbServerName = name;
    } else {
        m_gdbServerPort = name.mid(pos + 1).toInt();
        m_gdbServerName = name.left(pos);
    }
}

void Adapter::startServer()
{
    if (!openTrkPort(m_trkServerName)) {
        logMessage("Unable to connect to TRK server");
        return;
    }

    sendTrkInitialPing();
    sendTrkMessage(0x01); // Connect
    sendTrkMessage(0x05, CB(handleTrkSupportMask));
    sendTrkMessage(0x06, CB(handleTrkCpuType));
    sendTrkMessage(0x04); // Versions
    sendTrkMessage(0x09); // Unrecognized command
    sendTrkMessage(0x4a, 0,
        "10 " + formatString("C:\\data\\usingdlls.sisx")); // Open File
    sendTrkMessage(0x4B, 0, "00 00 00 01 73 1C 3A C8"); // Close File

    QByteArray exe = "C:\\sys\\bin\\UsingDLLs.exe";
    exe.append('\0');
    exe.append('\0');
    sendTrkMessage(0x40, CB(handleTrkCreateProcess),
        "00 00 00 " + formatString(exe)); // Create Item

    logMessage("Connected to TRK server");

return;

    if (!m_gdbServer.listen(QHostAddress(m_gdbServerName), m_gdbServerPort)) {
        logMessage(QString("Unable to start the gdb server at %1:%2: %3.")
            .arg(m_gdbServerName).arg(m_gdbServerPort)
            .arg(m_gdbServer.errorString()));
        return;
    }

    logMessage(QString("Gdb server running on port %1. Run arm-gdb now.")
        .arg(m_gdbServer.serverPort()));

    connect(&m_gdbServer, SIGNAL(newConnection()),
        this, SLOT(handleGdbConnection()));
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

        if (code != '$') {
            logMessage("Broken package (2) " + ba);
            continue;
        }

        int pos = ba.indexOf('#');
        if (pos == -1) {
            logMessage("Invalid checksum format in " + ba);
            continue;
        }

        bool ok = false;
        uint checkSum = ba.mid(pos + 1, 2).toInt(&ok, 16);
        if (!ok) {
            logMessage("Invalid checksum format 2 in " + ba);
            return;
        }

        //logMessage(QString("Packet checksum: %1").arg(checkSum));
        uint sum = 0;
        for (int i = 0; i < pos; ++i)
            sum += ba.at(i);

        if (sum % 256 != checkSum) {
            logMessage(QString("ERROR: Packet checksum wrong: %1 %2 in " + ba)
                .arg(checkSum).arg(sum % 256));
        }

        QByteArray response = ba.left(pos);
        ba = ba.mid(pos + 3);
        handleGdbResponse(response);
    }
}

void Adapter::writeAckToGdb()
{
    if (!m_gdbAckMode)
        return;
    QByteArray packet = "+";
    logMessage("gdb: <- " + packet);
    m_gdbConnection->write(packet);
}

void Adapter::writeToGdb(const QByteArray &msg, bool addAck)
{
    uint sum = 0;
    for (int i = 0; i != msg.size(); ++i)
        sum += msg.at(i);
    QByteArray checkSum = QByteArray::number(sum % 256, 16);
    //logMessage(QString("Packet checksum: %1").arg(sum));

    QByteArray packet;
    if (addAck)
        packet.append("+");
    packet.append("$");
    packet.append(msg);
    packet.append('#');
    if (checkSum.size() < 2)
        packet.append('0');
    packet.append(checkSum);
    logMessage("gdb: <- " + packet);
    m_gdbConnection->write(packet);
}

void Adapter::handleGdbResponse(const QByteArray &response)
{
    // http://sourceware.org/gdb/current/onlinedocs/gdb_34.html
    if (response == "qSupported") {
        //$qSupported#37
        //logMessage("Handling 'qSupported'");
        writeToGdb(QByteArray());
    }

    else if (response == "qAttached") {
        //$qAttached#8f
        // 1: attached to an existing process
        // 0: created a new process
        writeToGdb("0");
    }

    else if (response == "QStartNoAckMode") {
        //$qSupported#37
        //logMessage("Handling 'QStartNoAckMode'");
        writeToGdb(QByteArray("OK"));
        m_gdbAckMode = false;
    }

    else if (response == "g") {
        // Read general registers.
        writeToGdb("00000000");
    }

    else if (response.startsWith("Hc")) {
        // Set thread for subsequent operations (`m', `M', `g', `G', et.al.).
        // for step and continue operations
        //$Hc-1#09
        writeToGdb("OK");
    }

    else if (response.startsWith("Hg")) {
        // Set thread for subsequent operations (`m', `M', `g', `G', et.al.).
        // for 'other operations.  0 - any thread
        //$Hg0#df
        writeToGdb("OK");
    }

    else if (response == "pf") {
        // current instruction pointer?
        writeToGdb("0000");
    }

    else if (response.startsWith("qC")) {
        // Return the current thread ID
        //$qC#b4
        writeToGdb("QC-1");
    }

    else if (response.startsWith("?")) {
        // Indicate the reason the target halted.
        // The reply is the same as for step and continue.
        writeToGdb("S0b");
        //$?#3f
        //$qAttached#8f
        //$qOffsets#4b
        //$qOffsets#4b

    } else {
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
    // QFile does not work with "COM3", so work around
    /*
    FILE *f = fopen("COM3", "r+");
    if (!f) {
        logMessage("Could not open file ");
        return;
    }
    m_trkDevice = new QFile;
    if (!m_trkDevice->open(f, QIODevice::ReadWrite))
    */

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
        return;
    }
#else
    m_trkDevice = new QLocalSocket(this);
    m_trkDevice->connectToServer(port);
    return m_trkDevice->waitForConnected();
#endif
}

void Adapter::timerEvent(QTimerEvent *)
{
    //qDebug(".");
    tryTrkWrite();
    tryTrkRead();
}

unsigned char Adapter::nextTrkWriteToken()
{
    ++m_trkWriteToken;
    if (m_trkWriteToken == 0)
        ++m_trkWriteToken;
    return m_trkWriteToken;
}

void Adapter::sendTrkMessage(byte command, TrkCallBack callBack,
    const QByteArray &lit)
{
    TrkMessage msg;
    msg.command = command;
    msg.token = nextTrkWriteToken();
    msg.callBack = callBack;
    QList<QByteArray> list = lit.split(' ');
    foreach (const QByteArray &item, list) {
        if (item.isEmpty())
            continue;
        bool ok = false;
        int i = item.toInt(&ok, 16);
        msg.data.append(char(i));
    }
    //logMessage("PARSED: " << lit << " -> " << stringFromArray(data).toLatin1().data());
    queueTrkMessage(msg);
}

void Adapter::sendTrkInitialPing()
{
    TrkMessage msg;
    msg.command = 0x00; // Ping
    msg.token = 0; // reset sequence count
    queueTrkMessage(msg);
}

void Adapter::waitForTrkFinished()
{
    TrkMessage msg;
    // initiate one last roundtrip to ensure all is flushed
    msg.command = 0x00; // Ping
    msg.token = nextTrkWriteToken();
    msg.callBack = CB(handleTrkWaitForFinished);
    queueTrkMessage(msg);
}

void Adapter::sendTrkAck(byte token)
{
    logMessage(QString("SENDING ACKNOWLEDGEMENT FOR TOKEN ").arg(int(token)));
    TrkMessage msg;
    msg.command = 0x80;
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

    trkWrite(m_trkWriteQueue.dequeue());
}

void Adapter::trkWrite(const TrkMessage &msg)
{
    QByteArray ba = frameMessage(msg.command, msg.token, msg.data);

    m_writtenTrkMessages.insert(msg.token, msg);
    m_trkWriteBusy = true;

#if USE_NATIVE

    DWORD charsWritten;
    if (!WriteFile(m_hdevice, ba.data(), ba.size(), &charsWritten, NULL))
        logMessage("WRITE ERROR: ");

    logMessage("WRITE: " + stringFromArray(ba));
    FlushFileBuffers(m_hdevice);

#else

    logMessage("WRITE: " + stringFromArray(ba));
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

    const int BUFFERSIZE = 1024;
    char buffer[BUFFERSIZE];
    DWORD charsRead;

    while (ReadFile(m_hdevice, buffer, BUFFERSIZE, &charsRead, NULL)
            && BUFFERSIZE == charsRead) {
        m_trkReadQueue.append(buffer, charsRead);
    }
    m_trkReadQueue.append(buffer, charsRead);

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
        handleTrkResult(extractResult(&m_trkReadQueue));

    m_trkWriteBusy = false;
}


void Adapter::handleTrkResult(const TrkResult &result)
{
    QByteArray prefix = "READ BUF:                                       ";
    QByteArray str = result.toString().toUtf8();
    switch (result.code) {
        case 0x80: { // ACK
            logMessage(prefix + "ACK: " + str);
            if (!result.data.isEmpty() && result.data.at(0))
                logMessage(prefix + "ERR: " +QByteArray::number(result.data.at(0)));
            //logMessage("READ RESULT FOR TOKEN: " << token);
            if (!m_writtenTrkMessages.contains(result.token)) {
                logMessage("NO ENTRY FOUND!");
            }
            TrkMessage msg = m_writtenTrkMessages.take(result.token);
            TrkCallBack cb = msg.callBack;
            if (cb) {
                //logMessage("HANDLE: " << stringFromArray(result.data));
                (this->*cb)(result);
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
            logMessage(prefix + "NOTE: STOPPED" + str);
            // 90 01   78 6a 40 40   00 00 07 23   00 00 07 24  00 00
            //const char *data = result.data.data();
//            uint addr = extractInt(data); //code address: 4 bytes; code base address for the library
//            uint pid = extractInt(data + 4); // ProcessID: 4 bytes;
//            uint tid = extractInt(data + 8); // ThreadID: 4 bytes
            //logMessage(prefix << "      ADDR: " << addr << " PID: " << pid << " TID: " << tid);
            sendTrkAck(result.token);
            //Sleep(10000);
            //cleanUp();
            break;
        }
        case 0x91: { // Notify Exception (obsolete)
            logMessage(prefix + "NOTE: EXCEPTION" + str);
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

            sendTrkMessage(0x18, CB(handleTrkContinue),
                formatInt(m_session.pid) + formatInt(m_session.tid));

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

void Adapter::handleTrkCpuType(const TrkResult &result)
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

void Adapter::handleTrkCreateProcess(const TrkResult &result)
{
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 08 00  00 00 01 B5  00 00 01 B6  78 67 40 00  00
    //  40 00 00]

    logMessage("       RESULT: " + result.toString());
    const char *data = result.data.data();
    m_session.pid = extractInt(data);
    m_session.tid = extractInt(data + 4);
    m_session.codeseg = extractInt(data + 8);
    m_session.dataseg = extractInt(data + 12);

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
    sendTrkMessage(0x42, CB(handleTrkReadInfo),
        "00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F "
        "72 70 68 69 63 44 4C 4C 32 2E 64 6C 6C 00");
    //sendTrkMessage(0x42, CB(handleTrkReadInfo),
    //        "00 01 00 00 00 00");
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x20 Unspecified general OS-related error
    // [80 0C 20]


    //---IDE------------------------------------------------------
    //  Command: 0x42 Read Info
    // [42 0D 00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F
    //  72 70 68 69 63 44 4C 4C 31 2E 64 6C 6C 00]
    sendTrkMessage(0x42, CB(handleTrkReadInfo),
        "00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F "
        "72 70 68 69 63 44 4C 4C 31 2E 64 6C 6C 00");
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x20 Unspecified general OS-related error
    // [80 0D 20]
#endif

    //sendTrkMessage(0x18, CB(handleTrkStop),
    //    "01 " + formatInt(m_session.pid) + formatInt(m_session.tid));

    //---IDE------------------------------------------------------
    //  Command: 0x18 Continue
    //ProcessID: 0x000001B5 (437)
    // ThreadID: 0x000001B6 (438)
    // [18 0E 00 00 01 B5 00 00 01 B6]
    sendTrkMessage(0x18, CB(handleTrkContinue),
        formatInt(m_session.pid) + formatInt(m_session.tid));
    //sendTrkMessage(0x18, CB(handleTrkContinue),
    //    formatInt(m_session.pid) + "ff ff ff ff");
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 0E 00]
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
    sendTrkMessage(0x1B, CB(handleTrkSetBreakpoint),
        "82 "
        + QByteArray(bp.mode == ArmMode ? "00 " : "01 ")
        + formatInt(m_session.codeseg + bp.offset)
        + "00 00 00 01 00 00 00 00 " + formatInt(m_session.pid)
        + "FF FF FF FF");
    //m_session.toekn

    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 09 00 00 00 00 0A]
}

void Adapter::handleTrkSetBreakpoint(const TrkResult &result)
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
    sendTrkMessage(0x1C, CB(handleTrkClearBreakpoint),
        //formatInt(m_session.codeseg + bp.offset));
        "00 " + formatShort(bp.number)
            + formatInt(m_session.codeseg + bp.offset));

    //---IDE------------------------------------------------------
    //  Command: 0x1C Clear Break
    // [1C 25 00 00 00 0A 78 6A 43 40]
}

void Adapter::handleTrkClearBreakpoint(const TrkResult &result)
{
    Q_UNUSED(result);
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 09 00 00 00 00 0A]
    logMessage("CLEAR BREAKPOINT ");
}

void Adapter::handleTrkContinue(const TrkResult &result)
{
    logMessage("   HANDLE CONTINUE: " + stringFromArray(result.data));
    //if (result.result.token)
        //logMessage("   ERROR: " + byte(result.result.token)
    //    sendTrkMessage(0x18, CB(handleTrkContinue),
    //        formatInt(m_session.pid) + formatInt(m_session.tid));
    //}
}

void Adapter::handleTrkDisconnect(const TrkResult &result)
{
    logMessage("   HANDLE DISCONNECT: " + stringFromArray(result.data));
     //if (result.result.token)
        //logMessage("   ERROR: " + byte(result.result.token)
    //    sendTrkMessage(0x18, CB(handleTrkContinue),
    //        formatInt(m_session.pid) + formatInt(m_session.tid));
    //}
}

void Adapter::handleTrkDeleteProcess(const TrkResult &result)
{
    logMessage("   HANDLE DELETE PROCESS: " + stringFromArray(result.data));
    //if (result.result.token)
        //logMessage("   ERROR: " + byte(result.token)
    //    sendTrkMessage(0x18, CB(handleTrkContinue),
    //        formatInt(m_session.pid) + formatInt(m_session.tid));
    //}
}

void Adapter::handleTrkStep(const TrkResult &result)
{
    logMessage("   HANDLE STEP: " + stringFromArray(result.data));
}

void Adapter::handleTrkStop(const TrkResult &result)
{
    logMessage("   HANDLE STOP: " + stringFromArray(result.data));
}


void Adapter::handleTrkReadInfo(const TrkResult &result)
{
    logMessage("   HANDLE READ INFO: " + stringFromArray(result.data));
}

void Adapter::handleTrkWaitForFinished(const TrkResult &result)
{
    logMessage("   FINISHED: " + stringFromArray(result.data));
    //qApp->exit(1);
}

void Adapter::handleTrkSupportMask(const TrkResult &result)
{
    const char *data = result.data.data();
    QByteArray str;
    for (int i = 0; i < 32; ++i) {
        //str.append("  [" + formatByte(data[i]) + "]: ");
        for (int j = 0; j < 8; ++j)
        if (data[i] & (1 << j))
            str.append(formatByte(i * 8 + j));
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
    sendTrkMessage(0x41, CB(handleTrkDeleteProcess),
        "00 00 " + formatInt(m_session.pid));
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
    sendTrkMessage(0x02, CB(handleTrkDisconnect));
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    // Error: 0x00
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
    adapter.startServer();

    return app.exec();
}

#include "adapter.moc"
