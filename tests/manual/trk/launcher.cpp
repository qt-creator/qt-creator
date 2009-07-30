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

#include "launcher.h"

#if USE_NATIVE
#include <windows.h>

// Non-blocking replacement for win-api ReadFile function
BOOL WINAPI TryReadFile(HANDLE          hFile,
                        LPVOID          lpBuffer,
                        DWORD           nNumberOfBytesToRead,
                        LPDWORD         lpNumberOfBytesRead,
                        LPOVERLAPPED    lpOverlapped)
{
    COMSTAT comStat;
    if (!ClearCommError(hFile, NULL, &comStat)){
        qDebug() << "ClearCommError() failed";
        return FALSE;
    }
    if (comStat.cbInQue == 0) {
        *lpNumberOfBytesRead = 0;
        return FALSE;
    }
    return ReadFile(hFile,
                    lpBuffer,
                    qMin(comStat.cbInQue, nNumberOfBytesToRead),
                    lpNumberOfBytesRead,
                    lpOverlapped);
}
#endif

using namespace trk;

#define CB(s) &Adapter::s

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

    logMessage("Shutting down.\n");
}

bool Adapter::startServer()
{
    if (!openTrkPort(m_trkServerName)) {
        qDebug("Unable to connect to TRK server");
        return false;
    }
    m_timerId = startTimer(100);
    qDebug("Connecting");
    sendTrkInitialPing();
    sendTrkMessage(TrkConnect); // Connect
    sendTrkMessage(TrkSupported, CB(handleSupportMask));
    sendTrkMessage(TrkCpuType, CB(handleCpuType));
    sendTrkMessage(TrkVersions); // Versions
    if (!m_copySrcFileName.isEmpty() && !m_copyDstFileName.isEmpty())
        copyFileToRemote();
    else
        installAndRun();
    return true;
}

void Adapter::installAndRun()
{
    if (!m_installFileName.isEmpty()) {
        installRemotePackageSilently(m_installFileName);
        startInferiorIfNeeded();
    } else {
        startInferiorIfNeeded();
    }
}
void Adapter::logMessage(const QString &msg)
{
    if (DEBUG_TRK)
        qDebug() << "ADAPTER: " << qPrintable(msg);
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
    return true;
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

void Adapter::waitForTrkFinished(const TrkResult &result)
{
    Q_UNUSED(result)
    sendTrkMessage(TrkPing, CB(handleWaitForFinished));
}

void Adapter::terminate()
{
    QByteArray ba;
    appendShort(&ba, 0x0000, TargetByteOrder);
    appendInt(&ba, m_session.pid, TargetByteOrder);
    sendTrkMessage(TrkDeleteItem, CB(waitForTrkFinished), ba);
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
    trkWrite(msg);
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

    //logMessage("WRITE: " + stringFromArray(ba));
    FlushFileBuffers(m_hdevice);
#else
    if (!m_trkDevice->write(ba))
        logMessage("WRITE ERROR: " + m_trkDevice->errorString());
    m_trkDevice->flush();

#endif
}

void Adapter::tryTrkRead()
{
#if USE_NATIVE
    const DWORD BUFFERSIZE = 1024;
    char buffer[BUFFERSIZE];
    DWORD charsRead;

    while (TryReadFile(m_hdevice, buffer, BUFFERSIZE, &charsRead, NULL)) {
        m_trkReadQueue.append(buffer, charsRead);
        if (isValidTrkResult(m_trkReadQueue))
            break;
    }
    if (charsRead == 0 && m_trkReadQueue.isEmpty()) {
        return;
    }
#else // USE_NATIVE
    if (m_trkDevice->bytesAvailable() == 0 && m_trkReadQueue.isEmpty()) {
        return;
    }

    QByteArray res = m_trkDevice->readAll();
    m_trkReadQueue.append(res);
#endif // USE_NATIVE

    logMessage("READ:  " + stringFromArray(m_trkReadQueue));
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
        case TrkNotifyAck: { // ACK
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
        case TrkNotifyNak: { // NAK
            logMessage(prefix + "NAK: " + str);
            //logMessage(prefix << "TOKEN: " << result.token);
            logMessage(prefix + "ERROR: " + errorMessage(result.data.at(0)));
            break;
        }
        case TrkNotifyStopped: { // Notified Stopped
            logMessage(prefix + "NOTE: STOPPED  " + str);
            // 90 01   78 6a 40 40   00 00 07 23   00 00 07 24  00 00
            //const char *data = result.data.data();
//            uint addr = extractInt(data); //code address: 4 bytes; code base address for the library
//            uint pid = extractInt(data + 4); // ProcessID: 4 bytes;
//            uint tid = extractInt(data + 8); // ThreadID: 4 bytes
            //logMessage(prefix << "      ADDR: " << addr << " PID: " << pid << " TID: " << tid);
            sendTrkAck(result.token);
            break;
        }
        case TrkNotifyException: { // Notify Exception (obsolete)
            logMessage(prefix + "NOTE: EXCEPTION  " + str);
            sendTrkAck(result.token);
            break;
        }
        case TrkNotifyInternalError: { //
            logMessage(prefix + "NOTE: INTERNAL ERROR: " + str);
            sendTrkAck(result.token);
            break;
        }

        // target->host OS notification
        case TrkNotifyCreated: { // Notify Created
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
            sendTrkMessage(TrkContinue, 0, ba, "CONTINUE");
            //sendTrkAck(result.token)
            break;
        }
        case TrkNotifyDeleted: { // NotifyDeleted
            logMessage(prefix + "NOTE: LIBRARY UNLOAD: " + str);
            sendTrkAck(result.token);
            const char *data = result.data.data();
            ushort itemType = extractShort(data);
            if (itemType == 0) { // process
                sendTrkMessage(TrkDisconnect, CB(waitForTrkFinished));
            }
            break;
        }
        case TrkNotifyProcessorStarted: { // NotifyProcessorStarted
            logMessage(prefix + "NOTE: PROCESSOR STARTED: " + str);
            sendTrkAck(result.token);
            break;
        }
        case TrkNotifyProcessorStandBy: { // NotifyProcessorStandby
            logMessage(prefix + "NOTE: PROCESSOR STANDBY: " + str);
            sendTrkAck(result.token);
            break;
        }
        case TrkNotifyProcessorReset: { // NotifyProcessorReset
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

void Adapter::handleFileCreation(const TrkResult &result)
{
    // we don't do any error handling yet, which is bad
    const char *data = result.data.data();
    uint copyFileHandle = extractInt(data + 2);
    QFile file(m_copySrcFileName);
    file.open(QIODevice::ReadOnly);
    QByteArray src = file.readAll();
    file.close();
    const int BLOCKSIZE = 1024;
    int size = src.length();
    int pos = 0;
    while (pos < size) {
        QByteArray ba;
        appendInt(&ba, copyFileHandle, TargetByteOrder);
        appendString(&ba, src.mid(pos, BLOCKSIZE), TargetByteOrder, false);
        sendTrkMessage(TrkWriteFile, 0, ba);
        pos += BLOCKSIZE;
    }
    QByteArray ba;
    appendInt(&ba, copyFileHandle, TargetByteOrder);
    appendInt(&ba, QDateTime::currentDateTime().toTime_t(), TargetByteOrder);
    sendTrkMessage(TrkCloseFile, CB(handleFileCreated), ba);
}

void Adapter::handleFileCreated(const TrkResult &result)
{
    Q_UNUSED(result)
    installAndRun();
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
    QByteArray ba;
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    sendTrkMessage(TrkContinue, 0, ba, "CONTINUE");
}

void Adapter::handleWaitForFinished(const TrkResult &result)
{
    logMessage("   FINISHED: " + stringFromArray(result.data));
    killTimer(m_timerId);
    emit finished();
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
    sendTrkMessage(TrkDeleteItem, 0, ba, "Delete process");

    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 24 00]

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

void Adapter::copyFileToRemote()
{
    qDebug("Copying file");
    QByteArray ba;
    appendByte(&ba, 0x10);
    appendString(&ba, m_copyDstFileName.toLocal8Bit(), TargetByteOrder, false);
    sendTrkMessage(TrkOpenFile, CB(handleFileCreation), ba);
}

void Adapter::installRemotePackageSilently(const QString &fileName)
{
    qDebug("Installing file");
    QByteArray ba;
    appendByte(&ba, 'C');
    appendString(&ba, fileName.toLocal8Bit(), TargetByteOrder, false);
    sendTrkMessage(TrkInstallFile, 0, ba);
}

void Adapter::startInferiorIfNeeded()
{
    qDebug("Starting");
    if (m_session.pid != 0) {
        qDebug() << "Process already 'started'";
        return;
    }
    // It's not started yet
    QByteArray ba;
    appendByte(&ba, 0); // ?
    appendByte(&ba, 0); // ?
    appendByte(&ba, 0); // ?
    appendString(&ba, m_fileName.toLocal8Bit(), TargetByteOrder);
    sendTrkMessage(TrkCreateItem, CB(handleCreateProcess), ba); // Create Item
}
