// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui/qgenericplugin.h>

#include <QtCore/qdebug.h>
#include <QtCore/qbytearray.h>

#include <cerrno>
#include <cstring>
#include <string>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

using namespace Qt::StringLiterals;

// The device refuses PTRACE_TRACEME but permits PTRACE_ATTACH inside an installed,
// debug-profile-signed application, so the server has to be started from within the
// process it debugs. Loading this through "-plugin qtcdebug[:<port>]" is what makes
// that happen without the application knowing about it.

static void reportLoadAddresses()
{
    // /proc files report a size of zero, which makes QFile-based reading come up empty.
    const int fd = ::open("/proc/self/maps", O_RDONLY);
    if (fd < 0) {
        qWarning("qtcdebug: cannot open /proc/self/maps");
        return;
    }

    QByteArray maps;
    char buffer[8192];
    for (ssize_t n = ::read(fd, buffer, sizeof(buffer)); n > 0; n = ::read(fd, buffer, sizeof(buffer)))
        maps.append(buffer, int(n));
    ::close(fd);

    // The addresses change per launch, so the host side can only learn them from here.
    QByteArray reported;
    for (const QByteArray &line : maps.split('\n')) {
        if (!line.endsWith(".so"))
            continue;
        const QByteArray library = line.mid(line.lastIndexOf('/') + 1);
        if (library == reported)
            continue;
        reported = library;
        qWarning("qtcdebug: base %s %s", line.left(line.indexOf('-')).constData(), library.constData());
    }
}

#ifndef QTC_SERVER_PATH
#define QTC_SERVER_PATH "lldb-server"
#endif

static const char *sharedServerPath = QTC_SERVER_PATH;

static void startServer(quint16 port)
{
    // Saying so is worth it: without the server there is nothing to attach to, and the
    // failure would otherwise happen in the child where nobody sees it.
    if (::access(sharedServerPath, X_OK) != 0) {
        qWarning("qtcdebug: %s is not usable: %s", sharedServerPath, std::strerror(errno));
    }

    const pid_t target = ::getpid();
    const pid_t child = ::fork();
    if (child < 0) {
        qWarning("qtcdebug: fork failed");
        return;
    }
    if (child > 0) {
        qWarning("qtcdebug: lldb-server platform %d for %d listening on %d", child,
                 ::getpid(), port);
        return;
    }

    // A platform, not a bare server: that way the debugger asks it for the process and
    // learns what is mapped where by itself, which a bare server cannot tell it here. The
    // server it spawns for the process inherits this context, where attaching is allowed.
    const std::string listen = "*:" + std::to_string(port);
    // A native package is not on PATH, so its path is tried first, the name as a fallback.
    ::execl(sharedServerPath, "lldb-server", "platform", "--listen", listen.c_str(), nullptr);
    ::execlp("lldb-server", "lldb-server", "platform", "--listen", listen.c_str(), nullptr);
    ::_exit(127);
}

class QtcDebugPlugin : public QGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QGenericPluginFactoryInterface_iid FILE "qtcdebug.json")

public:
    QObject *create(const QString &key, const QString &specification) override
    {
        if (key.compare("qtcdebug"_L1, Qt::CaseInsensitive) != 0)
            return nullptr;

        bool ok = false;
        const quint16 port = specification.toUShort(&ok);
        reportLoadAddresses();
        startServer(ok ? port : 8123);
        return nullptr;
    }
};

#include "qtcdebugplugin.moc"
