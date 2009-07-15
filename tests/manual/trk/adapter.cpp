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


class Adapter : public QObject
{
    Q_OBJECT

public:
    Adapter();
    ~Adapter();
    void setGdbServerName(const QString &name);
    void setTrkServerName(const QString &name) { m_trkServerName = name; }
    void startServer();

public slots:
    void handleGdbConnection();
    void readFromGdb();
    void readFromTrk();

private:
    void handleGdbResponse(const QByteArray &ba);
    void writeToGdb(const QByteArray &msg);
    void writeAckToGdb();
    void logMessage(const QString &msg);

    QLocalSocket *m_trkClient;
    QTcpServer *m_gdbServer;
    QTcpSocket *m_gdbConnection;
    QString m_trkServerName;
    QString m_gdbServerName;
    quint16 m_gdbServerPort;
    QByteArray m_gdbReadBuffer;
    QByteArray m_trkReadBuffer;
};

Adapter::Adapter()
{
    m_trkClient = new QLocalSocket(this);
    m_gdbServer = new QTcpServer(this);
    m_gdbConnection = 0;
}

Adapter::~Adapter()
{
    m_gdbServer->close();
    logMessage("Shutting down");
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
    if (!m_gdbServer->listen(QHostAddress(m_gdbServerName), m_gdbServerPort)) {
        logMessage(QString("Unable to start the gdb server at %1:%2: %3.")
            .arg(m_gdbServerName).arg(m_gdbServerPort)
            .arg(m_gdbServer->errorString()));
        return;
    }

    logMessage(QString("Gdb server runnung on port %1. Run the Gdb Client now.")
           .arg(m_gdbServer->serverPort()));

    connect(m_gdbServer, SIGNAL(newConnection()), this, SLOT(handleGdbConnection()));
}

void Adapter::logMessage(const QString &msg)
{
    qDebug() << "ADAPTER: " << qPrintable(msg);
}

void Adapter::handleGdbConnection()
{
    logMessage("HANDLING GDB CONNECTION");

    m_gdbConnection = m_gdbServer->nextPendingConnection();
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
            logMessage("ACK");
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

        logMessage(QString("Packet checksum: %1").arg(checkSum));
        uint sum = 0;
        for (int i = 0; i < pos; ++i)
            sum += ba.at(i);

        if (sum % 256 != checkSum) {
            logMessage(QString("Packet checksum wrong: %1 %2 in " + ba)
                .arg(checkSum).arg(sum % 256));
        }

        QByteArray response = ba.left(pos);
        ba = ba.mid(pos + 3);
        handleGdbResponse(response);
    }
}

void Adapter::writeAckToGdb()
{
    QByteArray packet = "+";
    logMessage("gdb: <- " + packet);
    m_gdbConnection->write(packet);
}

void Adapter::writeToGdb(const QByteArray &msg)
{
    uint sum = 0;
    for (int i = 0; i != msg.size(); ++i) 
        sum += msg.at(i);
    QByteArray checkSum = QByteArray::number(sum % 256, 16);
    //logMessage(QString("Packet checksum: %1").arg(sum));

    QByteArray packet;
    packet.append("+$");
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
        logMessage("Handling 'qSupported'");
        writeAckToGdb();
        writeToGdb(QByteArray());
    }

    else if (response.startsWith("Hc")) {
        // Set thread for subsequent operations (`m', `M', `g', `G', et.al.).
        // for step and continue operations
        writeAckToGdb();
    }

    else if (response.startsWith("Hg")) {
        // Set thread for subsequent operations (`m', `M', `g', `G', et.al.).
        // for 'other operations
        //$Hg0#df 
        writeAckToGdb();
    }


    else if (response.startsWith("?")) {
        // Indicate the reason the target halted.
        // The reply is the same as for step and continue.

        //$?#3f 
        //$Hc-1#09 
        //$qC#b4 
        //$qAttached#8f 
        //$qOffsets#4b 
        //$qOffsets#4b 

    else {
        logMessage("FIXME unknown" + response); 
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
