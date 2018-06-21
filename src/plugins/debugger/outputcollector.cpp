/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "outputcollector.h"

#ifdef Q_OS_WIN

#include <QLocalServer>
#include <QLocalSocket>
#include <QCoreApplication>

#include <stdlib.h>

#else
#include <utils/temporaryfile.h>

#include <QSocketNotifier>
#include <QVarLengthArray>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef Q_OS_SOLARIS
# include <sys/filio.h> // FIONREAD
#endif
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#endif

namespace Debugger {
namespace Internal {

OutputCollector::~OutputCollector()
{
    shutdown();
}

bool OutputCollector::listen()
{
#ifdef Q_OS_WIN
    if (m_server)
        return m_server->isListening();
    m_server = new QLocalServer(this);
    connect(m_server, &QLocalServer::newConnection, this, &OutputCollector::newConnectionAvailable);
    return m_server->listen(QString::fromLatin1("creator-%1-%2")
                            .arg(QCoreApplication::applicationPid())
                            .arg(rand()));
#else
    if (!m_serverPath.isEmpty())
        return true;
    QByteArray codedServerPath;
    forever {
        {
            Utils::TemporaryFile tf("outputcollector");
            if (!tf.open()) {
                m_errorString = tr("Cannot create temporary file: %1").arg(tf.errorString());
                m_serverPath.clear();
                return false;
            }
            m_serverPath = tf.fileName();
        }
        // By now the temp file was deleted again
        codedServerPath = QFile::encodeName(m_serverPath);
        if (!::mkfifo(codedServerPath.constData(), 0600))
            break;
        if (errno != EEXIST) {
            m_errorString = tr("Cannot create FiFo %1: %2").
                            arg(m_serverPath, QString::fromLocal8Bit(strerror(errno)));
            m_serverPath.clear();
            return false;
        }
    }
    if ((m_serverFd = ::open(codedServerPath.constData(), O_RDONLY|O_NONBLOCK)) < 0) {
        m_errorString = tr("Cannot open FiFo %1: %2").
                        arg(m_serverPath, QString::fromLocal8Bit(strerror(errno)));
        m_serverPath.clear();
        return false;
    }
    m_serverNotifier = new QSocketNotifier(m_serverFd, QSocketNotifier::Read, this);
    connect(m_serverNotifier, &QSocketNotifier::activated, this, &OutputCollector::bytesAvailable);
    return true;
#endif
}

void OutputCollector::shutdown()
{
#ifdef Q_OS_WIN
    delete m_server; // Deletes socket as well (QObject parent)
    m_server = nullptr;
    m_socket = nullptr;
#else
    if (!m_serverPath.isEmpty()) {
        ::close(m_serverFd);
        ::unlink(QFile::encodeName(m_serverPath).constData());
        delete m_serverNotifier;
        m_serverPath.clear();
    }
#endif
}

QString OutputCollector::errorString() const
{
#ifdef Q_OS_WIN
    return m_socket ? m_socket->errorString() : m_server->errorString();
#else
    return m_errorString;
#endif
}

QString OutputCollector::serverName() const
{
#ifdef Q_OS_WIN
    return m_server->fullServerName();
#else
    return m_serverPath;
#endif
}

#ifdef Q_OS_WIN
void OutputCollector::newConnectionAvailable()
{
    if (m_socket)
        return;
    m_socket = m_server->nextPendingConnection();
    connect(m_socket, &QIODevice::readyRead, this, &OutputCollector::bytesAvailable);
}
#endif

void OutputCollector::bytesAvailable()
{
#ifdef Q_OS_WIN
    emit byteDelivery(m_socket->readAll());
#else
    size_t nbytes = 0;
    if (::ioctl(m_serverFd, FIONREAD, (char *) &nbytes) < 0)
        return;
    QVarLengthArray<char, 8192> buff(nbytes);
    if (::read(m_serverFd, buff.data(), nbytes) != (int)nbytes)
        return;
    if (nbytes) // Skip EOF notifications
        emit byteDelivery(QByteArray::fromRawData(buff.data(), nbytes));
#endif
}

} // namespace Internal
} // namespace Debugger
