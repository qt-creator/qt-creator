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
#include <utils/result.h>

#include <QSocketNotifier>

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

#ifndef Q_OS_WIN
static void closeFileDescriptor(int &fd)
{
    if (fd >= 0)
        ::close(fd);
    fd = -1;
}

// Returns the path of the slave side.
static Utils::Result<QString> openPseudoTerminal(int &masterFd, int &slaveFd)
{
    const auto fail = [&](const QString &message) -> Utils::Result<QString> {
        closeFileDescriptor(slaveFd);
        closeFileDescriptor(masterFd);
        return Utils::ResultError(message);
    };

    // Not every platform accepts O_CLOEXEC in posix_openpt(), so set it separately.
    masterFd = ::posix_openpt(O_RDWR | O_NOCTTY);
    if (masterFd < 0 || ::fcntl(masterFd, F_SETFD, FD_CLOEXEC) < 0
            || ::grantpt(masterFd) < 0 || ::unlockpt(masterFd) < 0) {
        return fail(Tr::tr("Cannot create pseudo terminal: %1")
                        .arg(QString::fromLocal8Bit(strerror(errno))));
    }
    const char *slavePath = ::ptsname(masterFd);
    if (!slavePath) {
        return fail(Tr::tr("Cannot determine the name of the pseudo terminal: %1")
                        .arg(QString::fromLocal8Bit(strerror(errno))));
    }
    const QString path = QString::fromLocal8Bit(slavePath);

    // Keep the slave side open ourselves. Otherwise the pseudo terminal is torn down
    // as soon as the debugger closes its own descriptor, discarding output that the
    // debuggee produced just before exiting.
    slaveFd = ::open(slavePath, O_RDWR | O_NOCTTY | O_CLOEXEC);
    if (slaveFd < 0) {
        return fail(Tr::tr("Cannot open pseudo terminal %1: %2")
                        .arg(path, QString::fromLocal8Bit(strerror(errno))));
    }

    // Raw mode: deliver what the debuggee wrote, with no echo and no newline translation.
    struct termios attributes;
    if (::tcgetattr(slaveFd, &attributes) == 0) {
        ::cfmakeraw(&attributes);
        ::tcsetattr(slaveFd, TCSANOW, &attributes);
    }
    ::fcntl(masterFd, F_SETFL, ::fcntl(masterFd, F_GETFL, 0) | O_NONBLOCK);
    return path;
}

static QByteArray readAvailable(int fd)
{
    if (fd < 0)
        return {};
    unsigned int nbytes = 0;
    if (::ioctl(fd, FIONREAD, (char *) &nbytes) < 0)
        return {};
    if (!nbytes) // Skip EOF notifications
        return {};
    QByteArray data(nbytes, Qt::Uninitialized);
    const ssize_t read = ::read(fd, data.data(), nbytes);
    data.resize(read > 0 ? read : 0);
    return data;
}
#endif

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
    const Utils::Result<QString> path = openPseudoTerminal(m_serverFd, m_slaveFd);
    if (!path) {
        m_errorString = path.error();
        return false;
    }
    m_serverPath = *path;
    m_serverNotifier = new QSocketNotifier(m_serverFd, QSocketNotifier::Read, this);
    connect(m_serverNotifier, &QSocketNotifier::activated, this, &OutputCollector::bytesAvailable);

    // A second pseudo terminal carries the inferior's stderr separately
    // (QTCREATORBUG-14675). If it cannot be created, stderr stays merged with stdout.
    const Utils::Result<QString> errorPath = openPseudoTerminal(m_errorFd, m_errorSlaveFd);
    if (errorPath) {
        m_errorServerPath = *errorPath;
        m_errorNotifier = new QSocketNotifier(m_errorFd, QSocketNotifier::Read, this);
        connect(m_errorNotifier, &QSocketNotifier::activated,
                this, &OutputCollector::errorBytesAvailable);
    }
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
    errorBytesAvailable();
    delete m_serverNotifier;
    m_serverNotifier = nullptr;
    delete m_errorNotifier;
    m_errorNotifier = nullptr;
    closeFileDescriptors();
    m_serverPath.clear();
    m_errorServerPath.clear();
#endif
}

#ifndef Q_OS_WIN
void OutputCollector::closeFileDescriptors()
{
    closeFileDescriptor(m_slaveFd);
    closeFileDescriptor(m_serverFd);
    closeFileDescriptor(m_errorSlaveFd);
    closeFileDescriptor(m_errorFd);
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

QString OutputCollector::errorServerName() const
{
#ifdef Q_OS_WIN
    return {};
#else
    return m_errorServerPath;
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
    const QByteArray data = readAvailable(m_serverFd);
    if (!data.isEmpty())
        emit byteDelivery(data);
#endif
}

#ifndef Q_OS_WIN
void OutputCollector::errorBytesAvailable()
{
    const QByteArray data = readAvailable(m_errorFd);
    if (!data.isEmpty())
        emit errorByteDelivery(data);
}
#endif

} // Debugger::Internal
