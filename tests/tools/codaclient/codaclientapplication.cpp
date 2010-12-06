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

#include "tcftrkdevice.h"
#include <QtNetwork/QTcpSocket>

#include <cstdio>

static const char usageC[] =
"\n%1 v0.1 built "__DATE__"\n\n"
"Test client for Symbian CODA\n\n"
"Usage:\n"
"%1 launch [-d] address:port binary uid [--] [arguments]\n"
"%1 install     address:port sis-file targetdrive\n"
"%1 copy        address:port local-file remote-file\n"
"\nOptions:\n"
"-d            Launch under Debug control\n";

static const unsigned short defaultPort = 65029;

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
            .arg(QCoreApplication::applicationName());
}

static inline CodaClientApplication::Mode modeArg(const QString &a)
{
    if (a == QLatin1String("launch"))
        return CodaClientApplication::Launch;
    if (a == QLatin1String("install"))
        return CodaClientApplication::Install;
    if (a == QLatin1String("copy"))
        return CodaClientApplication::Copy;
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
        case Copy:
            m_copyLocalFile = a;
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
        case Copy:
            m_copyRemoteFile = fixSlashes(a);
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
    case Install:
        if (m_address.isEmpty() || m_installSisFile.isEmpty()) {
            *errorMessage = QString::fromLatin1("Not enough parameters for install.");
            return ParseInitError;
        }
        break;
    case Copy:
        if (m_address.isEmpty() || m_copyLocalFile.isEmpty() || m_copyRemoteFile.isEmpty()) {
            *errorMessage = QString::fromLatin1("Not enough parameters for copy.");
            return ParseInitError;
        }
        break;
    default:
        break;
    }
    return ParseArgsOk;
}

bool CodaClientApplication::start()
{
    switch (m_mode) {
    case Launch: {
        const QString args = m_launchArgs.join(QString(QLatin1Char(' ')));
        std::printf("Launching 0x%x '%s '%s' on %s:%hu (debug: %d)\n",
                    m_launchUID, qPrintable(m_launchBinary),
                    qPrintable(args), qPrintable(m_address), m_port,
                    m_launchDebug);
    }
        break;
    case Install:
        std::printf("Installing '%s' to '%s' on %s:%hu\n",
                    qPrintable(m_installSisFile), qPrintable(m_installTargetDrive),
                    qPrintable(m_address), m_port);
        break;
    case Copy:
        std::printf("Copying '%s' to '%s' on %s:%hu\n",
                    qPrintable(m_copyLocalFile), qPrintable(m_copyRemoteFile),
                    qPrintable(m_address), m_port);
        break;
    case Invalid:
        break;
    }
    // Start connection
    const QSharedPointer<QTcpSocket> tcfTrkSocket(new QTcpSocket);
    m_trkDevice.reset(new tcftrk::TcfTrkDevice);
    m_trkDevice->setVerbose(m_verbose);
    connect(m_trkDevice.data(), SIGNAL(error(QString)),
        this, SLOT(slotError(QString)));
    connect(m_trkDevice.data(), SIGNAL(logMessage(QString)),
        this, SLOT(slotTrkLogMessage(QString)));
    connect(m_trkDevice.data(), SIGNAL(tcfEvent(tcftrk::TcfTrkEvent)),
        this, SLOT(slotTcftrkEvent(tcftrk::TcfTrkEvent)));
    m_trkDevice->setDevice(tcfTrkSocket);
    tcfTrkSocket->connectToHost(m_address, m_port);
    std::printf("Connecting...\n");
    return true;
}

void CodaClientApplication::slotError(const QString &e)
{
    std::fprintf(stderr, "Error: %s\n", qPrintable(e));
    doExit(-1);
}

void CodaClientApplication::slotTrkLogMessage(const QString &m)
{
    std::printf("%s\n", qPrintable(m));
}

void CodaClientApplication::handleCreateProcess(const tcftrk::TcfTrkCommandResult &result)
{
    const bool ok = result.type == tcftrk::TcfTrkCommandResult::SuccessReply;
    if (ok) {
        std::printf("Launch succeeded: %s\n", qPrintable(result.toString()));
        if (!m_launchDebug)
            doExit(0);
    } else {
        std::fprintf(stderr, "Launch failed: %s\n", qPrintable(result.toString()));
        doExit(-1);
    }
}

void CodaClientApplication::slotTcftrkEvent (const tcftrk::TcfTrkEvent &ev)
{
    std::printf("Event: %s\n", qPrintable(ev.toString()));
    switch (ev.type()) {
    case tcftrk::TcfTrkEvent::LocatorHello: // Commands accepted now
        switch (m_mode) {
        case Launch:
            m_trkDevice->sendProcessStartCommand(tcftrk::TcfTrkCallback(this, &CodaClientApplication::handleCreateProcess),
                                                 m_launchBinary, m_launchUID, m_launchArgs, QString(), m_launchDebug);
            break;
        case Install:
            break;
        case Copy:
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

void CodaClientApplication::doExit(int ex)
{
    if (!m_trkDevice.isNull()) {
        const QSharedPointer<QIODevice> dev = m_trkDevice->device();
        if (QAbstractSocket *socket = qobject_cast<QAbstractSocket *>(dev.data())) {
            if (socket->state() == QAbstractSocket::ConnectedState)
                socket->disconnectFromHost();
        } else {
            if (dev->isOpen())
                dev->close();
        }
    }
    std::printf("Exiting (%d)\n", ex);
    exit(ex);
}
