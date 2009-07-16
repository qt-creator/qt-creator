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


class TrkServer : public QObject
{
    Q_OBJECT

public:
    TrkServer(const QString &serverName);
    ~TrkServer();

private slots:
    void handleConnection();

private:
    void logMessage(const QString &msg);

    QList<QByteArray> m_data;
    QLocalServer *m_server;
    int m_lastSent;
};

TrkServer::TrkServer(const QString &serverName)
{
    QFile file("dump.pro");
    m_data = file.readAll().split('\n');
    logMessage(QString("Read %1 lines of data").arg(m_data.size()));
    
    m_server = new QLocalServer(this);
    m_lastSent = 0;
    if (!m_server->listen(serverName)) {
        logMessage(QString("Error: Unable to start the TRK server %1: %2.")
                    .arg(serverName).arg(m_server->errorString()));
        return;
    }

    logMessage("The TRK server is running. Run the adapter now.");
    connect(m_server, SIGNAL(newConnection()), this, SLOT(handleConnection()));
}

TrkServer::~TrkServer()
{
    logMessage("Shutting down");
    m_server->close();
}

void TrkServer::logMessage(const QString &msg)
{
    qDebug() << "TRKSERV: " << qPrintable(msg);
}

void TrkServer::handleConnection()
{
    QByteArray block;

    QByteArray msg = m_data[m_lastSent ++];

    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << (quint16)0;
    out << m_data;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));

    QLocalSocket *clientConnection = m_server->nextPendingConnection();
    connect(clientConnection, SIGNAL(disconnected()),
            clientConnection, SLOT(deleteLater()));

    clientConnection->write(block);
    clientConnection->flush();
    //clientConnection->disconnectFromHost();
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        qDebug() << "Usage: " << argv[0] << " <trkservername>";
        return 1;
    }

#ifdef Q_OS_UNIX
    signal(SIGUSR1, signalHandler);
#endif

    QCoreApplication app(argc, argv);
    QString serverName = argv[1];
    TrkServer server(serverName);
    return app.exec();
}

#include "trkserver.moc"
