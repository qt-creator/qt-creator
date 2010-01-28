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
#include "trkdevice.h"
#include "bluetoothlistener.h"

#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtCore/QVariant>
#include <QtCore/QDebug>
#include <QtCore/QQueue>
#include <QtCore/QFile>
#include <QtCore/QScopedPointer>

namespace trk {

struct LauncherPrivate {
    struct CopyState {
        QString sourceFileName;
        QString destinationFileName;
        uint copyFileHandle;
        QScopedPointer<QByteArray> data;
        int position;
    };

    explicit LauncherPrivate(const TrkDevicePtr &d);

    TrkDevicePtr m_device;
    QString m_trkServerName;
    QByteArray m_trkReadBuffer;
    Launcher::State m_state;

    void logMessage(const QString &msg);
    // Debuggee state
    Session m_session; // global-ish data (process id, target information)

    CopyState m_copyState;
    QString m_fileName;
    QStringList m_commandLineArgs;
    QString m_installFileName;
    int m_verbose;
    Launcher::Actions m_startupActions;
    bool m_closeDevice;
};

LauncherPrivate::LauncherPrivate(const TrkDevicePtr &d) :
    m_device(d),
    m_state(Launcher::Disconnected),
    m_verbose(0),
    m_closeDevice(true)
{
    if (m_device.isNull())
        m_device = TrkDevicePtr(new TrkDevice);
}

Launcher::Launcher(Actions startupActions,
                   const TrkDevicePtr &dev,
                   QObject *parent) :
    QObject(parent),
    d(new LauncherPrivate(dev))
{
    d->m_startupActions = startupActions;
    connect(d->m_device.data(), SIGNAL(messageReceived(trk::TrkResult)), this, SLOT(handleResult(trk::TrkResult)));    
    connect(this, SIGNAL(finished()), d->m_device.data(), SLOT(close()));
}

Launcher::~Launcher()
{
    logMessage("Shutting down.\n");
    delete d;
}

Launcher::State Launcher::state() const
{
    return d->m_state;
}

void Launcher::setState(State s)
{
    if (s != d->m_state) {
        d->m_state = s;
        emit stateChanged(s);
    }
}

void Launcher::addStartupActions(trk::Launcher::Actions startupActions)
{
    d->m_startupActions = Actions(d->m_startupActions | startupActions);
}

void Launcher::setTrkServerName(const QString &name)
{
    d->m_trkServerName = name;
}

QString Launcher::trkServerName() const
{
    return d->m_trkServerName;
}

TrkDevicePtr Launcher::trkDevice() const
{
    return d->m_device;
}

void Launcher::setFileName(const QString &name)
{
    d->m_fileName = name;
}

void Launcher::setCopyFileName(const QString &srcName, const QString &dstName)
{
    d->m_copyState.sourceFileName = srcName;
    d->m_copyState.destinationFileName = dstName;
}

void Launcher::setInstallFileName(const QString &name)
{
    d->m_installFileName = name;
}

void Launcher::setCommandLineArgs(const QStringList &args)
{
    d->m_commandLineArgs = args;
}

void Launcher::setSerialFrame(bool b)
{
    d->m_device->setSerialFrame(b);
}

bool Launcher::serialFrame() const
{
    return d->m_device->serialFrame();
}


bool Launcher::closeDevice() const
{
    return d->m_closeDevice;
}

void Launcher::setCloseDevice(bool c)
{
    d->m_closeDevice = c;
}

bool Launcher::startServer(QString *errorMessage)
{
    errorMessage->clear();
    if (d->m_verbose) {
        const QString msg = QString::fromLatin1("Port=%1 Executable=%2 Arguments=%3 Package=%4 Remote Package=%5 Install file=%6")
                            .arg(d->m_trkServerName, d->m_fileName,
                                 d->m_commandLineArgs.join(QString(QLatin1Char(' '))),
                                 d->m_copyState.sourceFileName, d->m_copyState.destinationFileName, d->m_installFileName);
        logMessage(msg);
    }
    if (d->m_startupActions & ActionCopy) {
        if (d->m_copyState.sourceFileName.isEmpty()) {
            qWarning("No local filename given for copying package.");
            return false;
        } else if (d->m_copyState.destinationFileName.isEmpty()) {
            qWarning("No remote filename given for copying package.");
            return false;
        }
    }
    if (d->m_startupActions & ActionInstall && d->m_installFileName.isEmpty()) {
        qWarning("No package name given for installing.");
        return false;
    }
    if (d->m_startupActions & ActionRun && d->m_fileName.isEmpty()) {
        qWarning("No remote executable given for running.");
        return false;
    }
    if (!d->m_device->isOpen() && !d->m_device->open(d->m_trkServerName, errorMessage))
        return false;
    if (d->m_closeDevice) {
        connect(this, SIGNAL(finished()), d->m_device.data(), SLOT(close()));
    } else {
        disconnect(this, SIGNAL(finished()), d->m_device.data(), 0);
    }
    setState(Connecting);
    // Set up the temporary 'waiting' state if we do not get immediate connection
    QTimer::singleShot(1000, this, SLOT(slotWaitingForTrk()));
    d->m_device->sendTrkInitialPing();
    d->m_device->sendTrkMessage(TrkDisconnect); // Disconnect, as trk might be still connected
    d->m_device->sendTrkMessage(TrkSupported, TrkCallback(this, &Launcher::handleSupportMask));
    d->m_device->sendTrkMessage(TrkCpuType, TrkCallback(this, &Launcher::handleCpuType));
    d->m_device->sendTrkMessage(TrkVersions, TrkCallback(this, &Launcher::handleTrkVersion));
    if (d->m_startupActions != ActionPingOnly)
        d->m_device->sendTrkMessage(TrkConnect, TrkCallback(this, &Launcher::handleConnect));
    return true;
}

void Launcher::slotWaitingForTrk()
{
    // Set temporary state if we are still in connected state
    if (state() == Connecting)
        setState(WaitingForTrk);
}

void Launcher::handleConnect(const TrkResult &result)
{
    if (result.errorCode()) {
        emit canNotConnect(result.errorString());
        return;
    }
    setState(Connected);
    if (d->m_startupActions & ActionCopy)
        copyFileToRemote();
    else if (d->m_startupActions & ActionInstall)
        installRemotePackageSilently();
    else if (d->m_startupActions & ActionRun)
        startInferiorIfNeeded();
}

void Launcher::setVerbose(int v)
{
    d->m_verbose = v;
    d->m_device->setVerbose(v);
}

void Launcher::logMessage(const QString &msg)
{
    if (d->m_verbose)
        qDebug() << "LAUNCHER: " << qPrintable(msg);
}

void Launcher::terminate()
{
    switch (state()) {
    case DeviceDescriptionReceived:
    case Connected:
        if (d->m_session.pid) {
            QByteArray ba;
            appendShort(&ba, 0x0000, TargetByteOrder);
            appendInt(&ba, d->m_session.pid, TargetByteOrder);
            d->m_device->sendTrkMessage(TrkDeleteItem, TrkCallback(this, &Launcher::handleRemoteProcessKilled), ba);
            return;
        }
        if (d->m_copyState.copyFileHandle)
            closeRemoteFile(true);
        disconnectTrk();
        break;
    case Disconnected:
        break;
    case Connecting:
    case WaitingForTrk:
        setState(Disconnected);
        emit finished();
        break;
    }
}

void Launcher::handleRemoteProcessKilled(const TrkResult &result)
{
    Q_UNUSED(result)
    disconnectTrk();
}

QString Launcher::msgStopped(uint pid, uint tid, uint address, const QString &why)
{
    return QString::fromLatin1("Process %1, thread %2 stopped at 0x%3: %4").
            arg(pid).arg(tid).arg(address, 0, 16).
            arg(why.isEmpty() ? QString::fromLatin1("<Unknown reason>") : why);
}

bool Launcher::parseNotifyStopped(const QByteArray &dataBA,
                                  uint *pid, uint *tid, uint *address,
                                  QString *why /* = 0 */)
{
    if (why)
        why->clear();
    *address = *pid = *tid = 0;
    if (dataBA.size() < 12)
        return false;
    const char *data = dataBA.data();
    *address = extractInt(data);
    *pid = extractInt(data + 4);
    *tid = extractInt(data + 8);
    if (why && dataBA.size() >= 14) {
        const unsigned short len = extractShort(data + 12);
        if (len > 0)
            *why = QString::fromLatin1(data + 14, len);
    }
    return true;
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
        case TrkNotifyAck:
            break;
        case TrkNotifyNak: { // NAK
            logMessage(prefix + "NAK: " + str);
            //logMessage(prefix << "TOKEN: " << result.token);
            logMessage(prefix + "ERROR: " + errorMessage(result.data.at(0)));
            break;
        }
        case TrkNotifyStopped: { // Notified Stopped
            QString reason;
            uint pc;
            uint pid;
            uint tid;
            parseNotifyStopped(result.data, &pid, &tid, &pc, &reason);
            logMessage(prefix + msgStopped(pid, tid, pc, reason));
            emit(processStopped(pc, pid, tid, reason));
            d->m_device->sendTrkAck(result.token);
            break;
        }
        case TrkNotifyException: { // Notify Exception (obsolete)
            logMessage(prefix + "NOTE: EXCEPTION  " + str);
            d->m_device->sendTrkAck(result.token);
            break;
        }
        case TrkNotifyInternalError: { //
            logMessage(prefix + "NOTE: INTERNAL ERROR: " + str);
            d->m_device->sendTrkAck(result.token);
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

            if (result.data.size() < 10)
                break;
            QByteArray ba;
            ba.append(result.data.mid(2, 8));
            d->m_device->sendTrkMessage(TrkContinue, TrkCallback(), ba, "CONTINUE");
            //d->m_device->sendTrkAck(result.token)
            break;
        }
        case TrkNotifyDeleted: { // NotifyDeleted
            const ushort itemType = (unsigned char)result.data.at(1);
            const ushort len = result.data.size() > 12 ? extractShort(result.data.data() + 10) : ushort(0);
            const QString name = len ? QString::fromAscii(result.data.mid(12, len)) : QString();
            logMessage(QString::fromLatin1("%1 %2 UNLOAD: %3").
                       arg(QString::fromAscii(prefix)).arg(itemType ? QLatin1String("LIB") : QLatin1String("PROCESS")).
                       arg(name));
            d->m_device->sendTrkAck(result.token);
            if (itemType == 0 // process
                && result.data.size() >= 10
                && d->m_session.pid == extractInt(result.data.data() + 6)) {
                disconnectTrk();
            }
            break;
        }
        case TrkNotifyProcessorStarted: { // NotifyProcessorStarted
            logMessage(prefix + "NOTE: PROCESSOR STARTED: " + str);
            d->m_device->sendTrkAck(result.token);
            break;
        }
        case TrkNotifyProcessorStandBy: { // NotifyProcessorStandby
            logMessage(prefix + "NOTE: PROCESSOR STANDBY: " + str);
            d->m_device->sendTrkAck(result.token);
            break;
        }
        case TrkNotifyProcessorReset: { // NotifyProcessorReset
            logMessage(prefix + "NOTE: PROCESSOR RESET: " + str);
            d->m_device->sendTrkAck(result.token);
            break;
        }
        default: {
            logMessage(prefix + "INVALID: " + str);
            break;
        }
    }
}

QString Launcher::deviceDescription(unsigned verbose) const
{
    return d->m_session.deviceDescription(verbose);
}

void Launcher::handleTrkVersion(const TrkResult &result)
{
    if (result.errorCode() || result.data.size() < 5) {
        if (d->m_startupActions == ActionPingOnly) {
            setState(Disconnected);
            emit finished();
        }
        return;
    }
    d->m_session.trkAppVersion.trkMajor = result.data.at(1);
    d->m_session.trkAppVersion.trkMinor = result.data.at(2);
    d->m_session.trkAppVersion.protocolMajor = result.data.at(3);
    d->m_session.trkAppVersion.protocolMinor = result.data.at(4);
    setState(DeviceDescriptionReceived);
    // Ping mode: Log & Terminate
    if (d->m_startupActions == ActionPingOnly) {
        qWarning("%s", qPrintable(deviceDescription()));
        setState(Disconnected);
        emit finished();
    }
}

void Launcher::handleFileCreation(const TrkResult &result)
{
    if (result.errorCode() || result.data.size() < 6) {
        emit canNotCreateFile(d->m_copyState.destinationFileName, result.errorString());
        disconnectTrk();
        return;
    }
    const char *data = result.data.data();
    d->m_copyState.copyFileHandle = extractInt(data + 2);
    QFile file(d->m_copyState.sourceFileName);
    file.open(QIODevice::ReadOnly);
    d->m_copyState.data.reset(new QByteArray(file.readAll()));
    d->m_copyState.position = 0;
    file.close();
    continueCopying();
}

void Launcher::handleCopy(const TrkResult &result)
{
    if (result.errorCode() || result.data.size() < 4) {
        closeRemoteFile(true);
        emit canNotWriteFile(d->m_copyState.destinationFileName, result.errorString());
        disconnectTrk();
    } else {
        continueCopying(extractShort(result.data.data() + 2));
    }
}

void Launcher::continueCopying(uint lastCopiedBlockSize)
{
    int size = d->m_copyState.data->length();
    d->m_copyState.position += lastCopiedBlockSize;
    if (size == 0)
        emit copyProgress(100);
    else {
        int percent = qMin((d->m_copyState.position*100)/size, 100);
        emit copyProgress(percent);
    }
    if (d->m_copyState.position < size) {
        QByteArray ba;
        appendInt(&ba, d->m_copyState.copyFileHandle, TargetByteOrder);
        appendString(&ba, d->m_copyState.data->mid(d->m_copyState.position, 2048), TargetByteOrder, false);
        d->m_device->sendTrkMessage(TrkWriteFile, TrkCallback(this, &Launcher::handleCopy), ba);
    } else {
        closeRemoteFile();
    }
}

void Launcher::closeRemoteFile(bool failed)
{
    QByteArray ba;
    appendInt(&ba, d->m_copyState.copyFileHandle, TargetByteOrder);
    appendDateTime(&ba, QDateTime::currentDateTime(), TargetByteOrder);
    d->m_device->sendTrkMessage(TrkCloseFile,
                               failed ? TrkCallback() : TrkCallback(this, &Launcher::handleFileCopied),
                               ba);
    d->m_copyState.data.reset();
    d->m_copyState.copyFileHandle = 0;
    d->m_copyState.position = 0;
}

void Launcher::handleFileCopied(const TrkResult &result)
{
    if (result.errorCode())
        emit canNotCloseFile(d->m_copyState.destinationFileName, result.errorString());
    if (d->m_startupActions & ActionInstall)
        installRemotePackageSilently();
    else if (d->m_startupActions & ActionRun)
        startInferiorIfNeeded();
    else
        disconnectTrk();
}

void Launcher::handleCpuType(const TrkResult &result)
{
    logMessage("HANDLE CPU TYPE: " + result.toString());
    if(result.errorCode() || result.data.size() < 7)
        return;
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
    if (result.errorCode()) {
        emit canNotRun(result.errorString());
        disconnectTrk();
        return;
    }
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
    d->m_device->sendTrkMessage(TrkContinue, TrkCallback(), ba, "CONTINUE");
}

void Launcher::handleWaitForFinished(const TrkResult &result)
{
    logMessage("   FINISHED: " + stringFromArray(result.data));
    setState(Disconnected);
    emit finished();
}

void Launcher::handleSupportMask(const TrkResult &result)
{
    if (result.errorCode() || result.data.size() < 32)
        return;
    const char *data = result.data.data() + 1;

    QString str = QLatin1String("SUPPORTED: ");
    for (int i = 0; i < 32; ++i) {
        //str.append("  [" + formatByte(data[i]) + "]: ");
        for (int j = 0; j < 8; ++j) {
            if (data[i] & (1 << j)) {
                str.append(QString::number(i * 8 + j, 16));
                str.append(QLatin1Char(' '));
            }
        }
    }
    logMessage(str);
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
    d->m_device->sendTrkMessage(TrkDeleteItem, TrkCallback(), ba, "Delete process");

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
//    sendTrkMessage(0x02, TrkCallback(this, &Launcher::handleDisconnect));
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    // Error: 0x00
}

void Launcher::disconnectTrk()
{
    d->m_device->sendTrkMessage(TrkDisconnect, TrkCallback(this, &Launcher::handleWaitForFinished));
}

void Launcher::copyFileToRemote()
{
    emit copyingStarted();
    QByteArray ba;
    appendByte(&ba, 0x10);
    appendString(&ba, d->m_copyState.destinationFileName.toLocal8Bit(), TargetByteOrder, false);
    d->m_device->sendTrkMessage(TrkOpenFile, TrkCallback(this, &Launcher::handleFileCreation), ba);
}

void Launcher::installRemotePackageSilently()
{
    emit installingStarted();
    QByteArray ba;
    appendByte(&ba, 'C');
    appendString(&ba, d->m_installFileName.toLocal8Bit(), TargetByteOrder, false);
    d->m_device->sendTrkMessage(TrkInstallFile, TrkCallback(this, &Launcher::handleInstallPackageFinished), ba);
}

void Launcher::handleInstallPackageFinished(const TrkResult &result)
{
    if (result.errorCode()) {
        emit canNotInstall(d->m_installFileName, result.errorString());
        disconnectTrk();
        return;
    } else {
        emit installingFinished();
    }
    if (d->m_startupActions & ActionRun) {
        startInferiorIfNeeded();
    } else {
        disconnectTrk();
    }
}

QByteArray Launcher::startProcessMessage(const QString &executable,
                                         const QStringList &arguments)
{
    // It's not started yet
    QByteArray ba;
    appendShort(&ba, 0, TargetByteOrder); // create new process
    appendByte(&ba, 0); // options - currently unused
    if(arguments.isEmpty()) {
        appendString(&ba, executable.toLocal8Bit(), TargetByteOrder);
        return ba;
    }
    // Append full command line as one string (leading length information).
    QByteArray commandLineBa;
    commandLineBa.append(executable.toLocal8Bit());
    commandLineBa.append('\0');
    commandLineBa.append(arguments.join(QString(QLatin1Char(' '))).toLocal8Bit());
    appendString(&ba, commandLineBa, TargetByteOrder);
    return ba;
}

void Launcher::startInferiorIfNeeded()
{
    emit startingApplication();
    if (d->m_session.pid != 0) {
        logMessage("Process already 'started'");
        return;
    }
    d->m_device->sendTrkMessage(TrkCreateItem, TrkCallback(this, &Launcher::handleCreateProcess),
                                startProcessMessage(d->m_fileName, d->m_commandLineArgs)); // Create Item
}

void Launcher::resumeProcess(uint pid, uint tid)
{
    QByteArray ba;
    appendInt(&ba, pid, BigEndian);
    appendInt(&ba, tid, BigEndian);
    d->m_device->sendTrkMessage(TrkContinue, TrkCallback(), ba, "CONTINUE");
}
} // namespace trk
