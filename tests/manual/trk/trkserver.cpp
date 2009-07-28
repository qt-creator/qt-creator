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
#include <QtCore/QFile>
#include <QtCore/QStringList>

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

struct TrkOptions {
    TrkOptions() : verbose(1) {}

    int verbose;
    QString serverName;
    QString dumpName;
};


// Format of the replay source is something like
// ---IDE------------------------------------------------------
//  Command: 0x05 Support Mask
// [05 02]
//---TRK------------------------------------------------------
//  Command: 0x80 Acknowledge
//    Error: 0x0
// [80 02 00 7E 00 4F 5F 01 00 00 00 0F 1F 00 00 00
//  00 00 00 01 00 11 00 03 00 00 00 00 00 03 00 00...]

struct Inferior
{
    Inferior();
    uint pid;
    uint tid;
    uint codeseg;
    uint dataseg;

    uint registers[RegisterCount];
};

Inferior::Inferior()
{
    pid = 0x000008F5;
    tid = 0x000008F6;

    codeseg = 0x786A4000;
    dataseg = 0x00400000;

    registers[0]  = 0x00000000;
    registers[1]  = 0xC92D7FBC;
    registers[2]  = 0x00000000;
    registers[3]  = 0x00600000;
    registers[4]  = 0x00000000;
    registers[5]  = 0x786A7970;
    registers[6]  = 0x00000000;
    registers[7]  = 0x00000000;
    registers[8]  = 0x00000012;
    registers[9]  = 0x00000040;
    registers[10] = 0xC82AF210;
    registers[11] = 0x00000000;
    registers[12] = 0xC8000548;
    registers[13] = 0x00403ED0;
    registers[14] = 0x786A6BD8;
    registers[15] = 0x786A4CC8;
    registers[16] = 0x68000010; // that's reg 25 on chip?
}

class TrkServer : public QObject
{
    Q_OBJECT

public:
    TrkServer();
    ~TrkServer();

    void setServerName(const QString &name) { m_serverName = name; }
    void setMemoryDumpName(const QString &source) { m_memoryDumpName = source; }
    void setVerbose(int v) { m_verbose = v; }
    bool startServer();

private slots:
    void handleConnection();
    void readFromAdapter();
    void writeToAdapter(byte command, byte token, const QByteArray &data);

    void handleAdapterMessage(const TrkResult &result);

private:
    void logMessage(const QString &msg, bool force = 0);
    byte nextNotificationToken();

    QString m_serverName;
    QString m_memoryDumpName;

    QByteArray m_adapterReadBuffer;

    QByteArray m_memoryData;
    QLocalServer m_server;
    int m_lastSent;
    QLocalSocket *m_adapterConnection;
    Inferior m_inferior;
    byte m_notificationToken;
    int m_verbose;
};

TrkServer::TrkServer() : m_verbose(1)
{
    m_adapterConnection = 0;
    m_notificationToken = 0;
}

TrkServer::~TrkServer()
{
    logMessage("Shutting down.\n");
    m_server.close();
}

bool TrkServer::startServer()
{
    QFile file(m_memoryDumpName);
    if (!file.open(QIODevice::ReadOnly)) {
        logMessage(QString::fromLatin1("Unable to read %1: %2").arg(m_memoryDumpName, file.errorString()), true);
        return false;
    }
    m_memoryData = file.readAll();
    file.close();

    logMessage(QString("Read %1 bytes of data from %2")
        .arg(m_memoryData.size()).arg(m_memoryDumpName), true);

    m_lastSent = 0;
    if (!m_server.listen(m_serverName)) {
        logMessage(QString("Error: Unable to start the TRK server %1: %2.")
            .arg(m_serverName).arg(m_server.errorString()), true);
        return false;
    }

    logMessage("The TRK server '" + m_serverName + "'is running. Run the adapter now.", true);
    connect(&m_server, SIGNAL(newConnection()), this, SLOT(handleConnection()));
    return true;
}

void TrkServer::logMessage(const QString &msg, bool force)
{
    if (m_verbose || force)
        qDebug("TRKSERVER: %s", qPrintable(msg));
}

void TrkServer::handleConnection()
{
    m_adapterConnection = m_server.nextPendingConnection();
    connect(m_adapterConnection, SIGNAL(disconnected()),
            m_adapterConnection, SLOT(deleteLater()));
    connect(m_adapterConnection, SIGNAL(readyRead()),
            this, SLOT(readFromAdapter()));
}

void TrkServer::readFromAdapter()
{
    QByteArray packet = m_adapterConnection->readAll();
    m_adapterReadBuffer.append(packet);

    logMessage("trk: -> " + stringFromArray(packet));

    if (packet != m_adapterReadBuffer)
        logMessage("buffer: " + stringFromArray(m_adapterReadBuffer));

    while (!m_adapterReadBuffer.isEmpty())
        handleAdapterMessage(extractResult(&m_adapterReadBuffer));
}

void TrkServer::writeToAdapter(byte command, byte token, const QByteArray &data)
{
    QByteArray msg = frameMessage(command, token, data);
    logMessage("trk: <- " + stringFromArray(msg));
    m_adapterConnection->write(msg);
}

void TrkServer::handleAdapterMessage(const TrkResult &result)
{
    QByteArray data;
    data.append(char(0x00));  // No error
    switch (result.code) {
        case 0x00: { // Ping
            m_notificationToken = 0;
            writeToAdapter(0x80, 0x00, data);
            break;
        }
        case 0x01: { // Connect
            writeToAdapter(0x80, result.token, data);
            break;
        }
        case 0x10: { // Read Memory
            const char *p = result.data.data();
            byte option = p[0];
            Q_UNUSED(option);
            ushort len = extractShort(p + 1);
            uint addr = extractInt(p + 3);
            //qDebug() << "MESSAGE: " << result.data.toHex();
            qDebug() << "ADDR: " << hexNumber(addr) << " " << hexNumber(len);
            if (addr < m_inferior.codeseg
                || addr + len >= m_inferior.codeseg + m_memoryData.size()) {
                qDebug() << "ADDRESS OUTSIDE CODESEG: " << hexNumber(addr)
                    << hexNumber(m_inferior.codeseg);
                for (int i = 0; i != len / 4; ++i)
                    appendInt(&data, 0xDEADBEEF);
                writeToAdapter(0x80, result.token, data);
                break;
            }
            for (int i = 0; i != len; ++i)
                appendByte(&data, m_memoryData[addr - m_inferior.codeseg + i]);
            writeToAdapter(0x80, result.token, data);
            break;
        }
        case 0x12: { // Read Registers
            data.clear();
            for (int i = 0; i < RegisterCount; ++i)
                appendInt(&data, m_inferior.registers[i], BigEndian);
            writeToAdapter(0x80, result.token, data);
            break;
        }
        case 0x18: { // Continue
            writeToAdapter(0x80, result.token, data); // ACK Package

            if (1) { // Fake "Stop"
                QByteArray note;
                appendInt(&note, 0); // FIXME: use proper address
                appendInt(&note, m_inferior.pid);
                appendInt(&note, m_inferior.tid);
                appendByte(&note, 0x00);
                appendByte(&note, 0x00);
                writeToAdapter(0x90, nextNotificationToken(), note);
            }

            break;
        }
        case 0x19: { // Step
            const char *p = result.data.data();
            //byte option = *p;
            uint startaddr = extractInt(p + 1);
            uint endaddr = extractInt(p + 5);
            uint pid = extractInt(p + 9);
            //uint tid = extractInt(p + 13);
            if (startaddr != m_inferior.registers[RegisterPC])
                logMessage("addr mismatch:" + hexNumber(startaddr) + " " +
                    hexNumber(m_inferior.registers[RegisterPC]));
            if (pid != m_inferior.pid)
                logMessage("pid mismatch:" + hexNumber(pid) + " " +
                    hexNumber(m_inferior.pid));
            writeToAdapter(0x80, result.token, data);

            // Fake "step"
            m_inferior.registers[RegisterPC] = endaddr;

            if (1) { // Fake "Stop"
                QByteArray note;
                appendInt(&note, 0); // FIXME: use proper address
                appendInt(&note, m_inferior.pid);
                appendInt(&note, m_inferior.tid);
                appendByte(&note, 0x00);
                appendByte(&note, 0x00);
                writeToAdapter(0x90, nextNotificationToken(), note);
            }
            break;
        }
        case 0x40: { // Create Item
            appendInt(&data, m_inferior.pid, BigEndian);
            appendInt(&data, m_inferior.tid, BigEndian);
            appendInt(&data, m_inferior.codeseg, BigEndian);
            appendInt(&data, m_inferior.dataseg, BigEndian);
            writeToAdapter(0x80, result.token, data);
            break;
        }
        case 0x41: { // Delete Item
            writeToAdapter(0x80, result.token, data);

            // A Process?
            // Command: 0xA1 Notify Deleted
            //[A1 02 00 00 00 00 00 00 00 00 01 B5]
            QByteArray note; // FIXME
            appendByte(&note, 0);
            appendByte(&note, 0);
            appendInt(&note, 0);
            appendInt(&note, m_inferior.pid);
            writeToAdapter(0xA1, nextNotificationToken(), note);
            break;
        }
        default:
            data[0] = 0x10; // Command not supported
            writeToAdapter(0xff, result.token, data);
            break;
    }

}

byte TrkServer::nextNotificationToken()
{
    ++m_notificationToken;
    if (m_notificationToken == 0)
        ++m_notificationToken;
    return m_notificationToken;
}

static bool readTrkArgs(const QStringList &args, TrkOptions *o)
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
            }
        } else {
            switch (argNumber++) {
            case 0:
                o->serverName = *it;
                break;
            case 1:
                o->dumpName = *it;
                break;
            }
        }
    }
    return !o->dumpName.isEmpty();
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_UNIX
    signal(SIGUSR1, signalHandler);
#endif

    QCoreApplication app(argc, argv);
    TrkOptions options;
    if (!readTrkArgs(app.arguments(), &options)) {
        qWarning("Usage: %s [-v|-q] <trkservername> <replaysource>\n"
                 "Options: -v verbose\n"
                 "         -q quiet\n", argv[0]);
        return 1;
    }

    TrkServer server;
    server.setServerName(options.serverName);
    server.setMemoryDumpName(options.dumpName);
    server.setVerbose(options.verbose);
    if (!server.startServer())
        return -1;

    return app.exec();
}

#include "trkserver.moc"
