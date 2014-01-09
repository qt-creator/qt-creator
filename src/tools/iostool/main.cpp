/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include <qglobal.h>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QApplication>
#else
#include <QGuiApplication>
#endif
#include <QTextStream>
#include <QDebug>
#include <QXmlStreamWriter>
#include <QFile>
#include <QMapIterator>
#include <QScopedArrayPointer>
#include <QTcpServer>
#include <QSocketNotifier>
#include <QTcpSocket>

#include "iosdevicemanager.h"
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#ifdef Q_OS_UNIX
#include <unistd.h>
#include <fcntl.h>
#endif

static const bool debugGdbEchoServer = false;

class IosTool: public QObject {
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
private slots:
    void isTransferringApp(const QString &bundlePath, const QString &deviceId, int progress,
                           const QString &info);
    void didTransferApp(const QString &bundlePath, const QString &deviceId,
                        Ios::IosDeviceManager::OpStatus status);
    void didStartApp(const QString &bundlePath, const QString &deviceId,
                     Ios::IosDeviceManager::OpStatus status, int gdbFd);
    void deviceInfo(const QString &deviceId, const Ios::IosDeviceManager::Dict &info);
    void appOutput(const QString &output);
    void errorMsg(const QString &msg);
    void handleNewConnection();
    void handleGdbServerSocketHasData(int socket);
    void handleCreatorHasData();
    void handleCreatorHasError(QAbstractSocket::SocketError error);
private:
    bool startServer();
    void stopGdbServer(int errorCode = 0);

    int maxProgress;
    int opLeft;
    bool debug;
    bool ipv6;
    bool inAppOutput;
    bool splitAppOutput; // as QXmlStreamReader reports the text attributes atomically it is better to split
    Ios::IosDeviceManager::AppOp appOp;
    QFile outFile;
    QXmlStreamWriter out;
    int gdbFileDescriptor;
    QTcpSocket *creatorSocket;
    QSocketNotifier *gdbServerNotifier;
    QTcpServer gdbServer;
};

IosTool::IosTool(QObject *parent):
    QObject(parent),
    maxProgress(0),
    opLeft(0),
    debug(false),
    ipv6(false),
    inAppOutput(false),
    splitAppOutput(true),
    appOp(Ios::IosDeviceManager::None),
    outFile(),
    out(&outFile),
    gdbFileDescriptor(-1),
    creatorSocket(0),
    gdbServerNotifier(0)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    outFile.open(stdout, QIODevice::WriteOnly, QFile::DontCloseHandle);
#else
    outFile.open(stdout, QIODevice::WriteOnly, QFileDevice::DontCloseHandle);
#endif
    out.setAutoFormatting(true);
    out.setCodec("UTF-8");
}

IosTool::~IosTool()
{
    delete creatorSocket; // not strictly required
    delete gdbServerNotifier; // not strictly required
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
            ++iarg;
            extraArgs = args.mid(iarg, args.size() - iarg);
        } else if (arg == QLatin1String("-help") || arg == QLatin1String("--help")) {
            printHelp = true;
        } else {
            writeMsg(QString::fromLatin1("unexpected argument '%1'").arg(arg));
        }
    }
    if (printHelp) {
        out.writeStartElement(QLatin1String("msg"));
        out.writeCharacters(QLatin1String("iosTool [-device-id <deviceId>] [-bundle <pathToBundle>] [-deploy] [-run] [-debug]\n"));
        out.writeCharacters(QLatin1String("    [-device-info] [-timeout <timeoutIn_ms>]\n")); // to do pass in env as stub does
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
    connect(manager,SIGNAL(didStartApp(QString,QString,Ios::IosDeviceManager::OpStatus,int)),
            SLOT(didStartApp(QString,QString,Ios::IosDeviceManager::OpStatus,int)));
    connect(manager,SIGNAL(deviceInfo(QString,Ios::IosDeviceManager::Dict)),
            SLOT(deviceInfo(QString,Ios::IosDeviceManager::Dict)));
    connect(manager,SIGNAL(appOutput(QString)), SLOT(appOutput(QString)));
    connect(manager,SIGNAL(errorMsg(QString)), SLOT(errorMsg(QString)));
    manager->watchDevices();
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
    if (status != Ios::IosDeviceManager::Success || --opLeft == 0)
        doExit((status == Ios::IosDeviceManager::Success) ? 0 : -1);
}

void IosTool::didStartApp(const QString &bundlePath, const QString &deviceId,
                             Ios::IosDeviceManager::OpStatus status, int gdbFd)
{
    Q_UNUSED(bundlePath);
    Q_UNUSED(deviceId);
    out.writeEmptyElement(QLatin1String("app_started"));
    out.writeAttribute(QLatin1String("status"),
                       (status == Ios::IosDeviceManager::Success) ?
                           QLatin1String("SUCCESS") :
                           QLatin1String("FAILURE"));
    //out.writeCharacters(QString()); // trigger a complete closing of the empty element
    outFile.flush();
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
    if (debug) {
        gdbFileDescriptor=gdbFd;
        if (!startServer()) {
            doExit(-4);
            return;
        }
        out.writeTextElement(QLatin1String("gdb_server_port"),
                             QString::number(gdbServer.serverPort()));
        outFile.flush();
    } else {
        if (!splitAppOutput) {
            out.writeStartElement(QLatin1String("app_output"));
            inAppOutput = true;
        }
        outFile.flush();
        Ios::IosDeviceManager::instance()->processGdbServer(gdbFd);
        if (!splitAppOutput) {
            inAppOutput = false;
            out.writeEndElement();
        }
        outFile.flush();
        close(gdbFd);
        doExit();
    }
}

void IosTool::writeMsg(const char *msg)
{
    writeMsg(QString::fromLatin1(msg));
}

void IosTool::writeMsg(const QString &msg)
{
    out.writeStartElement(QLatin1String("msg"));
    writeTextInElement(msg);
    out.writeCharacters(QLatin1String("\n"));
    out.writeEndElement();
    outFile.flush();
}

void IosTool::deviceInfo(const QString &deviceId, const Ios::IosDeviceManager::Dict &devInfo)
{
    Q_UNUSED(deviceId);
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
    doExit();
}

void IosTool::writeTextInElement(const QString &output)
{
    QRegExp controlCharRe(QLatin1String("[\x01-\x08]|\x0B|\x0C|[\x0E-\x1F]|\\0000"));
    int pos = 0;
    int oldPos = 0;

    while ((pos = controlCharRe.indexIn(output, pos)) != -1) {
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
    if (!inAppOutput)
        out.writeStartElement(QLatin1String("app_output"));
    writeTextInElement(output);
    if (!inAppOutput)
        out.writeEndElement();
    outFile.flush();
}

void IosTool::errorMsg(const QString &msg)
{
    writeMsg(msg);
}

void IosTool::handleNewConnection()
{
    if (creatorSocket) {
        gdbServer.close();
        QTcpSocket *s = gdbServer.nextPendingConnection();
        delete s;
        return;
    }
    creatorSocket = gdbServer.nextPendingConnection();
    connect(creatorSocket, SIGNAL(readyRead()), SLOT(handleCreatorHasData()));
    connect(creatorSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(handleCreatorHasError(QAbstractSocket::SocketError)));
    gdbServerNotifier = new QSocketNotifier(gdbFileDescriptor, QSocketNotifier::Read, this);
    connect(gdbServerNotifier, SIGNAL(activated(int)), SLOT(handleGdbServerSocketHasData(int)));
    gdbServer.close();
}

void IosTool::handleGdbServerSocketHasData(int socket)
{
    gdbServerNotifier->setEnabled(false);
    char buf[255];
    while (true) {
        qptrdiff rRead = read(socket, &buf, sizeof(buf)-1);
        if (rRead == -1) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN) {
                gdbServerNotifier->setEnabled(true);
                return;
            }
            errorMsg(qt_error_string(errno));
            close(socket);
            stopGdbServer(-1);
            return;
        }
        if (rRead == 0) {
            stopGdbServer(0);
            return;
        }
        if (debugGdbEchoServer) {
            writeMsg("gdbServerReplies:");
            buf[rRead] = 0;
            writeMsg(buf);
        }
        qint64 pos = 0;
        while (true) {
            qint64 writtenNow = creatorSocket->write(buf + int(pos), rRead);
            if (writtenNow == -1) {
                writeMsg(creatorSocket->errorString());
                stopGdbServer(-1);
                return;
            }
            if (writtenNow < rRead) {
                pos += writtenNow;
                rRead -= qptrdiff(writtenNow);
            } else {
                break;
            }
        }
    }
}

void IosTool::stopGdbServer(int errorCode)
{
    if (debugGdbEchoServer)
        writeMsg("gdbServerStops");
    if (!creatorSocket)
        return;
    if (gdbFileDescriptor > 0) {
        ::close(gdbFileDescriptor);
        gdbFileDescriptor = -1;
        if (gdbServerNotifier)
            delete gdbServerNotifier;
    }
    if (creatorSocket->isOpen())
        creatorSocket->close();
    delete creatorSocket;
    doExit(errorCode);
}

void IosTool::handleCreatorHasData()
{
    char buf[255];
    while (true) {
        qint64 toRead = creatorSocket->bytesAvailable();
        if (qint64(sizeof(buf)-1) < toRead)
            toRead = sizeof(buf)-1;
        qint64 rRead = creatorSocket->read(buf, toRead);
        if (rRead == -1) {
            errorMsg(creatorSocket->errorString());
            stopGdbServer();
            return;
        }
        if (rRead == 0) {
            if (!creatorSocket->isOpen())
                stopGdbServer();
            return;
        }
        int pos = 0;
        int irep = 0;
        if (debugGdbEchoServer) {
            writeMsg("sendToGdbServer:");
            buf[rRead] = 0;
            writeMsg(buf);
        }
        while (true) {
            qptrdiff written = write(gdbFileDescriptor, buf + pos, rRead);
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
                errorMsg(creatorSocket->errorString());
                stopGdbServer();
                return;
            }
            if (written == 0) {
                stopGdbServer();
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

void IosTool::handleCreatorHasError(QAbstractSocket::SocketError error)
{
    errorMsg(tr("iOS Debugging connection to creator failed with error %1").arg(error));
    stopGdbServer();
}

bool IosTool::startServer()
{
    if (gdbFileDescriptor <= 0 || gdbServer.isListening() || creatorSocket != 0)
        return false;
    fcntl(gdbFileDescriptor, F_SETFL, fcntl(gdbFileDescriptor, F_GETFL, 0) | O_NONBLOCK);
    gdbServer.setMaxPendingConnections(1);
    connect(&gdbServer, SIGNAL(newConnection()), SLOT(handleNewConnection()));
    if (ipv6)
        return gdbServer.listen(QHostAddress(QHostAddress::LocalHostIPv6), 0);
    else
        return gdbServer.listen(QHostAddress(QHostAddress::LocalHost), 0);
}


int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QApplication a(argc, argv);
#else
    QGuiApplication a(argc, argv);
#endif
    IosTool tool;
    tool.run(QCoreApplication::arguments());
    int res = a.exec();
    exit(res);
}

#include "main.moc"
