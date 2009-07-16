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

// Format of the replay source is something like
// ---IDE------------------------------------------------------
//  Command: 0x05 Support Mask
// [05 02]
//---TRK------------------------------------------------------
//  Command: 0x80 Acknowledge
//    Error: 0x00
// [80 02 00 7E 00 4F 5F 01 00 00 00 0F 1F 00 00 00
//  00 00 00 01 00 11 00 03 00 00 00 00 00 03 00 00...]

class TrkServer : public QObject
{
    Q_OBJECT

public:
    TrkServer();
    ~TrkServer();

    void setServerName(const QString &name) { m_serverName = name; }
    void setReplaySource(const QString &source) { m_replaySource = source; }
    void startServer();

private slots:
    void handleConnection();
    void readFromAdapter();
    void writeToAdapter(byte command, byte token, const QByteArray &data);

    void handleAdapterMessage(const TrkResult &result);

private:
    void logMessage(const QString &msg);

    QString m_serverName;
    QString m_replaySource;

    QByteArray m_adapterReadBuffer;

    QList<QByteArray> m_replayData;
    QLocalServer m_server;
    int m_lastSent;
    QLocalSocket *m_adapterConnection;
};

TrkServer::TrkServer()
{
    m_adapterConnection = 0;
}

TrkServer::~TrkServer()
{
    logMessage("Shutting down.\n");
    m_server.close();
}

void TrkServer::startServer()
{
    QFile file(m_replaySource);
    file.open(QIODevice::ReadOnly);
    m_replayData = file.readAll().split('\n');
    file.close();

    logMessage(QString("Read %1 lines of data from %2")
        .arg(m_replayData.size()).arg(m_replaySource));

    m_lastSent = 0;
    if (!m_server.listen(m_serverName)) {
        logMessage(QString("Error: Unable to start the TRK server %1: %2.")
            .arg(m_serverName).arg(m_server.errorString()));
        return;
    }

    logMessage("The TRK server is running. Run the adapter now.");
    connect(&m_server, SIGNAL(newConnection()), this, SLOT(handleConnection()));
}

void TrkServer::logMessage(const QString &msg)
{
    qDebug() << "TRKSERVER: " << qPrintable(msg);
}

void TrkServer::handleConnection()
{
    //QByteArray block;

    //QByteArray msg = m_replayData[m_lastSent ++];

    //QDataStream out(&block, QIODevice::WriteOnly);
    //out.setVersion(QDataStream::Qt_4_0);
    //out << (quint16)0;
    //out << m_replayData;
    //out.device()->seek(0);
    //out << (quint16)(block.size() - sizeof(quint16));

    m_adapterConnection = m_server.nextPendingConnection();
    connect(m_adapterConnection, SIGNAL(disconnected()),
            m_adapterConnection, SLOT(deleteLater()));
    connect(m_adapterConnection, SIGNAL(readyRead()),
            this, SLOT(readFromAdapter()));

    //m_adapterConnection->write(block);
    //m_adapterConnection->flush();
    //m_adapterConnection->disconnectFromHost();
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
            writeToAdapter(0x80, 0x00, data);
            break;
        }
        case 0x01: { // Connect
            writeToAdapter(0x80, result.token, data);
            break;
        }
        default:
            data[0] = 0x10; // Command not supported
            writeToAdapter(0xff, result.token, data);
            break;
    }

}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        qDebug() << "Usage: " << argv[0] << " <trkservername>"
            << " <replaysource>";
        return 1;
    }

#ifdef Q_OS_UNIX
    signal(SIGUSR1, signalHandler);
#endif

    QCoreApplication app(argc, argv);

    TrkServer server;
    server.setServerName(argv[1]);
    server.setReplaySource(argv[2]);
    server.startServer();

    return app.exec();
}

#include "trkserver.moc"
