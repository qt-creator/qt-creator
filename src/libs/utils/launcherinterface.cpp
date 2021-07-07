/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "launcherinterface.h"

#include "launcherpackets.h"
#include "launchersocket.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qprocess.h>
#include <QtNetwork/qlocalserver.h>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

namespace Utils {

namespace Internal {

class LauncherProcess : public QProcess
{
public:
    LauncherProcess(QObject *parent) : QProcess(parent)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && defined(Q_OS_UNIX)
        setChildProcessModifier([this] { setupChildProcess_impl(); });
#endif
    }

private:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void setupChildProcess() override
    {
        setupChildProcess_impl();
    }
#endif

    void setupChildProcess_impl()
    {
#ifdef Q_OS_UNIX
        const auto pid = static_cast<pid_t>(processId());
        setpgid(pid, pid);
#endif
    }
};

} // namespace Internal

using namespace Utils::Internal;

static QString launcherSocketName()
{
    return QStringLiteral("qtcreator_processlauncher-%1")
            .arg(QString::number(qApp->applicationPid()));
}

LauncherInterface::LauncherInterface()
    : m_server(new QLocalServer(this)), m_socket(new LauncherSocket(this))
{
    QObject::connect(m_server, &QLocalServer::newConnection,
                     this, &LauncherInterface::handleNewConnection);
}

LauncherInterface &LauncherInterface::instance()
{
    static LauncherInterface p;
    return p;
}

LauncherInterface::~LauncherInterface()
{
    m_server->disconnect();
}

void LauncherInterface::doStart()
{
    if (++m_startRequests > 1)
        return;
    const QString &socketName = launcherSocketName();
    QLocalServer::removeServer(socketName);
    if (!m_server->listen(socketName)) {
        emit errorOccurred(m_server->errorString());
        return;
    }
    m_process = new LauncherProcess(this);
    connect(m_process, &QProcess::errorOccurred, this, &LauncherInterface::handleProcessError);
    connect(m_process,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &LauncherInterface::handleProcessFinished);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &LauncherInterface::handleProcessStderr);
    m_process->start(qApp->applicationDirPath() + QLatin1Char('/')
                            + QLatin1String(RELATIVE_LIBEXEC_PATH)
                            + QLatin1String("/qtcreator_processlauncher"),
                           QStringList(m_server->fullServerName()));
}

void LauncherInterface::doStop()
{
    if (--m_startRequests > 0)
        return;
    m_server->close();
    if (!m_process)
        return;
    m_process->disconnect();
    m_socket->shutdown();
    m_process->waitForFinished(3000);
    m_process->deleteLater();
    m_process = nullptr;
}

void LauncherInterface::handleNewConnection()
{
    QLocalSocket * const socket = m_server->nextPendingConnection();
    if (!socket)
        return;
    m_server->close();
    m_socket->setSocket(socket);
}

void LauncherInterface::handleProcessError()
{
    if (m_process->error() == QProcess::FailedToStart) {
        const QString launcherPathForUser
                = QDir::toNativeSeparators(QDir::cleanPath(m_process->program()));
        emit errorOccurred(QCoreApplication::translate("Utils::LauncherSocket",
                           "Failed to start process launcher at '%1': %2")
                           .arg(launcherPathForUser, m_process->errorString()));
    }
}

void LauncherInterface::handleProcessFinished()
{
    emit errorOccurred(QCoreApplication::translate("Utils::LauncherSocket",
                       "Process launcher closed unexpectedly: %1")
                       .arg(m_process->errorString()));
}

void LauncherInterface::handleProcessStderr()
{
    qDebug() << "[launcher]" << m_process->readAllStandardError();
}

} // namespace Utils
