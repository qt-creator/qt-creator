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

    trk::TrkClient m_trkClient;
    QString m_trkServerName;
    QByteArray m_trkReadBuffer;

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
    bool m_ackMode;

    uint m_registers[100];
};

Adapter::Adapter()
{
    m_gdbConnection = 0;
    m_ackMode = true;
}

Adapter::~Adapter()
{
    m_gdbServer.close();
    //m_trkClient->disconnectFromServer();
    m_trkClient.abort();
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
    if (!m_trkClient.openPort(m_trkServerName)) {
        logMessage("Unable to connect to TRK server");
        return;
    }

    m_trkClient.sendInitialPing();
    m_trkClient.sendMessage(0x01); // Connect
    //m_trkClient.sendMessage(0x05, CB(handleSupportMask));
    //m_trkClient.sendMessage(0x06, CB(handleCpuType));
    m_trkClient.sendMessage(0x04); // Versions
    //sendMessage(0x09); Unrecognized command
    m_trkClient.sendMessage(0x4a, 0,
        "10 " + formatString("C:\\data\\usingdlls.sisx")); // Open File
    m_trkClient.sendMessage(0x4B, 0, "00 00 00 01 73 1C 3A C8"); // Close File

    QByteArray exe = "C:\\sys\\bin\\UsingDLLs.exe";
    exe.append('\0');
    exe.append('\0');
    //m_trkClient.sendMessage(0x40, CB(handleCreateProcess),
    //    "00 00 00 " + formatString(exe)); // Create Item

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
    if (!m_ackMode)
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
        m_ackMode = false;
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
