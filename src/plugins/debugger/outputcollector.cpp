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
#include <QSocketNotifier>
#include <QVarLengthArray>

#include <sys/ioctl.h>
#ifdef Q_OS_SOLARIS
# include <sys/filio.h> // FIONREAD
#endif
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
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

    // The debugger points all three of the debuggee's standard descriptors at this
    // device, so it needs two independent directions, which a pseudo terminal has.
    // Being a terminal also keeps the debuggee's stdout line buffered, so output
    // shows up while it still runs.
    // Not every platform accepts O_CLOEXEC in posix_openpt(), so set it separately.
    m_serverFd = ::posix_openpt(O_RDWR | O_NOCTTY);
    if (m_serverFd < 0 || ::fcntl(m_serverFd, F_SETFD, FD_CLOEXEC) < 0
            || ::grantpt(m_serverFd) < 0 || ::unlockpt(m_serverFd) < 0) {
        m_errorString = Tr::tr("Cannot create pseudo terminal: %1")
                            .arg(QString::fromLocal8Bit(strerror(errno)));
        closeFileDescriptors();
        return false;
    }
    const char *slavePath = ::ptsname(m_serverFd);
    if (!slavePath) {
        m_errorString = Tr::tr("Cannot determine the name of the pseudo terminal: %1")
                            .arg(QString::fromLocal8Bit(strerror(errno)));
        closeFileDescriptors();
        return false;
    }

    // Keep the slave side open ourselves. Otherwise the pseudo terminal is torn down
    // as soon as the debugger closes its own descriptor, discarding output that the
    // debuggee produced just before exiting.
    m_slaveFd = ::open(slavePath, O_RDWR | O_NOCTTY | O_CLOEXEC);
    if (m_slaveFd < 0) {
        m_errorString = Tr::tr("Cannot open pseudo terminal %1: %2")
                            .arg(QString::fromLocal8Bit(slavePath),
                                 QString::fromLocal8Bit(strerror(errno)));
        closeFileDescriptors();
        return false;
    }

    // Raw mode: deliver what the debuggee wrote, with no echo and no newline translation.
    struct termios attributes;
    if (::tcgetattr(m_slaveFd, &attributes) == 0) {
        ::cfmakeraw(&attributes);
        ::tcsetattr(m_slaveFd, TCSANOW, &attributes);
    }
    ::fcntl(m_serverFd, F_SETFL, ::fcntl(m_serverFd, F_GETFL, 0) | O_NONBLOCK);

    m_serverPath = QString::fromLocal8Bit(slavePath);
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
    delete m_serverNotifier;
    m_serverNotifier = nullptr;
    closeFileDescriptors();
    m_serverPath.clear();
#endif
}

#ifndef Q_OS_WIN
void OutputCollector::closeFileDescriptors()
{
    if (m_slaveFd >= 0)
        ::close(m_slaveFd);
    if (m_serverFd >= 0)
        ::close(m_serverFd);
    m_slaveFd = -1;
    m_serverFd = -1;
}
#endif

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
    if (m_serverFd < 0)
        return;
    unsigned int nbytes = 0;
    if (::ioctl(m_serverFd, FIONREAD, (char *) &nbytes) < 0)
        return;
    if (!nbytes) // Skip EOF notifications
        return;
    QVarLengthArray<char, 8192> buff(nbytes);
    const int read = ::read(m_serverFd, buff.data(), nbytes);
    if (read > 0)
        emit byteDelivery(QByteArray::fromRawData(buff.data(), read));
#endif
}

} // Debugger::Internal
