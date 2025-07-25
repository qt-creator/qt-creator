// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "outputcollector.h"

#include "debuggertr.h"

#ifdef Q_OS_WIN

#include <QLocalServer>
#include <QLocalSocket>
#include <QCoreApplication>

#include <stdlib.h>

#else
#include <utils/filepath.h>
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

namespace Debugger::Internal {

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
                m_errorString = Tr::tr("Cannot create temporary file: %1").arg(tf.errorString());
                m_serverPath.clear();
                return false;
            }
            m_serverPath = tf.filePath().path();
        }
        // By now the temp file was deleted again
        codedServerPath = QFile::encodeName(m_serverPath);
        if (!::mkfifo(codedServerPath.constData(), 0600))
            break;
        if (errno != EEXIST) {
            m_errorString = Tr::tr("Cannot create FiFo %1: %2").
                            arg(m_serverPath, QString::fromLocal8Bit(strerror(errno)));
            m_serverPath.clear();
            return false;
        }
    }
    if ((m_serverFd = ::open(codedServerPath.constData(), O_RDONLY|O_NONBLOCK)) < 0) {
        m_errorString = Tr::tr("Cannot open FiFo %1: %2").
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
    // Make sure any last data is read first.
    bytesAvailable();
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
    if (m_socket)
        emit byteDelivery(m_socket->readAll());
#else
    unsigned int nbytes = 0;
    if (::ioctl(m_serverFd, FIONREAD, (char *) &nbytes) < 0)
        return;
    if (!nbytes)
        return;
    QVarLengthArray<char, 8192> buff(nbytes);
    if (::read(m_serverFd, buff.data(), nbytes) != (int)nbytes)
        return;
    if (nbytes) // Skip EOF notifications
        emit byteDelivery(QByteArray::fromRawData(buff.data(), nbytes));
#endif
}

} // Debugger::Internal
