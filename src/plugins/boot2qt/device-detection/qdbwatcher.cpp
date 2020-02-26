/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qdbwatcher.h"

#include "../qdbutils.h"
#include "hostmessages.h"

#include <utils/fileutils.h>

#include <QFile>
#include <QProcess>
#include <QTimer>

namespace Qdb {
namespace Internal {

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
    constexpr void (QLocalSocket::*LocalSocketErrorFunction)(QLocalSocket::LocalSocketError)
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                = &QLocalSocket::error;
#else
                = &QLocalSocket::errorOccurred;
#endif

    m_socket = std::unique_ptr<QLocalSocket>(new QLocalSocket());
    connect(m_socket.get(), &QLocalSocket::connected,
            this, &QdbWatcher::handleWatchConnection);
    connect(m_socket.get(), LocalSocketErrorFunction,
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
        emit watcherError(tr("Unexpected QLocalSocket error: %1")
                          .arg(m_socket->errorString()));
        return;
    }

    if (m_retried) {
        stop();
        emit watcherError(tr("Could not connect to QDB host server even after trying to start it."));
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
                    tr("Invalid JSON response received from QDB server: %1");
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
        const QString message = tr("Could not find QDB host server executable. "
                                   "You can set the location with environment variable %1.")
                .arg(QLatin1String(overridingEnvironmentVariable(QdbTool::Qdb)));
        showMessage(message, true);
        return;
    }
    if (QProcess::startDetached(qdbFilePath.toString(), {"server"}))
        showMessage(tr("QDB host server started."), false);
    else
        showMessage(tr("Could not start QDB host server in %1").arg(qdbFilePath.toString()), true);
}

void QdbWatcher::retry()
{
    m_retried = true;
    {
        QMutexLocker lock(&s_startMutex);
        if (!s_startedServer) {
            showMessage(tr("Starting QDB host server."), false);
            forkHostServer();
            s_startedServer = true;
        }
    }
    QTimer::singleShot(startupDelay, this, &QdbWatcher::startPrivate);
}

} // namespace Internal
} // namespace Qdb
