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

#include <QtCore/QDebug>

#define logMessage(s)  do { qDebug() << "TRKCLIENT: " << s; } while (0)

namespace trk {

QString TrkResult::toString() const
{
    QString res = stringFromByte(code) + "[" + stringFromByte(token);
    res.chop(1);
    return res + "] " + stringFromArray(data);
}

QByteArray frameMessage(byte command, byte token, const QByteArray &data)
{
    byte s = command + token;
    for (int i = 0; i != data.size(); ++i)
        s += data.at(i);
    byte checksum = 255 - (s & 0xff);
    //int x = s + ~s;
    //logMessage("check: " << s << checksum << x;

    QByteArray response;
    response.append(char(command));
    response.append(char(token));
    response.append(data);
    response.append(char(checksum));

    QByteArray encodedData = encode7d(response);
    ushort encodedSize = encodedData.size() + 2; // 2 x 0x7e

    QByteArray ba;
    ba.append(char(0x01));
    ba.append(char(0x90));
    ba.append(char(encodedSize / 256));
    ba.append(char(encodedSize % 256));
    ba.append(char(0x7e));
    ba.append(encodedData);
    ba.append(char(0x7e));

    return ba;
}

TrkResult extractResult(QByteArray *buffer)
{
    TrkResult result;
    if (buffer->at(0) != 0x01 || byte(buffer->at(1)) != 0x90) {
        logMessage("*** ERROR READBUFFER INVALID (2): "
            << stringFromArray(*buffer)
            << int(buffer->at(0))
            << int(buffer->at(1))
            << buffer->size());
        return result;
    }
    ushort len = extractShort(buffer->data() + 2);

    //logMessage("   READ BUF: " << stringFromArray(*buffer));
    if (buffer->size() < len + 4) {
        logMessage("*** INCOMPLETE RESPONSE: "
            << stringFromArray(*buffer));
        return result;
    }

    if (byte(buffer->at(4)) != 0x7e) {
        logMessage("** ERROR READBUFFER BEGIN FRAME MARKER INVALID: "
            << stringFromArray(*buffer) << len);
        return result;
    }

    if (byte(buffer->at(4 + len - 1)) != 0x7e) {
        logMessage("** ERROR READBUFFER END FRAME MARKER INVALID: "
            << stringFromArray(*buffer) << len);
        return result;
    }

    // FIXME: what happens if the length contains 0xfe?
    // Assume for now that it passes unencoded!
    QByteArray data = decode7d(buffer->mid(5, len - 2));
    *buffer = buffer->mid(4 + len);

    byte sum = 0;
    for (int i = 0; i < data.size(); ++i) // 3 = 2 * 0xfe + sum
        sum += byte(data.at(i));
    if (sum != 0xff)
        logMessage("*** CHECKSUM ERROR: " << byte(sum));

    result.code = data.at(0);
    result.token = data.at(1);
    result.data = data.mid(2);
    //logMessage("   REST BUF: " << stringFromArray(*buffer));
    //logMessage("   CURR DATA: " << stringFromArray(data));
    //QByteArray prefix = "READ BUF:                                       ";
    //logMessage((prefix + "HEADER: " + stringFromArray(header).toLatin1()).data());
    return result;
}

ushort extractShort(const char *data)
{
    return data[0] * 256 + data[1];
}

uint extractInt(const char *data)
{
    uint res = data[0];
    res *= 256; res += data[1];
    res *= 256; res += data[2];
    res *= 256; res += data[3];
    return res;
}

QString quoteUnprintableLatin1(const QByteArray &ba)
{
    QString res;
    char buf[10];
    for (int i = 0, n = ba.size(); i != n; ++i) {
        const byte c = ba.at(i);
        if (isprint(c)) {
            res += c;
        } else {
            qsnprintf(buf, sizeof(buf) - 1, "\\%x", int(c));
            res += buf;
        }
    }
    return res;
}

QByteArray decode7d(const QByteArray &ba)
{
    QByteArray res;
    res.reserve(ba.size() + 2);
    for (int i = 0; i < ba.size(); ++i) {
        byte c = byte(ba.at(i));
        if (c == 0x7d) {
            ++i;
            c = 0x20 ^ byte(ba.at(i));
        }
        res.append(c);
    }
    //if (res != ba)
    //    logMessage("DECODED: " << stringFromArray(ba)
    //        << " -> " << stringFromArray(res));
    return res;
}

QByteArray encode7d(const QByteArray &ba)
{
    QByteArray res;
    res.reserve(ba.size() + 2);
    for (int i = 0; i < ba.size(); ++i) {
        byte c = byte(ba.at(i));
        if (c == 0x7e || c == 0x7d) {
            res.append(0x7d);
            res.append(0x20 ^ c);
        } else {
            res.append(c);
        }
    }
    //if (res != ba)
    //    logMessage("ENCODED: " << stringFromArray(ba)
    //        << " -> " << stringFromArray(res));
    return res;
}

#define CB(s) &TrkClient::s

// FIXME: Use the QByteArray based version below?
QString stringFromByte(byte c)
{
    return QString("%1 ").arg(c, 2, 16, QChar('0'));
}

QString stringFromArray(const QByteArray &ba)
{
    QString str;
    QString ascii;
    for (int i = 0; i < ba.size(); ++i) {
        //if (i == 5 || i == ba.size() - 2)
        //    str += "  ";
        int c = byte(ba.at(i));
        str += QString("%1 ").arg(c, 2, 16, QChar('0'));
        if (i >= 8 && i < ba.size() - 2)
            ascii += QChar(c).isPrint() ? QChar(c) : QChar('.');
    }
    return str + "  " + ascii;
}


QByteArray formatByte(byte b)
{
    char buf[30];
    qsnprintf(buf, sizeof(buf) - 1, "%x ", b);
    return buf;
}

QByteArray formatShort(ushort s)
{
    char buf[30];
    qsnprintf(buf, sizeof(buf) - 1, "%x %x ", s / 256, s % 256);
    return buf;
}

QByteArray formatInt(uint i)
{
    char buf[30];
    int b3 = i % 256; i -= b3; i /= 256;
    int b2 = i % 256; i -= b2; i /= 256;
    int b1 = i % 256; i -= b1; i /= 256;
    int b0 = i % 256; i -= b0; i /= 256;
    qsnprintf(buf, sizeof(buf) - 1, "%x %x %x %x ", b0, b1, b2, b3);
    return buf;
}

QByteArray formatString(const QByteArray &str)
{
    QByteArray ba = formatShort(str.size());
    foreach (byte b, str)
        ba.append(formatByte(b));
    return ba;
}

QByteArray errorMessage(byte code)
{
    switch (code) {
        case 0x00: return "No error";
        case 0x01: return "Generic error in CWDS message";
        case 0x02: return "Unexpected packet size in send msg";
        case 0x03: return "Internal error occurred in CWDS";
        case 0x04: return "Escape followed by frame flag";
        case 0x05: return "Bad FCS in packet";
        case 0x06: return "Packet too long";
        case 0x07: return "Sequence ID not expected (gap in sequence)";

        case 0x10: return "Command not supported";
        case 0x11: return "Command param out of range";
        case 0x12: return "An option was not supported";
        case 0x13: return "Read/write to invalid memory";
        case 0x14: return "Read/write invalid registers";
        case 0x15: return "Exception occurred in CWDS";
        case 0x16: return "Targeted system or thread is running";
        case 0x17: return "Breakpoint resources (HW or SW) exhausted";
        case 0x18: return "Requested breakpoint conflicts with existing one";

        case 0x20: return "General OS-related error";
        case 0x21: return "Request specified invalid process";
        case 0x22: return "Request specified invalid thread";
    }
    return "Unknown error";
}

TrkClient::TrkClient()
{
#if USE_NATIVE
    m_hdevice = NULL;
#else
    m_device = 0;
#endif
    m_writeToken = 0;
    m_readToken = 0;
    m_writeBusy = false;
    //m_breakpoints.append(Breakpoint(0x0370));
    //m_breakpoints.append(Breakpoint(0x0340));
    //m_breakpoints.append(Breakpoint(0x0040)); // E32Main
    startTimer(100);
}

TrkClient::~TrkClient()
{
#if USE_NATIVE
    CloseHandle(m_hdevice);
#else
    delete m_device;
#endif
}

bool TrkClient::openPort(const QString &port)
{
    // QFile does not work with "COM3", so work around
    /*
    FILE *f = fopen("COM3", "r+");
    if (!f) {
        logMessage("Could not open file ");
        return;
    }
    m_device = new QFile;
    if (!m_device->open(f, QIODevice::ReadWrite))
    */

#if 0
    m_device = new Win_QextSerialPort(port);
    m_device->setBaudRate(BAUD115200);
    m_device->setDataBits(DATA_8);
    m_device->setParity(PAR_NONE);
    //m_device->setStopBits(STO);
    m_device->setFlowControl(FLOW_OFF);
    m_device->setTimeout(0, 500);

    if (!m_device->open(QIODevice::ReadWrite)) {
        QByteArray ba = m_device->errorString().toLatin1();
        logMessage("Could not open device " << ba);
        return;
    }
#else
    m_device = new QLocalSocket(this);
    m_device->connectToServer(port);
    return m_device->waitForConnected();
#endif
}

void TrkClient::timerEvent(QTimerEvent *)
{
    //qDebug(".");
    tryWrite();
    tryRead();
}

unsigned char TrkClient::nextWriteToken()
{
    ++m_writeToken;
    if (m_writeToken == 0)
        ++m_writeToken;
    return m_writeToken;
}

void TrkClient::sendMessage(byte command,
    CallBack callBack, const QByteArray &lit)
{
    Message msg;
    msg.command = command;
    msg.token = nextWriteToken();
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
    queueMessage(msg);
}

void TrkClient::sendInitialPing()
{
    Message msg;
    msg.command = 0x00; // Ping
    msg.token = 0; // reset sequence count
    queueMessage(msg);
}

void TrkClient::waitForFinished()
{
    Message msg;
    // initiate one last roundtrip to ensure all is flushed
    msg.command = 0x00; // Ping
    msg.token = nextWriteToken();
    msg.callBack = CB(handleWaitForFinished);
    queueMessage(msg);
}

void TrkClient::sendAck(byte token)
{
    logMessage(QString("SENDING ACKNOWLEDGEMENT FOR TOKEN ").arg(int(token)));
    Message msg;
    msg.command = 0x80;
    msg.token = token;
    msg.data.append('\0');
    // The acknowledgement must not be queued!
    //queueMessage(msg);
    doWrite(msg);
    // 01 90 00 07 7e 80 01 00 7d 5e 7e
}

void TrkClient::queueMessage(const Message &msg)
{
    m_writeQueue.append(msg);
}

void TrkClient::tryWrite()
{
    if (m_writeBusy)
        return;
    if (m_writeQueue.isEmpty())
        return;

    doWrite(m_writeQueue.dequeue());
}

void TrkClient::doWrite(const Message &msg)
{
    QByteArray ba = frameMessage(msg.command, msg.token, msg.data);

    m_written.insert(msg.token, msg);
    m_writeBusy = true;

#if USE_NATIVE

    DWORD charsWritten;

    if (!WriteFile( m_hdevice,
                    ba.data(),
                    ba.size(),
                    &charsWritten,
                    NULL)){
        logMessage("WRITE ERROR: ");
    }


    logMessage("WRITE: " << qPrintable(stringFromArray(ba)));
    FlushFileBuffers(m_hdevice);

#else

    logMessage("WRITE: " << qPrintable(stringFromArray(ba)));
    if (!m_device->write(ba))
        logMessage("WRITE ERROR: " << m_device->errorString());
    m_device->flush();

#endif

}

void TrkClient::tryRead()
{
    //logMessage("TRY READ: " << m_device->bytesAvailable()
    //        << stringFromArray(m_readQueue);

#if USE_NATIVE

    const int BUFFERSIZE = 1024;
    char buffer[BUFFERSIZE];
    DWORD charsRead;

    while (ReadFile(m_hdevice, buffer, BUFFERSIZE, &charsRead, NULL)
            && BUFFERSIZE == charsRead) {
        m_readQueue.append(buffer, charsRead);
    }
    m_readQueue.append(buffer, charsRead);

#else // USE_NATIVE

    if (m_device->bytesAvailable() == 0 && m_readQueue.isEmpty())
        return;

    QByteArray res = m_device->readAll();
    m_readQueue.append(res);


#endif // USE_NATIVE

    if (m_readQueue.size() < 9) {
        logMessage("ERROR READBUFFER INVALID (1): "
            << stringFromArray(m_readQueue));
        m_readQueue.clear();
        return;
    }

    while (!m_readQueue.isEmpty())
        handleResult(extractResult(&m_readQueue));

    m_writeBusy = false;
}


void TrkClient::handleResult(const TrkResult &result)
{
    const char *prefix = "READ BUF:                                       ";
    QByteArray str = result.toString().toUtf8();
    switch (result.code) {
        case 0x80: { // ACK
            logMessage(prefix << "ACK: " << str.data());
            if (!result.data.isEmpty() && result.data.at(0))
                logMessage(prefix << "ERR: " << QByteArray::number(result.data.at(0)));
            //logMessage("READ RESULT FOR TOKEN: " << token);
            if (!m_written.contains(result.token)) {
                logMessage("NO ENTRY FOUND!");
            }
            Message msg = m_written.take(result.token);
            CallBack cb = msg.callBack;
            if (cb) {
                //logMessage("HANDLE: " << stringFromArray(result.data));
                (this->*cb)(result);
            }
            break;
        }
        case 0xff: { // NAK
            logMessage(prefix << "NAK: " << str.data());
            //logMessage(prefix << "TOKEN: " << result.token);
            logMessage(prefix << "ERROR: " << errorMessage(result.data.at(0)));
            break;
        }
        case 0x90: { // Notified Stopped
            logMessage(prefix << "NOTE: STOPPED" << str.data());
            // 90 01   78 6a 40 40   00 00 07 23   00 00 07 24  00 00
            const char *data = result.data.data();
            uint addr = extractInt(data); //code address: 4 bytes; code base address for the library
            uint pid = extractInt(data + 4); // ProcessID: 4 bytes;
            uint tid = extractInt(data + 8); // ThreadID: 4 bytes
            logMessage(prefix << "      ADDR: " << addr << " PID: " << pid << " TID: " << tid);
            sendAck(result.token);
            //Sleep(10000);
            //cleanUp();
            break;
        }
        case 0x91: { // Notify Exception (obsolete)
            logMessage(prefix << "NOTE: EXCEPTION" << str.data());
            sendAck(result.token);
            break;
        }
        case 0x92: { //
            logMessage(prefix << "NOTE: INTERNAL ERROR: " << str.data());
            sendAck(result.token);
            break;
        }

        // target->host OS notification
        case 0xa0: { // Notify Created
            const char *data = result.data.data();
            byte error = result.data.at(0);
            byte type = result.data.at(1); // type: 1 byte; for dll item, this value is 2.
            uint pid = extractInt(data + 2); //  ProcessID: 4 bytes;
            uint tid = extractInt(data + 6); //threadID: 4 bytes
            uint codeseg = extractInt(data + 10); //code address: 4 bytes; code base address for the library
            uint dataseg = extractInt(data + 14); //data address: 4 bytes; data base address for the library
            uint len = extractShort(data + 18); //length: 2 bytes; length of the library name string to follow
            QByteArray name = result.data.mid(20, len); // name: library name

            logMessage(prefix << "NOTE: LIBRARY LOAD: " << str.data());
            logMessage(prefix << "TOKEN: " << result.token);
            logMessage(prefix << "ERROR: " << int(error));
            logMessage(prefix << "TYPE:  " << int(type));
            logMessage(prefix << "PID:   " << pid);
            logMessage(prefix << "TID:   " << tid);
            logMessage(prefix << "CODE:  " << codeseg);
            logMessage(prefix << "DATA:  " << dataseg);
            logMessage(prefix << "LEN:   " << len);
            logMessage(prefix << "NAME:  " << name);

            sendMessage(0x18, CB(handleContinue),
                formatInt(m_session.pid) + formatInt(m_session.tid));

            //sendAck(result.token)
            break;
        }
        case 0xa1: { // NotifyDeleted
            logMessage(prefix << "NOTE: LIBRARY UNLOAD: " << str.data());
            sendAck(result.token);
            break;
        }
        case 0xa2: { // NotifyProcessorStarted
            logMessage(prefix << "NOTE: PROCESSOR STARTED: " << str.data());
            sendAck(result.token);
            break;
        }
        case 0xa6: { // NotifyProcessorStandby
            logMessage(prefix << "NOTE: PROCESSOR STANDBY: " << str.data());
            sendAck(result.token);
            break;
        }
        case 0xa7: { // NotifyProcessorReset
            logMessage(prefix << "NOTE: PROCESSOR RESET: " << str.data());
            sendAck(result.token);
            break;
        }
        default: {
            logMessage(prefix << "INVALID: " << str << result.data.size());
            break;
        }
    }
}

void TrkClient::handleCpuType(const TrkResult &result)
{
    logMessage("HANDLE CPU TYPE: " << result.toString());
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

void TrkClient::handleCreateProcess(const TrkResult &result)
{
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 08 00  00 00 01 B5  00 00 01 B6  78 67 40 00  00
    //  40 00 00]

    logMessage("       RESULT: " << qPrintable(result.toString()));
    const char *data = result.data.data();
    m_session.pid = extractInt(data);
    m_session.tid = extractInt(data + 4);
    m_session.codeseg = extractInt(data + 8);
    m_session.dataseg = extractInt(data + 12);

    logMessage("PID: " << formatInt(m_session.pid) << m_session.pid);
    logMessage("TID: " << formatInt(m_session.tid) << m_session.tid);
    logMessage("COD: " << formatInt(m_session.codeseg) << m_session.codeseg);
    logMessage("DAT: " << formatInt(m_session.dataseg) << m_session.dataseg);


    //setBreakpoint(0x0000, ArmMode);
    //clearBreakpoint(0x0000);

#if 1
    foreach (const Breakpoint &bp, m_breakpoints)
        setBreakpoint(bp);
#endif

#if 1
    //---IDE------------------------------------------------------
    //  Command: 0x42 Read Info
    //          [42 0C 00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F
    //  72 70 68 69 63 44 4C 4C 32 2E 64 6C 6C 00]
    sendMessage(0x42, CB(handleReadInfo),
        "00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F "
        "72 70 68 69 63 44 4C 4C 32 2E 64 6C 6C 00");
    //sendMessage(0x42, CB(handleReadInfo),
    //        "00 01 00 00 00 00");
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x20 Unspecified general OS-related error
    // [80 0C 20]


    //---IDE------------------------------------------------------
    //  Command: 0x42 Read Info
    // [42 0D 00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F
    //  72 70 68 69 63 44 4C 4C 31 2E 64 6C 6C 00]
    sendMessage(0x42, CB(handleReadInfo),
        "00 06 00 00 00 00 00 14 50 6F 6C 79 6D 6F "
        "72 70 68 69 63 44 4C 4C 31 2E 64 6C 6C 00");
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x20 Unspecified general OS-related error
    // [80 0D 20]
#endif

    //sendMessage(0x18, CB(handleStop),
    //    "01 " + formatInt(m_session.pid) + formatInt(m_session.tid));

    //---IDE------------------------------------------------------
    //  Command: 0x18 Continue
    //ProcessID: 0x000001B5 (437)
    // ThreadID: 0x000001B6 (438)
    // [18 0E 00 00 01 B5 00 00 01 B6]
    sendMessage(0x18, CB(handleContinue),
        formatInt(m_session.pid) + formatInt(m_session.tid));
    //sendMessage(0x18, CB(handleContinue),
    //    formatInt(m_session.pid) + "ff ff ff ff");
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 0E 00]
}

void TrkClient::setBreakpoint(const Breakpoint &bp)
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
    sendMessage(0x1B, CB(handleSetBreakpoint),
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

void TrkClient::handleSetBreakpoint(const TrkResult &result)
{
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 09 00 00 00 00 0A]
    uint bpnr = extractInt(result.data.data());
    logMessage("SET BREAKPOINT " << bpnr
        << stringFromArray(result.data.data()));
}

void TrkClient::clearBreakpoint(const Breakpoint &bp)
{
    sendMessage(0x1C, CB(handleClearBreakpoint),
        //formatInt(m_session.codeseg + bp.offset));
        "00 " + formatShort(bp.number)
            + formatInt(m_session.codeseg + bp.offset));

    //---IDE------------------------------------------------------
    //  Command: 0x1C Clear Break
    // [1C 25 00 00 00 0A 78 6A 43 40]
}

void TrkClient::handleClearBreakpoint(const TrkResult &result)
{
    Q_UNUSED(result);
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 09 00 00 00 00 0A]
    logMessage("CLEAR BREAKPOINT ");
}

void TrkClient::handleContinue(const TrkResult &result)
{
    logMessage("   HANDLE CONTINUE: " << qPrintable(stringFromArray(result.data)));
    //if (result.result.token)
        //logMessage("   ERROR: " << byte(result.result.token)
    //    sendMessage(0x18, CB(handleContinue),
    //        formatInt(m_session.pid) + formatInt(m_session.tid));
    //}
}

void TrkClient::handleDisconnect(const TrkResult &result)
{
    logMessage("   HANDLE DISCONNECT: "
        << qPrintable(stringFromArray(result.data)));
     //if (result.result.token)
        //logMessage("   ERROR: " << byte(result.result.token)
    //    sendMessage(0x18, CB(handleContinue),
    //        formatInt(m_session.pid) + formatInt(m_session.tid));
    //}
}

void TrkClient::handleDeleteProcess(const TrkResult &result)
{
    logMessage("   HANDLE DELETE PROCESS: " <<
qPrintable(stringFromArray(result.data)));
    //if (result.result.token)
        //logMessage("   ERROR: " << byte(result.token)
    //    sendMessage(0x18, CB(handleContinue),
    //        formatInt(m_session.pid) + formatInt(m_session.tid));
    //}
}

void TrkClient::handleStep(const TrkResult &result)
{
    logMessage("   HANDLE STEP: " <<
qPrintable(stringFromArray(result.data)));
}

void TrkClient::handleStop(const TrkResult &result)
{
    logMessage("   HANDLE STOP: " <<
qPrintable(stringFromArray(result.data)));
}


void TrkClient::handleReadInfo(const TrkResult &result)
{
    logMessage("   HANDLE READ INFO: " <<
qPrintable(stringFromArray(result.data)));
}

void TrkClient::handleWaitForFinished(const TrkResult &result)
{
    logMessage("   FINISHED: " << qPrintable(stringFromArray(result.data)));
    //qApp->exit(1);
}

void TrkClient::handleSupportMask(const TrkResult &result)
{
    const char *data = result.data.data();
    QByteArray str;
    for (int i = 0; i < 32; ++i) {
        //str.append("  [" + formatByte(data[i]) + "]: ");
        for (int j = 0; j < 8; ++j)
        if (data[i] & (1 << j))
            str.append(formatByte(i * 8 + j));
    }
    logMessage("SUPPORTED: " << str);
}


void TrkClient::cleanUp()
{
    //
    //---IDE------------------------------------------------------
    //  Command: 0x41 Delete Item
    //  Sub Cmd: Delete Process
    //ProcessID: 0x0000071F (1823)
    // [41 24 00 00 00 00 07 1F]
    sendMessage(0x41, CB(handleDeleteProcess),
        "00 00 " + formatInt(m_session.pid));
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 24 00]

    foreach (const Breakpoint &bp, m_breakpoints)
        clearBreakpoint(bp);

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
    sendMessage(0x02, CB(handleDisconnect));
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    // Error: 0x00
}

void TrkClient::onStopped(const TrkResult &result)
{
    Q_UNUSED(result);
}

} // namespace trk

