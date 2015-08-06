/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "iosdevicemanager.h"

#include <qglobal.h>
#include <QGuiApplication>
#include <QTextStream>
#include <QDebug>
#include <QXmlStreamWriter>
#include <QFile>
#include <QMapIterator>
#include <QScopedArrayPointer>
#include <QTcpServer>
#include <QSocketNotifier>
#include <QTcpSocket>
#include <QTimer>
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>

#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#ifdef Q_OS_UNIX
#include <unistd.h>
#include <fcntl.h>
#endif
#include <thread>

// avoid utils dependency
#define QTC_CHECK(cond) if (cond) {} else { qWarning() << "assert failed " << #cond << " " \
    << __FILE__ << ":" << __LINE__; } do {} while (0)

static bool echoRelays = false;

class IosTool;
class RelayServer;
class GenericRelayServer;

class Relayer: public QObject
{
    Q_OBJECT
public:
    Relayer(RelayServer *parent, QTcpSocket *clientSocket);
    ~Relayer();
    void setClientSocket(QTcpSocket *clientSocket);
    bool startRelay(int serverFileDescriptor);
public slots:
    void handleSocketHasData(int socket);
    void handleClientHasData();
    void handleClientHasError(QAbstractSocket::SocketError error);
protected:
    IosTool *iosTool();
    RelayServer *server();
    Ios::ServiceSocket m_serverFileDescriptor;
    QTcpSocket *m_clientSocket;
    QSocketNotifier *m_serverNotifier;
};

class RemotePortRelayer: public Relayer
{
    Q_OBJECT
public:
    static const int reconnectMsecDelay = 500;
    static const int maxReconnectAttempts = 2*60*5; // 5 min
    RemotePortRelayer(GenericRelayServer *parent, QTcpSocket *clientSocket);
public slots:
    void tryRemoteConnect();
signals:
    void didConnect(GenericRelayServer *serv);
private:
    QTimer m_remoteConnectTimer;
};

class RelayServer: public QObject
{
    Q_OBJECT
public:
    RelayServer(IosTool *parent);
    ~RelayServer();
    bool startServer(int port, bool ipv6);
    void stopServer();
    quint16 serverPort();
    IosTool *iosTool();
public slots:
    void handleNewRelayConnection();
protected:
    virtual void newRelayConnection() = 0;

    QTcpServer m_server;
    QList<Relayer *> m_connections;
};

class SingleRelayServer: public RelayServer
{
    Q_OBJECT
public:
    SingleRelayServer(IosTool *parent, int serverFileDescriptor);
protected:
    void newRelayConnection() override;
private:
    int m_serverFileDescriptor;
};

class GenericRelayServer: public RelayServer {
    Q_OBJECT
public:
    GenericRelayServer(IosTool *parent, int remotePort,
                       Ios::DeviceSession *deviceSession);
protected:
    void newRelayConnection() override;
private:
    int m_remotePort;
    Ios::DeviceSession *m_deviceSession;
    friend class RemotePortRelayer;
};

class GdbRunner: public QObject
{
    Q_OBJECT
public:
    GdbRunner(IosTool *iosTool, int gdbFd);
    void stop(int phase);
public slots:
    void run();
signals:
    void finished();
private:
    IosTool *m_iosTool;
    int m_gdbFd;
};

class IosTool: public QObject
{
    Q_OBJECT
public:
    IosTool(QObject *parent = 0);
    virtual ~IosTool();
    void run(const QStringList &args);
    void doExit(int errorCode = 0);
    void writeMsg(const char *msg);
    void writeMsg(const QString &msg);
    void stopXml(int errorCode);
    void writeTextInElement(const QString &output);
    void stopRelayServers(int errorCode = 0);
    void writeMaybeBin(const QString &extraMsg, const char *msg, quintptr len);
public slots:
    void errorMsg(const QString &msg);
    void stopGdbRunner();
private slots:
    void stopGdbRunner2();
    void isTransferringApp(const QString &bundlePath, const QString &deviceId, int progress,
                           const QString &info);
    void didTransferApp(const QString &bundlePath, const QString &deviceId,
                        Ios::IosDeviceManager::OpStatus status);
    void didStartApp(const QString &bundlePath, const QString &deviceId,
                     Ios::IosDeviceManager::OpStatus status, int gdbFd,
                     Ios::DeviceSession *deviceSession);
    void deviceInfo(const QString &deviceId, const Ios::IosDeviceManager::Dict &info);
    void appOutput(const QString &output);
private:
    void readStdin();

    QMutex m_xmlMutex;
    int maxProgress;
    int opLeft;
    bool debug;
    bool ipv6;
    bool inAppOutput;
    bool splitAppOutput; // as QXmlStreamReader reports the text attributes atomically it is better to split
    Ios::IosDeviceManager::AppOp appOp;
    QFile outFile;
    QString m_qmlPort;
    QXmlStreamWriter out;
    SingleRelayServer *gdbServer;
    GenericRelayServer *qmlServer;
    GdbRunner *gdbRunner;
    friend class GdbRunner;
};

Relayer::Relayer(RelayServer *parent, QTcpSocket *clientSocket) :
    QObject(parent), m_serverFileDescriptor(0), m_clientSocket(0), m_serverNotifier(0)
{
    setClientSocket(clientSocket);
}

Relayer::~Relayer()
{
    if (m_serverFileDescriptor > 0) {
        ::close(m_serverFileDescriptor);
        m_serverFileDescriptor = -1;
        if (m_serverNotifier)
            delete m_serverNotifier;
    }
    if (m_clientSocket->isOpen())
        m_clientSocket->close();
    delete m_clientSocket;
}

void Relayer::setClientSocket(QTcpSocket *clientSocket)
{
    QTC_CHECK(!m_clientSocket);
    m_clientSocket = clientSocket;
    if (m_clientSocket)
        connect(m_clientSocket, SIGNAL(error(QAbstractSocket::SocketError)),
                SLOT(handleClientHasError(QAbstractSocket::SocketError)));
}

bool Relayer::startRelay(int serverFileDescriptor)
{
    QTC_CHECK(!m_serverFileDescriptor);
    m_serverFileDescriptor = serverFileDescriptor;
    if (!m_clientSocket || m_serverFileDescriptor <= 0)
        return false;
    fcntl(serverFileDescriptor,F_SETFL, fcntl(serverFileDescriptor, F_GETFL) | O_NONBLOCK);
    connect(m_clientSocket, SIGNAL(readyRead()), SLOT(handleClientHasData()));
    m_serverNotifier = new QSocketNotifier(m_serverFileDescriptor, QSocketNotifier::Read, this);
    connect(m_serverNotifier, SIGNAL(activated(int)), SLOT(handleSocketHasData(int)));
    // no way to check if an error did happen?
    if (m_clientSocket->bytesAvailable() > 0)
        handleClientHasData();
    return true;
}

void Relayer::handleSocketHasData(int socket)
{
    m_serverNotifier->setEnabled(false);
    char buf[255];
    while (true) {
        qptrdiff rRead = read(socket, &buf, sizeof(buf)-1);
        if (rRead == -1) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN) {
                m_serverNotifier->setEnabled(true);
                return;
            }
            iosTool()->errorMsg(qt_error_string(errno));
            close(socket);
            iosTool()->stopRelayServers(-1);
            return;
        }
        if (rRead == 0) {
            iosTool()->stopRelayServers(0);
            return;
        }
        if (echoRelays) {
            iosTool()->writeMaybeBin(QString::fromLatin1("%1 serverReplies:")
                                     .arg((quintptr)(void *)this), buf, rRead);
        }
        qint64 pos = 0;
        while (true) {
            qint64 writtenNow = m_clientSocket->write(buf + int(pos), rRead);
            if (writtenNow == -1) {
                iosTool()->writeMsg(m_clientSocket->errorString());
                iosTool()->stopRelayServers(-1);
                return;
            }
            if (writtenNow < rRead) {
                pos += writtenNow;
                rRead -= qptrdiff(writtenNow);
            } else {
                break;
            }
        }
        m_clientSocket->flush();
    }
}

void Relayer::handleClientHasData()
{
    char buf[255];
    while (true) {
        qint64 toRead = m_clientSocket->bytesAvailable();
        if (qint64(sizeof(buf)-1) < toRead)
            toRead = sizeof(buf)-1;
        qint64 rRead = m_clientSocket->read(buf, toRead);
        if (rRead == -1) {
            iosTool()->errorMsg(m_clientSocket->errorString());
            iosTool()->stopRelayServers();
            return;
        }
        if (rRead == 0) {
            if (!m_clientSocket->isOpen())
                iosTool()->stopRelayServers();
            return;
        }
        int pos = 0;
        int irep = 0;
        if (echoRelays) {
            iosTool()->writeMaybeBin(QString::fromLatin1("%1 clientAsks:")
                                     .arg((quintptr)(void *)this), buf, rRead);
        }
        while (true) {
            qptrdiff written = write(m_serverFileDescriptor, buf + pos, rRead);
            if (written == -1) {
                if (errno == EINTR)
                    continue;
                if (errno == EAGAIN) {
                    if (++irep > 10) {
                        sleep(1);
                        irep = 0;
                    }
                    continue;
                }
                iosTool()->errorMsg(qt_error_string(errno));
                iosTool()->stopRelayServers();
                return;
            }
            if (written == 0) {
                iosTool()->stopRelayServers();
                return;
            }
            if (written < rRead) {
                pos += written;
                rRead -= written;
            } else {
                break;
            }
        }
    }
}

void Relayer::handleClientHasError(QAbstractSocket::SocketError error)
{
    iosTool()->errorMsg(tr("iOS Debugging connection to creator failed with error %1").arg(error));
    iosTool()->stopRelayServers();
}

IosTool *Relayer::iosTool()
{
    return (server() ? server()->iosTool() : 0);
}

RelayServer *Relayer::server()
{
    return qobject_cast<RelayServer *>(parent());
}

RemotePortRelayer::RemotePortRelayer(GenericRelayServer *parent, QTcpSocket *clientSocket) :
    Relayer(parent, clientSocket)
{
    m_remoteConnectTimer.setSingleShot(true);
    m_remoteConnectTimer.setInterval(reconnectMsecDelay);
    connect(&m_remoteConnectTimer, SIGNAL(timeout()),
            SLOT(tryRemoteConnect()));
}

void RemotePortRelayer::tryRemoteConnect()
{
    iosTool()->errorMsg(QLatin1String("tryRemoteConnect"));
    if (m_serverFileDescriptor > 0)
        return;
    Ios::ServiceSocket ss;
    GenericRelayServer *grServer = qobject_cast<GenericRelayServer *>(server());
    if (!grServer)
        return;
    if (grServer->m_deviceSession->connectToPort(grServer->m_remotePort, &ss)) {
        if (ss > 0) {
            iosTool()->errorMsg(QString::fromLatin1("tryRemoteConnect *succeeded* on remote port %1")
                                .arg(grServer->m_remotePort));
            startRelay(ss);
            emit didConnect(grServer);
            return;
        }
    }
    iosTool()->errorMsg(QString::fromLatin1("tryRemoteConnect *failed* on remote port %1")
                        .arg(grServer->m_remotePort));
    m_remoteConnectTimer.start();
}

RelayServer::RelayServer(IosTool *parent):
    QObject(parent)
{ }

RelayServer::~RelayServer()
{
    stopServer();
}

bool RelayServer::startServer(int port, bool ipv6)
{
    QTC_CHECK(!m_server.isListening());
    m_server.setMaxPendingConnections(1);
    connect(&m_server, SIGNAL(newConnection()), SLOT(handleNewRelayConnection()));
    quint16 portValue = static_cast<quint16>(port);
    if (port < 0 || port > 0xFFFF)
        return false;
    if (ipv6)
        return m_server.listen(QHostAddress(QHostAddress::LocalHostIPv6), portValue);
    else
        return m_server.listen(QHostAddress(QHostAddress::LocalHost), portValue);
}

void RelayServer::stopServer()
{
    foreach (Relayer *connection, m_connections)
        delete connection;
    if (m_server.isListening())
        m_server.close();
}

quint16 RelayServer::serverPort()
{
    return m_server.serverPort();
}

IosTool *RelayServer::iosTool()
{
    return qobject_cast<IosTool *>(parent());
}

void RelayServer::handleNewRelayConnection()
{
    iosTool()->errorMsg(QLatin1String("handleNewRelayConnection"));
    newRelayConnection();
}

SingleRelayServer::SingleRelayServer(IosTool *parent,
                                     int serverFileDescriptor) :
    RelayServer(parent)
{
    m_serverFileDescriptor = serverFileDescriptor;
    if (m_serverFileDescriptor > 0)
        fcntl(m_serverFileDescriptor, F_SETFL, fcntl(m_serverFileDescriptor, F_GETFL, 0) | O_NONBLOCK);
}

void SingleRelayServer::newRelayConnection()
{
    if (m_connections.size() > 0) {
        m_server.close();
        QTcpSocket *s = m_server.nextPendingConnection();
        delete s;
        return;
    }
    QTcpSocket *clientSocket = m_server.nextPendingConnection();
    if (clientSocket) {
        Relayer *newConnection = new Relayer(this, clientSocket);
        m_connections.append(newConnection);
        newConnection->startRelay(m_serverFileDescriptor);
    }
    m_server.close();
}

GenericRelayServer::GenericRelayServer(IosTool *parent, int remotePort,
                                       Ios::DeviceSession *deviceSession) :
    RelayServer(parent),
    m_remotePort(remotePort),
    m_deviceSession(deviceSession)
{
    parent->errorMsg(QLatin1String("created qml server"));
}


void GenericRelayServer::newRelayConnection()
{
    QTcpSocket *clientSocket = m_server.nextPendingConnection();
    if (clientSocket) {
        iosTool()->errorMsg(QString::fromLatin1("setting up relayer for new connection"));
        RemotePortRelayer *newConnection = new RemotePortRelayer(this, clientSocket);
        m_connections.append(newConnection);
        newConnection->tryRemoteConnect();
    }
}

IosTool::IosTool(QObject *parent):
    QObject(parent),
    m_xmlMutex(QMutex::Recursive),
    maxProgress(0),
    opLeft(0),
    debug(false),
    ipv6(false),
    inAppOutput(false),
    splitAppOutput(true),
    appOp(Ios::IosDeviceManager::None),
    outFile(),
    out(&outFile),
    gdbServer(0),
    qmlServer(0)
{
    outFile.open(stdout, QIODevice::WriteOnly, QFileDevice::DontCloseHandle);
    out.setAutoFormatting(true);
    out.setCodec("UTF-8");
}

IosTool::~IosTool()
{
}

void IosTool::run(const QStringList &args)
{
    Ios::IosDeviceManager *manager = Ios::IosDeviceManager::instance();
    QString deviceId;
    QString bundlePath;
    bool deviceInfo = false;
    bool printHelp = false;
    int timeout = 1000;
    QStringList extraArgs;

    out.writeStartDocument();
    out.writeStartElement(QLatin1String("query_result"));
    for (int iarg = 1; iarg < args.size(); ++iarg) {
        const QString &arg = args[iarg];
        if (arg == QLatin1String("-device-id")) {
            if (++iarg == args.size()) {
                writeMsg("missing device id value after -device-id");
                printHelp = true;
            }
            deviceId = args.value(iarg);
        } else if (arg == QLatin1String("-bundle")) {
            if (++iarg == args.size()) {
                writeMsg("missing bundle path after -bundle");
                printHelp = true;
            }
            bundlePath = args.value(iarg);
        } else if (arg == QLatin1String("-deploy")) {
            appOp = Ios::IosDeviceManager::AppOp(appOp | Ios::IosDeviceManager::Install);
        } else if (arg == QLatin1String("-run")) {
            appOp = Ios::IosDeviceManager::AppOp(appOp | Ios::IosDeviceManager::Run);
        } else if (arg == QLatin1String("-ipv6")) {
            ipv6 = true;
        } else if (arg == QLatin1String("-verbose")) {
            echoRelays = true;
        } else if (arg == QLatin1String("-debug")) {
            appOp = Ios::IosDeviceManager::AppOp(appOp | Ios::IosDeviceManager::Run);
            debug = true;
        } else if (arg == QLatin1String("-device-info")) {
            deviceInfo = true;
        } else if (arg == QLatin1String("-timeout")) {
            if (++iarg == args.size()) {
                writeMsg("missing timeout value after -timeout");
                printHelp = true;
            }
            bool ok = false;
            int tOut = args.value(iarg).toInt(&ok);
            if (ok && tOut > 0) {
                timeout = tOut;
            } else {
                writeMsg("timeout value should be an integer");
                printHelp = true;
            }
        } else if (arg == QLatin1String("-extra-args")) {
            extraArgs = args.mid(iarg + 1, args.size() - iarg - 1);
            iarg = args.size();
        } else if (arg == QLatin1String("-help") || arg == QLatin1String("--help")) {
            printHelp = true;
        } else {
            writeMsg(QString::fromLatin1("unexpected argument \"%1\"").arg(arg));
        }
    }
    if (printHelp) {
        out.writeStartElement(QLatin1String("msg"));
        out.writeCharacters(QLatin1String("iosTool [-device-id <deviceId>] [-bundle <pathToBundle>] [-deploy] [-run] [-debug]\n"));
        out.writeCharacters(QLatin1String("    [-device-info] [-timeout <timeoutIn_ms>] [-verbose]\n")); // to do pass in env as stub does
        out.writeCharacters(QLatin1String("    [-extra-args <arguments for the target app>]"));
        out.writeEndElement();
        doExit(-1);
        return;
    }
    outFile.flush();
    connect(manager,SIGNAL(isTransferringApp(QString,QString,int,QString)),
            SLOT(isTransferringApp(QString,QString,int,QString)));
    connect(manager,SIGNAL(didTransferApp(QString,QString,Ios::IosDeviceManager::OpStatus)),
            SLOT(didTransferApp(QString,QString,Ios::IosDeviceManager::OpStatus)));
    connect(manager,SIGNAL(didStartApp(QString,QString,Ios::IosDeviceManager::OpStatus,int,Ios::DeviceSession*)),
            SLOT(didStartApp(QString,QString,Ios::IosDeviceManager::OpStatus,int,Ios::DeviceSession*)));
    connect(manager,SIGNAL(deviceInfo(QString,Ios::IosDeviceManager::Dict)),
            SLOT(deviceInfo(QString,Ios::IosDeviceManager::Dict)));
    connect(manager,SIGNAL(appOutput(QString)), SLOT(appOutput(QString)));
    connect(manager,SIGNAL(errorMsg(QString)), SLOT(errorMsg(QString)));
    manager->watchDevices();
    QRegExp qmlPortRe=QRegExp(QLatin1String("-qmljsdebugger=port:([0-9]+)"));
    foreach (const QString &arg, extraArgs) {
        if (qmlPortRe.indexIn(arg) == 0) {
            bool ok;
            int qmlPort = qmlPortRe.cap(1).toInt(&ok);
            if (ok && qmlPort > 0 && qmlPort <= 0xFFFF)
                m_qmlPort = qmlPortRe.cap(1);
            break;
        }
    }
    if (deviceInfo) {
        if (!bundlePath.isEmpty())
            writeMsg("-device-info overrides bundlePath");
        ++opLeft;
        manager->requestDeviceInfo(deviceId, timeout);
    } else if (!bundlePath.isEmpty()) {
        switch (appOp) {
        case Ios::IosDeviceManager::None:
            break;
        case Ios::IosDeviceManager::Install:
        case Ios::IosDeviceManager::Run:
            ++opLeft;
            break;
        case Ios::IosDeviceManager::InstallAndRun:
            opLeft += 2;
            break;
        }
        maxProgress = 200;
        manager->requestAppOp(bundlePath, extraArgs, appOp, deviceId, timeout);
    }
    if (opLeft == 0)
        doExit(0);
}

void IosTool::stopXml(int errorCode)
{
    QMutexLocker l(&m_xmlMutex);
    out.writeEmptyElement(QLatin1String("exit"));
    out.writeAttribute(QLatin1String("code"), QString::number(errorCode));
    out.writeEndElement(); // result element (hopefully)
    out.writeEndDocument();
    outFile.flush();
}

void IosTool::doExit(int errorCode)
{
    stopXml(errorCode);
    QCoreApplication::exit(errorCode); // sometime does not really exit
    exit(errorCode);
}

void IosTool::isTransferringApp(const QString &bundlePath, const QString &deviceId, int progress,
                                const QString &info)
{
    Q_UNUSED(bundlePath);
    Q_UNUSED(deviceId);
    QMutexLocker l(&m_xmlMutex);
    out.writeStartElement(QLatin1String("status"));
    out.writeAttribute(QLatin1String("progress"), QString::number(progress));
    out.writeAttribute(QLatin1String("max_progress"), QString::number(maxProgress));
    out.writeCharacters(info);
    out.writeEndElement();
    outFile.flush();
}

void IosTool::didTransferApp(const QString &bundlePath, const QString &deviceId,
                             Ios::IosDeviceManager::OpStatus status)
{
    Q_UNUSED(bundlePath);
    Q_UNUSED(deviceId);
    {
        QMutexLocker l(&m_xmlMutex);
        if (status == Ios::IosDeviceManager::Success) {
            out.writeStartElement(QLatin1String("status"));
            out.writeAttribute(QLatin1String("progress"), QString::number(maxProgress));
            out.writeAttribute(QLatin1String("max_progress"), QString::number(maxProgress));
            out.writeCharacters(QLatin1String("App Transferred"));
            out.writeEndElement();
        }
        out.writeEmptyElement(QLatin1String("app_transfer"));
        out.writeAttribute(QLatin1String("status"),
                           (status == Ios::IosDeviceManager::Success) ?
                               QLatin1String("SUCCESS") :
                               QLatin1String("FAILURE"));
        //out.writeCharacters(QString()); // trigger a complete closing of the empty element
        outFile.flush();
    }
    if (status != Ios::IosDeviceManager::Success || --opLeft == 0)
        doExit((status == Ios::IosDeviceManager::Success) ? 0 : -1);
}

void IosTool::didStartApp(const QString &bundlePath, const QString &deviceId,
                          Ios::IosDeviceManager::OpStatus status, int gdbFd,
                          Ios::DeviceSession *deviceSession)
{
    Q_UNUSED(bundlePath);
    Q_UNUSED(deviceId);
    {
        QMutexLocker l(&m_xmlMutex);
        out.writeEmptyElement(QLatin1String("app_started"));
        out.writeAttribute(QLatin1String("status"),
                           (status == Ios::IosDeviceManager::Success) ?
                               QLatin1String("SUCCESS") :
                               QLatin1String("FAILURE"));
        //out.writeCharacters(QString()); // trigger a complete closing of the empty element
        outFile.flush();
    }
    if (status != Ios::IosDeviceManager::Success || appOp == Ios::IosDeviceManager::Install) {
        doExit();
        return;
    }
    if (gdbFd <= 0) {
        writeMsg("no gdb connection");
        doExit(-2);
        return;
    }
    if (appOp != Ios::IosDeviceManager::InstallAndRun && appOp != Ios::IosDeviceManager::Run) {
        writeMsg(QString::fromLatin1("unexpected appOp value %1").arg(appOp));
        doExit(-3);
        return;
    }
    if (deviceSession) {
        int qmlPort = deviceSession->qmljsDebugPort();
        if (qmlPort) {
            qmlServer = new GenericRelayServer(this, qmlPort, deviceSession);
            qmlServer->startServer(0, ipv6);
        }
    }
    if (debug) {
        gdbServer = new SingleRelayServer(this, gdbFd);
        if (!gdbServer->startServer(0, ipv6)) {
            doExit(-4);
            return;
        }
    }
    {
        QMutexLocker l(&m_xmlMutex);
        out.writeStartElement(QLatin1String("server_ports"));
        out.writeAttribute(QLatin1String("gdb_server"),
                           QString::number(gdbServer ? gdbServer->serverPort() : 0));
        out.writeAttribute(QLatin1String("qml_server"),
                           QString::number(qmlServer ? qmlServer->serverPort() : 0));
        out.writeEndElement();
        outFile.flush();
    }
    if (!debug) {
        gdbRunner = new GdbRunner(this, gdbFd);
        // we should not stop the event handling of the main thread
        // all output moves to the new thread (other option would be to signal it back)
        QThread *gdbProcessThread = new QThread();
        gdbRunner->moveToThread(gdbProcessThread);
        QObject::connect(gdbProcessThread, SIGNAL(started()), gdbRunner, SLOT(run()));
        QObject::connect(gdbRunner, SIGNAL(finished()), gdbProcessThread, SLOT(quit()));
        QObject::connect(gdbProcessThread, SIGNAL(finished()), gdbProcessThread, SLOT(deleteLater()));
        gdbProcessThread->start();

        new std::thread([this]() -> void { readStdin();});
    }
}

void IosTool::writeMsg(const char *msg)
{
    writeMsg(QString::fromLatin1(msg));
}

void IosTool::writeMsg(const QString &msg)
{
    QMutexLocker l(&m_xmlMutex);
    out.writeStartElement(QLatin1String("msg"));
    writeTextInElement(msg);
    out.writeCharacters(QLatin1String("\n"));
    out.writeEndElement();
    outFile.flush();
}

void IosTool::writeMaybeBin(const QString &extraMsg, const char *msg, quintptr len)
{
    char *buf2 = new char[len * 2 + 4];
    buf2[0] = '[';
    const char toHex[] = "0123456789abcdef";
    for (quintptr i = 0; i < len; ++i) {
        buf2[2 * i + 1] = toHex[(0xF & (msg[i] >> 4))];
        buf2[2 * i + 2] = toHex[(0xF & msg[i])];
    }
    buf2[2 * len + 1] = ']';
    buf2[2 * len + 2] = ' ';
    buf2[2 * len + 3] = 0;

    QMutexLocker l(&m_xmlMutex);
    out.writeStartElement(QLatin1String("msg"));
    out.writeCharacters(extraMsg);
    out.writeCharacters(QLatin1String(buf2));
    for (quintptr i = 0; i < len; ++i) {
        if (msg[i] < 0x20 || msg[i] > 0x7f)
            buf2[i] = '_';
        else
            buf2[i] = msg[i];
    }
    buf2[len] = 0;
    out.writeCharacters(QLatin1String(buf2));
    delete[] buf2;
    out.writeEndElement();
    outFile.flush();
}

void IosTool::deviceInfo(const QString &deviceId, const Ios::IosDeviceManager::Dict &devInfo)
{
    Q_UNUSED(deviceId);
    {
        QMutexLocker l(&m_xmlMutex);
        out.writeTextElement(QLatin1String("device_id"), deviceId);
        out.writeStartElement(QLatin1String("device_info"));
        QMapIterator<QString,QString> i(devInfo);
        while (i.hasNext()) {
            i.next();
            out.writeStartElement(QLatin1String("item"));
            out.writeTextElement(QLatin1String("key"), i.key());
            out.writeTextElement(QLatin1String("value"), i.value());
            out.writeEndElement();
        }
        out.writeEndElement();
        outFile.flush();
    }
    doExit();
}

void IosTool::writeTextInElement(const QString &output)
{
    QRegExp controlCharRe(QLatin1String("[\x01-\x08]|\x0B|\x0C|[\x0E-\x1F]|\\0000"));
    int pos = 0;
    int oldPos = 0;

    while ((pos = controlCharRe.indexIn(output, pos)) != -1) {
        QMutexLocker l(&m_xmlMutex);
        out.writeCharacters(output.mid(oldPos, pos - oldPos));
        out.writeEmptyElement(QLatin1String("control_char"));
        out.writeAttribute(QLatin1String("code"), QString::number(output.at(pos).toLatin1()));
        pos += 1;
        oldPos = pos;
    }
    out.writeCharacters(output.mid(oldPos, output.length() - oldPos));
}

void IosTool::appOutput(const QString &output)
{
    QMutexLocker l(&m_xmlMutex);
    if (!inAppOutput)
        out.writeStartElement(QLatin1String("app_output"));
    writeTextInElement(output);
    if (!inAppOutput)
        out.writeEndElement();
    outFile.flush();
}

void IosTool::readStdin()
{
    int c = getchar();
    if (c == 'k') {
        QMetaObject::invokeMethod(this, "stopGdbRunner");
        errorMsg(QLatin1String("iostool: Killing inferior.\n"));
    } else if (c != EOF) {
        errorMsg(QLatin1String("iostool: Unexpected character in stdin, stop listening.\n"));
    }
}

void IosTool::errorMsg(const QString &msg)
{
    writeMsg(msg);
}

void IosTool::stopGdbRunner()
{
    if (gdbRunner) {
        gdbRunner->stop(0);
        QTimer::singleShot(100, this, SLOT(stopGdbRunner2()));
    }
}

void IosTool::stopGdbRunner2()
{
    if (gdbRunner)
        gdbRunner->stop(1);
}

void IosTool::stopRelayServers(int errorCode)
{
    if (echoRelays)
        writeMsg("gdbServerStops");
    if (qmlServer)
        qmlServer->stopServer();
    if (gdbServer)
        gdbServer->stopServer();
    doExit(errorCode);
}

int main(int argc, char *argv[])
{
    //This keeps iostool from stealing focus
    qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "true");
    // We do not pass the real arguments to QCoreApplication because this wrapper needs to be able
    // to forward arguments like -qmljsdebugger=... that are filtered by QCoreApplication
    QStringList args;
    for (int iarg = 0; iarg < argc ; ++iarg)
        args << QString::fromLocal8Bit(argv[iarg]);
    char *qtArg = 0;
    int qtArgc = 0;
    if (argc > 0) {
        qtArg = argv[0];
        qtArgc = 1;
    }

    QGuiApplication a(qtArgc, &qtArg);
    IosTool tool;
    tool.run(args);
    int res = a.exec();
    exit(res);
}

GdbRunner::GdbRunner(IosTool *iosTool, int gdbFd) :
    QObject(0), m_iosTool(iosTool), m_gdbFd(gdbFd)
{
}

void GdbRunner::run()
{
    {
        QMutexLocker l(&m_iosTool->m_xmlMutex);
        if (!m_iosTool->splitAppOutput) {
            m_iosTool->out.writeStartElement(QLatin1String("app_output"));
            m_iosTool->inAppOutput = true;
        }
        m_iosTool->outFile.flush();
    }
    Ios::IosDeviceManager::instance()->processGdbServer(m_gdbFd);
    {
        QMutexLocker l(&m_iosTool->m_xmlMutex);
        if (!m_iosTool->splitAppOutput) {
            m_iosTool->inAppOutput = false;
            m_iosTool->out.writeEndElement();
        }
        m_iosTool->outFile.flush();
    }
    close(m_gdbFd);
    m_iosTool->doExit();
    emit finished();
}

void GdbRunner::stop(int phase)
{
    Ios::IosDeviceManager::instance()->stopGdbServer(m_gdbFd, phase);
}

#include "main.moc"
