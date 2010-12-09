/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "codaclientapplication.h"

#ifdef HAS_SERIALPORT
#    include <qextserialport/qextserialport.h>
#endif

#include "tcftrkdevice.h"
#include <QtNetwork/QTcpSocket>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <cstdio>

static const char usageC[] =
"\n%1 v0.1 built "__DATE__"\n\n"
"Test client for Symbian CODA\n\n"
"Usage:\n"
"%1 ping            connection   Note: For serial connections ONLY.\n"
"%1 launch [-d]     connection binary uid [--] [arguments]\n"
"%1 install[-s]     connection remote-sis-file [targetdrive]\n"
"%1 put    [c size] connection local-file remote-file\n"
"%1 stat            connection remote-file\n\n"
"'connection': address[:port] or serial-port\n\n"
"Options:\n"
"-d            Launch: Launch under Debug control (wait for termination)\n"
"-c [size]     Put: Chunk size in KB (default %2KB)\n"
"-s            Install: Silent installation\n\n"
"Notes:\n"
"UIDs take the form '0xfdaa278'. The target directory for sis-files on a\n"
"device typically is 'c:\\data'. CODA's default port is %3.\n\n"
"Example session:\n"
"%1 put     192.168.0.42 test.sis c:\\data\\test.sis\n"
"%1 stat    192.168.0.42 c:\\data\\test.sis\n"
"%1 install 192.168.0.42 c:\\data\\test.sis c:\n"
"%1 launch  192.168.0.42 c:\\sys\\bin\\test.exe  0x34f2b\n"
"%1 ping    /dev/ttyUSB1\n";

static const unsigned short defaultPort = 65029;
static const quint64  defaultChunkSize = 10240;

static inline bool isSerialPort(const QString &address)
{
    return address.startsWith(QLatin1String("/dev"))
        || address.startsWith(QLatin1String("com"), Qt::CaseInsensitive)
        || address.startsWith(QLatin1String("tty"), Qt::CaseInsensitive)
        || address.startsWith(QLatin1Char('\\'));
}

static inline QString fixSlashes(QString s)
{
    s.replace(QLatin1Char('/'), QLatin1Char('\\'));
    return s;
}

CodaClientApplication::CodaClientApplication(int &argc, char **argv) :
    QCoreApplication(argc, argv),
    m_mode(Invalid),
    m_port(defaultPort),
    m_launchUID(0),
    m_launchDebug(false),
    m_installTargetDrive(QLatin1String("C:")),
    m_installSilently(false),
    m_putWriteOk(false),
    m_statFstatOk(false),
    m_putLastChunkSize(0),
    m_putChunkSize(defaultChunkSize),
    m_verbose(0)
{
    setApplicationName(QLatin1String("codaclient"));
}

CodaClientApplication::~CodaClientApplication()
{
}

QString CodaClientApplication::usage()
{
    return QString::fromLatin1(usageC)
            .arg(QCoreApplication::applicationName())
            .arg(defaultChunkSize / 1024).arg(defaultPort);
}

static inline CodaClientApplication::Mode modeArg(const QString &a)
{
    if (a == QLatin1String("launch"))
        return CodaClientApplication::Launch;
    if (a == QLatin1String("ping"))
        return CodaClientApplication::Ping;
    if (a == QLatin1String("install"))
        return CodaClientApplication::Install;
    if (a == QLatin1String("put"))
        return CodaClientApplication::Put;
    if (a == QLatin1String("stat"))
        return CodaClientApplication::Stat;
    return CodaClientApplication::Invalid;
}

bool CodaClientApplication::parseArgument(const QString &a, int argNumber, QString *errorMessage)
{
    switch (argNumber) {
    case 1:
        m_mode = modeArg(a);
        if (m_mode == Invalid) {
            *errorMessage = QLatin1String("Invalid mode");
            return false;
        }
        return true;
    case 2:  { // Address/port
        m_address = a;
        const int colonPos = m_address.indexOf(':');
        if (colonPos != -1) {
            m_port = m_address.mid(colonPos + 1).toUInt();
            m_address.truncate(colonPos);
        }
    }
        break;
    case 3:
        switch (m_mode) {
        case Launch:
            m_launchBinary = fixSlashes(a);
            break;
        case Install:
            m_installSisFile = a;
            break;
        case Put:
            m_putLocalFile = a;
            break;
        case Stat:
            m_statRemoteFile = fixSlashes(a);
            break;
        default:
            break;
        }
        return true;
    case 4:
        switch (m_mode) {
        case Launch:
            m_launchUID = a.toUInt(0, 0);
            if (!m_launchUID) {
                *errorMessage = QLatin1String("Invalid UID");
                return false;
            }
            break;
        case Install:
            m_installTargetDrive = a;
            break;
        case Put:
            m_putRemoteFile = fixSlashes(a);
            break;
        default:
            break;
        }
        return true;

    default:
        if (m_mode == Launch)
            m_launchArgs.push_back(a);
        break;
    }
    return true;
}

CodaClientApplication::ParseArgsResult CodaClientApplication::parseArguments(QString *errorMessage)
{
    int argNumber = 1;
    const QStringList args = QCoreApplication::arguments();
    const QStringList::const_iterator cend = args.constEnd();
    QStringList::const_iterator it = args.constBegin();
    bool optionsEnd = false;
    for (++it; it != cend; ++it) {
        if (!optionsEnd  && *it == QLatin1String("--")) {
            optionsEnd = true;
            continue;
        }
        if (!optionsEnd &&  it->startsWith(QLatin1Char('-')) && it->size() == 2) {
            switch (it->at(1).toAscii()) {
            case 'v':
                m_verbose++;
                break;
            case 'd':
                m_launchDebug = true;
                break;
            case 's':
                m_installSilently = true;
                break;
            case 'c':
                if (++it == cend) {
                    *errorMessage = QString::fromLatin1("Parameter missing for -c");
                    return ParseArgsError;
                }
                m_putChunkSize = it->toULongLong() * 1024;
                if (!m_putChunkSize) {
                    *errorMessage = QString::fromLatin1("Invalid chunk size.");
                    return ParseArgsError;
                }
                break;
            default:
                *errorMessage = QString::fromLatin1("Invalid option %1").arg(*it);
                return ParseArgsError;
            }
        } else {
            if (!parseArgument(*it, argNumber++, errorMessage))
                return ParseArgsError;
        }
    } //for loop
    // Basic Check & init
    switch (m_mode) {
    case Invalid:
        return ParseArgsError;
    case Launch:
        if (m_address.isEmpty() || !m_launchUID || m_launchBinary.isEmpty()) {
            *errorMessage = QString::fromLatin1("Not enough parameters for launch.");
            return ParseInitError;
        }
        break;
    case Ping:
        if (m_address.isEmpty()) {
            *errorMessage = QString::fromLatin1("Not enough parameters for ping.");
            return ParseInitError;
        }
        if (!isSerialPort(m_address)) {
            *errorMessage = QString::fromLatin1("'ping' not supported for TCP/IP.");
            return ParseInitError;
        }
        break;
    case Install:
        if (m_address.isEmpty() || m_installSisFile.isEmpty()) {
            *errorMessage = QString::fromLatin1("Not enough parameters for install.");
            return ParseInitError;
        }
        break;
    case Put: {
        if (m_address.isEmpty() || m_putLocalFile.isEmpty() || m_putRemoteFile.isEmpty()) {
            *errorMessage = QString::fromLatin1("Not enough parameters for put.");
            return ParseInitError;
        }
        const QFileInfo fi(m_putLocalFile);
        if (!fi.isFile() || !fi.isReadable()) {
            *errorMessage = QString::fromLatin1("Local file '%1' not readable.").arg(m_putLocalFile);
            return ParseInitError;
        }
    }
        break;
    default:
        break;
    }
    return ParseArgsOk;
}

bool CodaClientApplication::start()
{
    m_startTime.start();
    switch (m_mode) {
    case Ping:
        break;
    case Launch: {
        const QString args = m_launchArgs.join(QString(QLatin1Char(' ')));
        std::printf("Launching 0x%x '%s '%s' (debug: %d)\n",
                    m_launchUID, qPrintable(m_launchBinary),
                    qPrintable(args), m_launchDebug);
    }
        break;
    case Install:
        std::printf("Installing '%s' to '%s'\n",
                    qPrintable(m_installSisFile), qPrintable(m_installTargetDrive));
        break;
    case Put:
        std::printf("Copying '%s' to '%s' in chunks of %lluKB\n",
                    qPrintable(m_putLocalFile), qPrintable(m_putRemoteFile),
                    m_putChunkSize / 1024);
        break;
    case Stat:
        std::printf("Retrieving attributes of '%s'\n", qPrintable(m_statRemoteFile));
        break;
    case Invalid:
        break;
    }
    // Start connection
    m_trkDevice.reset(new tcftrk::TcfTrkDevice);
    m_trkDevice->setVerbose(m_verbose);
    connect(m_trkDevice.data(), SIGNAL(error(QString)),
        this, SLOT(slotError(QString)));
    connect(m_trkDevice.data(), SIGNAL(logMessage(QString)),
        this, SLOT(slotTrkLogMessage(QString)));
    connect(m_trkDevice.data(), SIGNAL(tcfEvent(tcftrk::TcfTrkEvent)),
        this, SLOT(slotTcftrkEvent(tcftrk::TcfTrkEvent)));
    connect(m_trkDevice.data(), SIGNAL(serialPong(QString)),
            this, SLOT(slotSerialPong(QString)));
    if (isSerialPort(m_address)) {
#ifdef HAS_SERIALPORT
        // Serial
#ifdef Q_OS_WIN
        const QString fullPort = QextSerialPort::fullPortNameWin(m_address);
#else
        const QString fullPort = m_address;
#endif
        const QSharedPointer<QextSerialPort>
                serialPort(new QextSerialPort(fullPort, QextSerialPort::EventDriven));
        std::printf("Opening port %s...\n", qPrintable(fullPort));

        // Magic USB serial parameters
        serialPort->setTimeout(2000);
        serialPort->setBaudRate(BAUD115200);
        serialPort->setFlowControl(FLOW_OFF);
        serialPort->setParity(PAR_NONE);
        serialPort->setDataBits(DATA_8);
        serialPort->setStopBits(STOP_1);

        m_trkDevice->setSerialFrame(true);
        m_trkDevice->setDevice(serialPort); // Grab all data from start
        if (!serialPort->open(QIODevice::ReadWrite|QIODevice::Unbuffered)) {
            std::fprintf(stderr, "Cannot open port: %s", qPrintable(serialPort->errorString()));
            return false;
        }
        // Initiate communication
        m_trkDevice->sendSerialPing(m_mode == Ping);
        serialPort->flush();
#else
        std::fprintf(stderr, "Not implemented\n");
        return false;
#endif
    } else {
        // TCP/IP
        const QSharedPointer<QTcpSocket> tcfTrkSocket(new QTcpSocket);
        m_trkDevice->setDevice(tcfTrkSocket);
        tcfTrkSocket->connectToHost(m_address, m_port);
        std::printf("Connecting to %s:%hu...\n", qPrintable(m_address), m_port);
    }
    return true;
}

void CodaClientApplication::slotError(const QString &e)
{
    std::fprintf(stderr, "Error: %s\n", qPrintable(e));
    doExit(-1);
}

void CodaClientApplication::slotTrkLogMessage(const QString &m)
{
    printTimeStamp();
    std::printf("%s\n", qPrintable(m));
}

void CodaClientApplication::handleCreateProcess(const tcftrk::TcfTrkCommandResult &result)
{
    const bool ok = result.type == tcftrk::TcfTrkCommandResult::SuccessReply;
    if (ok) {
        printTimeStamp();
        std::printf("Launch succeeded: %s\n", qPrintable(result.toString()));
        if (!m_launchDebug)
            doExit(0);
    } else {
        std::fprintf(stderr, "Launch failed: %s\n", qPrintable(result.toString()));
        doExit(-1);
    }
}

void CodaClientApplication::handleFileSystemOpen(const tcftrk::TcfTrkCommandResult &result)
{
    if (result.type != tcftrk::TcfTrkCommandResult::SuccessReply) {
        std::fprintf(stderr, "Open remote file failed: %s\n", qPrintable(result.toString()));
        doExit(-1);
        return;
    }

    if (result.values.size() < 1 || result.values.at(0).data().isEmpty()) {
        std::fprintf(stderr, "Internal error: No filehandle obtained\n");
        doExit(-1);
        return;
    }

    m_remoteFileHandle = result.values.at(0).data();

    if (m_mode == Stat) {
        m_trkDevice->sendFileSystemFstatCommand(tcftrk::TcfTrkCallback(this, &CodaClientApplication::handleFileSystemFStat),
                                               m_remoteFileHandle);
        return;
    }
    // Put.
    m_putFile.reset(new QFile(m_putLocalFile));
    if (!m_putFile->open(QIODevice::ReadOnly)) { // Should not fail, was checked before
        std::fprintf(stderr, "Open local file failed: %s\n", qPrintable(m_putFile->errorString()));
        doExit(-1);
        return;
    }
    putSendNextChunk();
}

void CodaClientApplication::putSendNextChunk()
{
    // Read and send off next chunk
    const quint64 pos = m_putFile->pos();
    const QByteArray data = m_putFile->read(m_putChunkSize);
    if (data.isEmpty()) {
        m_putWriteOk = true;
        closeRemoteFile();
    } else {
        m_putLastChunkSize = data.size();
        printTimeStamp();
        std::printf("Writing %llu bytes to remote file '%s' at %llu\n",
                    m_putLastChunkSize,
                    m_remoteFileHandle.constData(), pos);
        m_trkDevice->sendFileSystemWriteCommand(tcftrk::TcfTrkCallback(this, &CodaClientApplication::handleFileSystemWrite),
                                                m_remoteFileHandle, data, unsigned(pos));
    }
}

void CodaClientApplication::closeRemoteFile()
{
    m_trkDevice->sendFileSystemCloseCommand(tcftrk::TcfTrkCallback(this, &CodaClientApplication::handleFileSystemClose),
                                            m_remoteFileHandle);
}

void CodaClientApplication::handleFileSystemWrite(const tcftrk::TcfTrkCommandResult &result)
{
    // Close remote file even if copy fails
    m_putWriteOk = result;
    if (!m_putWriteOk)
        std::fprintf(stderr, "Writing data failed: %s\n", qPrintable(result.toString()));
    if (!m_putWriteOk || m_putLastChunkSize < m_putChunkSize) {
        closeRemoteFile();
    } else {
        putSendNextChunk();
    }
}

void CodaClientApplication::handleFileSystemFStat(const tcftrk::TcfTrkCommandResult &result)
{
    m_statFstatOk = result.type == tcftrk::TcfTrkCommandResult::SuccessReply;
    // Close remote file even if copy fails
    if (m_statFstatOk) {
        const tcftrk::TcfTrkStatResponse statr = tcftrk::TcfTrkDevice::parseStat(result);
        printTimeStamp();
        std::printf("File: %s\nSize: %llu bytes\nAccessed: %s\nModified: %s\n",
                    qPrintable(m_statRemoteFile), statr.size,
                    qPrintable(statr.accessTime.toString(Qt::LocalDate)),
                    qPrintable(statr.modTime.toString(Qt::LocalDate)));
    } else {
        std::fprintf(stderr, "FStat failed: %s\n", qPrintable(result.toString()));
    }
    closeRemoteFile();
}

void CodaClientApplication::handleFileSystemClose(const tcftrk::TcfTrkCommandResult &result)
{
    if (result.type == tcftrk::TcfTrkCommandResult::SuccessReply) {
        printTimeStamp();
        std::printf("File closed.\n");
        const bool ok = m_mode == Put ? m_putWriteOk : m_statFstatOk;
        doExit(ok ? 0 : -1);
    } else {
        std::fprintf(stderr, "File close failed: %s\n", qPrintable(result.toString()));
        doExit(-1);
    }
}

void CodaClientApplication::handleSymbianInstall(const tcftrk::TcfTrkCommandResult &result)
{
    if (result.type == tcftrk::TcfTrkCommandResult::SuccessReply) {
        printTimeStamp();
        std::printf("Installation succeeded\n.");
        doExit(0);
    } else {
        std::fprintf(stderr, "Installation failed: %s\n", qPrintable(result.toString()));
        doExit(-1);
    }
}

void CodaClientApplication::slotTcftrkEvent (const tcftrk::TcfTrkEvent &ev)
{
    printTimeStamp();
    std::printf("Event: %s\n", qPrintable(ev.toString()));
    switch (ev.type()) {
    case tcftrk::TcfTrkEvent::LocatorHello: // Commands accepted now
        switch (m_mode) {
        case Ping:
            break;
        case Launch:
            m_trkDevice->sendProcessStartCommand(tcftrk::TcfTrkCallback(this, &CodaClientApplication::handleCreateProcess),
                                                 m_launchBinary, m_launchUID, m_launchArgs, QString(), m_launchDebug);
            break;
        case Install:
            if (m_installSilently) {
                m_trkDevice->sendSymbianInstallSilentInstallCommand(tcftrk::TcfTrkCallback(this, &CodaClientApplication::handleSymbianInstall),
                                                                    m_installSisFile.toAscii(), m_installTargetDrive.toAscii());
            } else {
                m_trkDevice->sendSymbianInstallUIInstallCommand(tcftrk::TcfTrkCallback(this, &CodaClientApplication::handleSymbianInstall),
                                                                m_installSisFile.toAscii());
            }
            break;
        case Put: {
            const unsigned flags =
                    tcftrk::TcfTrkDevice::FileSystem_TCF_O_WRITE
                    |tcftrk::TcfTrkDevice::FileSystem_TCF_O_CREAT
                    |tcftrk::TcfTrkDevice::FileSystem_TCF_O_TRUNC;
            m_putWriteOk = false;
            m_trkDevice->sendFileSystemOpenCommand(tcftrk::TcfTrkCallback(this, &CodaClientApplication::handleFileSystemOpen),
                                                   m_putRemoteFile.toAscii(), flags);
}
            break;
        case Stat: {
            const unsigned flags = tcftrk::TcfTrkDevice::FileSystem_TCF_O_READ;
            m_statFstatOk = false;
            m_trkDevice->sendFileSystemOpenCommand(tcftrk::TcfTrkCallback(this, &CodaClientApplication::handleFileSystemOpen),
                                                   m_statRemoteFile.toAscii(), flags);
}
            break;
        case Invalid:
            break;
        }
        break;
    case tcftrk::TcfTrkEvent::RunControlModuleLoadSuspended:  {
        // Debug mode start: Continue:
        const tcftrk::TcfTrkRunControlModuleLoadContextSuspendedEvent &me =
                static_cast<const tcftrk::TcfTrkRunControlModuleLoadContextSuspendedEvent &>(ev);
        if (me.info().requireResume) {
            printTimeStamp();
            std::printf("Continuing...\n");
            m_trkDevice->sendRunControlResumeCommand(tcftrk::TcfTrkCallback(), me.id());
        }
    }
        break;
    case tcftrk::TcfTrkEvent::RunControlContextRemoved: // App terminated in debug mode
        doExit(0);
        break;
    default:
        break;
    }
}

void CodaClientApplication::slotSerialPong(const QString &v)
{
    printTimeStamp();
    std::printf("Pong from '%s'\n", qPrintable(v));
    if (m_mode == Ping)
        doExit(0);
}

void CodaClientApplication::doExit(int ex)
{
    if (!m_trkDevice.isNull()) {
        const QSharedPointer<QIODevice> dev = m_trkDevice->device();
        if (!dev.isNull()) {
            if (QAbstractSocket *socket = qobject_cast<QAbstractSocket *>(dev.data())) {
                if (socket->state() == QAbstractSocket::ConnectedState)
                    socket->disconnectFromHost();
            } else {
                if (dev->isOpen())
                    dev->close();
            }
        }
    }
    printTimeStamp();
    std::printf("Exiting (%d)\n", ex);
    exit(ex);
}

void CodaClientApplication::printTimeStamp()
{
    const int elapsedMS = m_startTime.elapsed();
    const int secs = elapsedMS / 1000;
    const int msecs = elapsedMS % 1000;
    std::printf("%4d.%03ds: ", secs, msecs);
}
