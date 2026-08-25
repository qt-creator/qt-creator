// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtlocalpeer.h"

#include <QCoreApplication>
#include <QDataStream>
#include <QElapsedTimer>
#include <QTime>

#include <memory>

#if defined(Q_OS_WIN)
#include <QLibrary>
#include <qt_windows.h>
typedef BOOL(WINAPI*PProcessIdToSessionId)(DWORD,DWORD*);
static PProcessIdToSessionId pProcessIdToSessionId = 0;
#endif

#if defined(Q_OS_UNIX)
#include <time.h>
#include <unistd.h>
#endif

namespace SharedTools {

static const char ack[] = "ack";

QString QtLocalPeer::appSessionId(const QString &appId)
{
    const QByteArray idc = appId.toUtf8();
    const quint16 idNum = qChecksum(idc);

    //### could do: two 16bit checksums over separate halves of id, for a 32bit result - improved uniqeness probability. Every-other-char split would be best.

    QString res = QLatin1String("qtsingleapplication-")
                 + QString::number(idNum, 16);
#if defined(Q_OS_WIN)
    if (!pProcessIdToSessionId) {
        QLibrary lib(QLatin1String("kernel32"));
        pProcessIdToSessionId = (PProcessIdToSessionId)lib.resolve("ProcessIdToSessionId");
    }
    if (pProcessIdToSessionId) {
        DWORD sessionId = 0;
        pProcessIdToSessionId(GetCurrentProcessId(), &sessionId);
        res += QLatin1Char('-') + QString::number(sessionId, 16);
    }
#else
    res += QLatin1Char('-') + QString::number(::getuid(), 16);
#endif
    return res;
}

QtLocalPeer::QtLocalPeer(QObject *parent, const QString &appId)
    : QObject(parent), id(appId)
{
    if (id.isEmpty())
        id = QCoreApplication::applicationFilePath();  //### On win, check if this returns .../argv[0] without casefolding; .\MYAPP == .\myapp on Win

    socketName = appSessionId(id);
    server = new QLocalServer(this);
    QString lockName = QDir(QDir::tempPath()).absolutePath()
                       + QLatin1Char('/') + socketName
                       + QLatin1String("-lockfile");
    lockFile.reset(new QLockFile(lockName));
    lockFile->setStaleLockTime(0);
}

bool QtLocalPeer::isClient()
{
    if (lockFile->isLocked())
        return false;

    if (!lockFile->tryLock())
        return true;

    if (!QLocalServer::removeServer(socketName))
        qWarning("QtSingleCoreApplication: could not cleanup socket");
    bool res = server->listen(socketName);
    if (!res)
        qWarning("QtSingleCoreApplication: listen on local socket failed, %s", qPrintable(server->errorString()));
    QObject::connect(server, &QLocalServer::newConnection, this, &QtLocalPeer::receiveConnection);
    return false;
}

bool QtLocalPeer::sendMessage(const QString &message, int timeout, bool block)
{
    if (!isClient())
        return false;

    QLocalSocket socket;
    bool connOk = false;
    for (int i = 0; i < 2; i++) {
        // Try twice, in case the other instance is just starting up
        socket.connectToServer(socketName);
        connOk = socket.waitForConnected(timeout/2);
        if (connOk || i)
            break;
        int ms = 250;
#if defined(Q_OS_WIN)
        Sleep(DWORD(ms));
#else
        struct timespec ts = {ms / 1000, (ms % 1000) * 1000 * 1000};
        nanosleep(&ts, nullptr);
#endif
    }
    if (!connOk)
        return false;

    QByteArray uMsg(message.toUtf8());
    QDataStream ds(&socket);
    ds.writeBytes(uMsg.constData(), uMsg.size());
    bool res = socket.waitForBytesWritten(timeout);
    res &= socket.waitForReadyRead(timeout); // wait for ack
    res &= (socket.read(qstrlen(ack)) == ack);
    if (res && block) // block until peer disconnects
        socket.waitForDisconnected(-1);
    return res;
}

// Messages are small IPC payloads (command-line args, file paths); reject
// anything grossly larger to avoid a hostile or broken peer causing an OOM.
static const quint32 maxMessageSize = 64 * 1024 * 1024; // 64 MB
static const qint64 receiveTimeoutMs = 5000;            // total time budget for reading one message

void QtLocalPeer::receiveConnection()
{
    std::unique_ptr<QLocalSocket> socket(server->nextPendingConnection());
    if (!socket)
        return;

    QElapsedTimer timer;
    timer.start();
    const auto remainingTime = [&timer]() -> qint64 {
        return qMax(0, receiveTimeoutMs - timer.elapsed());
    };

    // Why doesn't Qt have a blocking stream that takes care of this shait???
    while (socket->bytesAvailable() < static_cast<int>(sizeof(quint32))) {
        if (!socket->isValid()) // stale request
            return;
        if (timer.hasExpired(receiveTimeoutMs)) {
            qWarning() << "QtLocalPeer: Timed out waiting for message header";
            return;
        }
        socket->waitForReadyRead(remainingTime());
    }
    QDataStream ds(socket.get());
    QByteArray uMsg;
    quint32 remaining;
    ds >> remaining;
    if (remaining > maxMessageSize) {
        qWarning() << "QtLocalPeer: Rejecting oversized message of" << remaining << "bytes";
        return;
    }
    uMsg.resize(remaining);
    int got = 0;
    char* uMsgBuf = uMsg.data();
    //qDebug() << "RCV: remaining" << remaining;
    do {
        got = ds.readRawData(uMsgBuf, remaining);
        remaining -= got;
        uMsgBuf += got;
        //qDebug() << "RCV: got" << got << "remaining" << remaining;
        if (remaining == 0 || got < 0)
            break;
        if (timer.hasExpired(receiveTimeoutMs)) {
            qWarning() << "QtLocalPeer: Timed out receiving message";
            return;
        }
    } while (socket->waitForReadyRead(remainingTime()));
    if (got < 0) {
        qWarning() << "QtLocalPeer: Message reception failed" << socket->errorString();
        return;
    }
    if (remaining != 0) {
        qWarning() << "QtLocalPeer: Incomplete message," << remaining << "bytes missing";
        return;
    }
    QString message = QString::fromUtf8(uMsg.constData(), uMsg.size());
    socket->write(ack, qstrlen(ack));
    socket->waitForBytesWritten(1000);
    // might take a long time to return
    emit messageReceived(message, socket.release());
}

} // namespace SharedTools
