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

#include "launcher.h"
#include "trkutils.h"

#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtCore/QVariant>
#include <QtCore/QDebug>
#include <QtCore/QQueue>
#include <QtCore/QFile>

#ifdef Q_OS_WIN
#include <windows.h>

// Format windows error from GetLastError() value: TODO: Use the one provided by the utisl lib.
QString winErrorMessage(unsigned long error)
{
    QString rc = QString::fromLatin1("#%1: ").arg(error);
    ushort *lpMsgBuf;

    const int len = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, 0, (LPTSTR)&lpMsgBuf, 0, NULL);
    if (len) {
        rc = QString::fromUtf16(lpMsgBuf, len);
        LocalFree(lpMsgBuf);
    } else {
        rc += QString::fromLatin1("<unknown error>");
    }
    return rc;
}

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

namespace trk {

struct TrkMessage {
    TrkMessage() { code = token = 0; callBack = 0; }
    byte code;
    byte token;
    QByteArray data;
    QVariant cookie;
    Launcher::TrkCallBack callBack;
};

struct LauncherPrivate {
    LauncherPrivate();
#ifdef Q_OS_WIN
    HANDLE m_hdevice;
#endif

    QString m_trkServerName;
    QByteArray m_trkReadBuffer;

    unsigned char m_trkWriteToken;
    QQueue<TrkMessage> m_trkWriteQueue;
    QHash<byte, TrkMessage> m_writtenTrkMessages;
    QByteArray m_trkReadQueue;
    bool m_trkWriteBusy;

    void logMessage(const QString &msg);
    // Debuggee state
    Session m_session; // global-ish data (process id, target information)

    int m_timerId;
    QString m_fileName;
    QString m_copySrcFileName;
    QString m_copyDstFileName;
    QString m_installFileName;
    int m_verbose;
};

LauncherPrivate::LauncherPrivate() :
#ifdef Q_OS_WIN
    m_hdevice(0),
#endif
    m_trkWriteToken(0),
    m_trkWriteBusy(false),
    m_timerId(-1),
    m_verbose(0)
{
}

#define CB(s) &Launcher::s

Launcher::Launcher() :
    d(new LauncherPrivate)
{
}

Launcher::~Launcher()
{
    logMessage("Shutting down.\n");
    delete d;
}

void Launcher::setTrkServerName(const QString &name)
{
    d->m_trkServerName = name;
}

void Launcher::setFileName(const QString &name)
{
    d->m_fileName = name;
}

void Launcher::setCopyFileName(const QString &srcName, const QString &dstName)
{
    d->m_copySrcFileName = srcName;
    d->m_copyDstFileName = dstName;
}

void Launcher::setInstallFileName(const QString &name)
{
    d->m_installFileName = name;
}

bool Launcher::startServer(QString *errorMessage)
{
    if (d->m_verbose) {
        const QString msg = QString::fromLatin1("Port=%1 Executable=%2 Package=%3 Remote Package=%4 Install file=%5")
                            .arg(d->m_trkServerName, d->m_fileName, d->m_copySrcFileName, d->m_copyDstFileName, d->m_installFileName);
        logMessage(msg);
    }
    if (!openTrkPort(d->m_trkServerName, errorMessage))
        return false;
    d->m_timerId = startTimer(100);
    sendTrkInitialPing();
    sendTrkMessage(TrkConnect); // Connect
    sendTrkMessage(TrkSupported, CB(handleSupportMask));
    sendTrkMessage(TrkCpuType, CB(handleCpuType));
    sendTrkMessage(TrkVersions, CB(handleTrkVersion));
    if (d->m_fileName.isEmpty())
        return true;
    if (!d->m_copySrcFileName.isEmpty() && !d->m_copyDstFileName.isEmpty())
        copyFileToRemote();
    else
        installAndRun();
    return true;
}

void Launcher::setVerbose(int v)
{
    d->m_verbose = v;
}

void Launcher::installAndRun()
{
    if (!d->m_installFileName.isEmpty()) {
        installRemotePackageSilently(d->m_installFileName);
    } else {
        startInferiorIfNeeded();
    }
}
void Launcher::logMessage(const QString &msg)
{
    if (d->m_verbose)
        qDebug() << "ADAPTER: " << qPrintable(msg);
}

bool Launcher::openTrkPort(const QString &port, QString *errorMessage)
{
#ifdef Q_OS_WIN
    d->m_hdevice = CreateFile(port.toStdWString().c_str(),
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

    if (INVALID_HANDLE_VALUE == d->m_hdevice){
        *errorMessage = QString::fromLatin1("Could not open device '%1': %2").arg(port, winErrorMessage(GetLastError()));
        logMessage(*errorMessage);
        return false;
    }
    return true;
#else
    Q_UNUSED(port)
    *errorMessage = QString::fromLatin1("Not implemented");
    logMessage(*errorMessage);
    return false;
#endif
}

void Launcher::timerEvent(QTimerEvent *)
{
    //qDebug(".");
    tryTrkWrite();
    tryTrkRead();
}

byte Launcher::nextTrkWriteToken()
{
    ++d->m_trkWriteToken;
    if (d->m_trkWriteToken == 0)
        ++d->m_trkWriteToken;
    return d->m_trkWriteToken;
}

void Launcher::sendTrkMessage(byte code, TrkCallBack callBack,
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

void Launcher::sendTrkInitialPing()
{
    TrkMessage msg;
    msg.code = 0x00; // Ping
    msg.token = 0; // reset sequence count
    queueTrkMessage(msg);
}

void Launcher::waitForTrkFinished(const TrkResult &result)
{
    Q_UNUSED(result)
    sendTrkMessage(TrkPing, CB(handleWaitForFinished));
}

void Launcher::terminate()
{
    QByteArray ba;
    appendShort(&ba, 0x0000, TargetByteOrder);
    appendInt(&ba, d->m_session.pid, TargetByteOrder);
    sendTrkMessage(TrkDeleteItem, CB(waitForTrkFinished), ba);
}

void Launcher::sendTrkAck(byte token)
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

void Launcher::queueTrkMessage(const TrkMessage &msg)
{
    d->m_trkWriteQueue.append(msg);
}

void Launcher::tryTrkWrite()
{
    if (d->m_trkWriteBusy)
        return;
    if (d->m_trkWriteQueue.isEmpty())
        return;

    TrkMessage msg = d->m_trkWriteQueue.dequeue();
    trkWrite(msg);
}

void Launcher::trkWrite(const TrkMessage &msg)
{
    QByteArray ba = frameMessage(msg.code, msg.token, msg.data);

    d->m_writtenTrkMessages.insert(msg.token, msg);
    d->m_trkWriteBusy = true;

    logMessage("WRITE: " + stringFromArray(ba));

#ifdef Q_OS_WIN
    DWORD charsWritten;
    if (!WriteFile(d->m_hdevice, ba.data(), ba.size(), &charsWritten, NULL))
        logMessage("WRITE ERROR: ");

    //logMessage("WRITE: " + stringFromArray(ba));
    FlushFileBuffers(d->m_hdevice);
#endif
}

void Launcher::tryTrkRead()
{
#ifdef Q_OS_WIN
    const DWORD BUFFERSIZE = 1024;
    char buffer[BUFFERSIZE];
    DWORD charsRead;

    while (TryReadFile(d->m_hdevice, buffer, BUFFERSIZE, &charsRead, NULL)) {
        d->m_trkReadQueue.append(buffer, charsRead);
        if (isValidTrkResult(d->m_trkReadQueue))
            break;
    }
    if (!isValidTrkResult(d->m_trkReadQueue)) {
        logMessage("Partial message: " + stringFromArray(d->m_trkReadQueue));
        return;
    }
#endif // Q_OS_WIN
    logMessage("READ:  " + stringFromArray(d->m_trkReadQueue));
    handleResult(extractResult(&d->m_trkReadQueue));

    d->m_trkWriteBusy = false;
}


void Launcher::handleResult(const TrkResult &result)
{
    QByteArray prefix = "READ BUF:                                       ";
    QByteArray str = result.toString().toUtf8();
    if (result.isDebugOutput) { // handle application output
        logMessage("APPLICATION OUTPUT: " + result.data);
        emit applicationOutputReceived(result.data);
        return;
    }
    switch (result.code) {
        case TrkNotifyAck: { // ACK
            //logMessage(prefix + "ACK: " + str);
            if (!result.data.isEmpty() && result.data.at(0))
                logMessage(prefix + "ERR: " +QByteArray::number(result.data.at(0)));
            //logMessage("READ RESULT FOR TOKEN: " << token);
            if (!d->m_writtenTrkMessages.contains(result.token)) {
                logMessage("NO ENTRY FOUND!");
            }
            TrkMessage msg = d->m_writtenTrkMessages.take(result.token);
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
            appendInt(&ba, d->m_session.pid);
            appendInt(&ba, d->m_session.tid);
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

void Launcher::handleTrkVersion(const TrkResult &result)
{
    if (result.data.size() < 5)
        return;
    const int trkMajor = result.data.at(1);
    const int trkMinor = result.data.at(2);
    const int protocolMajor = result.data.at(3);
    const int protocolMinor = result.data.at(4);
    // Ping mode: Log & Terminate
    if (d->m_fileName.isEmpty()) {
        QString msg;
        QTextStream(&msg) << "CPU: " << d->m_session.cpuMajor << '.' << d->m_session.cpuMinor << ' '
                << (d->m_session.bigEndian ? "big endian" : "little endian")
                << " type size: " << d->m_session.defaultTypeSize
                << " float size: " << d->m_session.fpTypeSize
                << " Trk: v" << trkMajor << '.' << trkMinor << " Protocol: " << protocolMajor << '.' << protocolMinor;
        qWarning("%s", qPrintable(msg));
        sendTrkMessage(TrkPing, CB(waitForTrkFinished));
    }
}

void Launcher::handleFileCreation(const TrkResult &result)
{
    // we don't do any error handling yet, which is bad
    const char *data = result.data.data();
    uint copyFileHandle = extractInt(data + 2);
    QFile file(d->m_copySrcFileName);
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

void Launcher::handleFileCreated(const TrkResult &result)
{
    Q_UNUSED(result)
    installAndRun();
}

void Launcher::handleCpuType(const TrkResult &result)
{
    logMessage("HANDLE CPU TYPE: " + result.toString());
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 03 00  04 00 00 04 00 00 00]
    d->m_session.cpuMajor = result.data.at(1);
    d->m_session.cpuMinor = result.data.at(2);
    d->m_session.bigEndian = result.data.at(3);
    d->m_session.defaultTypeSize = result.data.at(4);
    d->m_session.fpTypeSize = result.data.at(5);
    d->m_session.extended1TypeSize = result.data.at(6);
    //d->m_session.extended2TypeSize = result.data[6];
}

void Launcher::handleCreateProcess(const TrkResult &result)
{
    //  40 00 00]
    //logMessage("       RESULT: " + result.toString());
    // [80 08 00   00 00 01 B5   00 00 01 B6   78 67 40 00   00 40 00 00]
    const char *data = result.data.data();
    d->m_session.pid = extractInt(data + 1);
    d->m_session.tid = extractInt(data + 5);
    d->m_session.codeseg = extractInt(data + 9);
    d->m_session.dataseg = extractInt(data + 13);
    if (d->m_verbose) {
        const QString msg = QString::fromLatin1("Process id: %1 Thread id: %2 code: 0x%3 data: 0x%4").
                            arg(d->m_session.pid).arg(d->m_session.tid).arg(d->m_session.codeseg, 0, 16).
                            arg(d->m_session.dataseg,  0 ,16);
        logMessage(msg);
    }
    emit applicationRunning(d->m_session.pid);
    QByteArray ba;
    appendInt(&ba, d->m_session.pid);
    appendInt(&ba, d->m_session.tid);
    sendTrkMessage(TrkContinue, 0, ba, "CONTINUE");
}

void Launcher::handleWaitForFinished(const TrkResult &result)
{
    logMessage("   FINISHED: " + stringFromArray(result.data));
    killTimer(d->m_timerId);
    emit finished();
}

void Launcher::handleSupportMask(const TrkResult &result)
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


void Launcher::cleanUp()
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
    appendInt(&ba, d->m_session.pid);
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

void Launcher::copyFileToRemote()
{
    emit copyingStarted();
    QByteArray ba;
    appendByte(&ba, 0x10);
    appendString(&ba, d->m_copyDstFileName.toLocal8Bit(), TargetByteOrder, false);
    sendTrkMessage(TrkOpenFile, CB(handleFileCreation), ba);
}

void Launcher::installRemotePackageSilently(const QString &fileName)
{
    emit installingStarted();
    QByteArray ba;
    appendByte(&ba, 'C');
    appendString(&ba, fileName.toLocal8Bit(), TargetByteOrder, false);
    sendTrkMessage(TrkInstallFile, CB(handleInstallPackageFinished), ba);
}

void Launcher::handleInstallPackageFinished(const TrkResult &)
{
    startInferiorIfNeeded();
}

void Launcher::startInferiorIfNeeded()
{
    emit startingApplication();
    if (d->m_session.pid != 0) {
        logMessage("Process already 'started'");
        return;
    }
    // It's not started yet
    QByteArray ba;
    appendByte(&ba, 0); // ?
    appendByte(&ba, 0); // ?
    appendByte(&ba, 0); // ?
    appendString(&ba, d->m_fileName.toLocal8Bit(), TargetByteOrder);
    sendTrkMessage(TrkCreateItem, CB(handleCreateProcess), ba); // Create Item
}

}
