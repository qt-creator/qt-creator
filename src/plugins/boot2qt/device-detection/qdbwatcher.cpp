// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbwatcher.h"

#include "hostmessages.h"
#include "../qdbtr.h"
#include "../qdbutils.h"

#include <utils/filepath.h>
#include <utils/process.h>

#include <QFile>
#include <QTimer>

namespace Qdb::Internal {

const int startupDelay = 500; // time in ms to wait for host server startup before retrying
const QString qdbSocketName = "qdb.socket";

QMutex QdbWatcher::s_startMutex;
bool QdbWatcher::s_startedServer = false;

QdbWatcher::QdbWatcher(QObject *parent)
    : QObject(parent)
    , m_requestType(RequestType::Unknown)
{
}

QdbWatcher::~QdbWatcher()
{
    stop();
}

void QdbWatcher::start(RequestType requestType)
{
    m_requestType = requestType;
    startPrivate();
}

void QdbWatcher::startPrivate()
{
    m_socket = std::unique_ptr<QLocalSocket>(new QLocalSocket());
    connect(m_socket.get(), &QLocalSocket::connected,
            this, &QdbWatcher::handleWatchConnection);
    connect(m_socket.get(), &QLocalSocket::errorOccurred,
            this, &QdbWatcher::handleWatchError);
    m_socket->connectToServer(qdbSocketName);
}

void QdbWatcher::stop()
{
    m_shuttingDown = true;
    if (m_socket)
        m_socket->disconnectFromServer();
}

void QdbWatcher::handleWatchConnection()
{
    m_retried = false;
    {
        QMutexLocker lock(&s_startMutex);
        s_startedServer = false;
    }
    connect(m_socket.get(), &QIODevice::readyRead, this, &QdbWatcher::handleWatchMessage);
    m_socket->write(createRequest(m_requestType));
}

void QdbWatcher::handleWatchError(QLocalSocket::LocalSocketError error)
{
    if (m_shuttingDown)
        return;

    if (error == QLocalSocket::PeerClosedError) {
        retry();
        return;
    }

    if (error != QLocalSocket::ServerNotFoundError
            && error != QLocalSocket::ConnectionRefusedError) {
        stop();
        emit watcherError(Tr::tr("Unexpected QLocalSocket error: %1")
                          .arg(m_socket->errorString()));
        return;
    }

    if (m_retried) {
        stop();
        emit watcherError(Tr::tr("Could not connect to QDB host server even after trying to start it."));
        return;
    }
    retry();
}

void QdbWatcher::handleWatchMessage()
{
    while (m_socket->bytesAvailable() > 0) {
        const QByteArray responseBytes = m_socket->readLine();
        const auto document = QJsonDocument::fromJson(responseBytes);
        if (document.isNull()) {
            const QString message =
                    Tr::tr("Invalid JSON response received from QDB server: %1");
            emit watcherError(message.arg(QString::fromUtf8(responseBytes)));
            return;
        }
        emit incomingMessage(document);
    }
}

void QdbWatcher::forkHostServer()
{
    Utils::FilePath qdbFilePath = findTool(QdbTool::Qdb);
    QFile executable(qdbFilePath.toString());
    if (!executable.exists()) {
        const QString message = Tr::tr("Could not find QDB host server executable. "
                                   "You can set the location with environment variable %1.")
                                    .arg(overridingEnvironmentVariable(QdbTool::Qdb));
        showMessage(message, true);
        return;
    }
    if (Utils::Process::startDetached({qdbFilePath, {"server"}}))
        showMessage(Tr::tr("QDB host server started."), false);
    else
        showMessage(Tr::tr("Could not start QDB host server in %1").arg(qdbFilePath.toString()), true);
}

void QdbWatcher::retry()
{
    m_retried = true;
    {
        QMutexLocker lock(&s_startMutex);
        if (!s_startedServer) {
            showMessage(Tr::tr("Starting QDB host server."), false);
            forkHostServer();
            s_startedServer = true;
        }
    }
    QTimer::singleShot(startupDelay, this, &QdbWatcher::startPrivate);
}

} // Qdb::Internal
