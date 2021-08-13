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

static QString launcherSocketName()
{
    return QStringLiteral("qtcreator_processlauncher-%1")
            .arg(QString::number(qApp->applicationPid()));
}

class LauncherInterfacePrivate : public QObject
{
    Q_OBJECT
public:
    LauncherInterfacePrivate();
    ~LauncherInterfacePrivate() override;

    void doStart();
    void doStop();
    void handleNewConnection();
    void handleProcessError();
    void handleProcessFinished();
    void handleProcessStderr();
    Internal::LauncherSocket *socket() const { return m_socket; }

    void setPathToLauncher(const QString &path) { if (!path.isEmpty()) m_pathToLauncher = path; }
signals:
    void errorOccurred(const QString &error);

private:
    QLocalServer * const m_server;
    Internal::LauncherSocket *const m_socket;
    Internal::LauncherProcess *m_process = nullptr;
    QString m_pathToLauncher;
    int m_startRequests = 0;
};

LauncherInterfacePrivate::LauncherInterfacePrivate()
    : m_server(new QLocalServer(this)), m_socket(new LauncherSocket(this))
{
    m_pathToLauncher = qApp->applicationDirPath() + '/' + QLatin1String(RELATIVE_LIBEXEC_PATH);
    QObject::connect(m_server, &QLocalServer::newConnection,
                     this, &LauncherInterfacePrivate::handleNewConnection);
}

LauncherInterfacePrivate::~LauncherInterfacePrivate()
{
    m_server->disconnect();
}

void LauncherInterfacePrivate::doStart()
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
    connect(m_process, &QProcess::errorOccurred, this, &LauncherInterfacePrivate::handleProcessError);
    connect(m_process,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &LauncherInterfacePrivate::handleProcessFinished);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &LauncherInterfacePrivate::handleProcessStderr);
    const QString launcherPath = m_pathToLauncher + QLatin1String("/qtcreator_processlauncher");
    m_process->start(launcherPath, QStringList(m_server->fullServerName()));
}

void LauncherInterfacePrivate::doStop()
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

void LauncherInterfacePrivate::handleNewConnection()
{
    QLocalSocket * const socket = m_server->nextPendingConnection();
    if (!socket)
        return;
    m_server->close();
    m_socket->setSocket(socket);
}

void LauncherInterfacePrivate::handleProcessError()
{
    if (m_process->error() == QProcess::FailedToStart) {
        const QString launcherPathForUser
                = QDir::toNativeSeparators(QDir::cleanPath(m_process->program()));
        emit errorOccurred(QCoreApplication::translate("Utils::LauncherSocket",
                           "Failed to start process launcher at '%1': %2")
                           .arg(launcherPathForUser, m_process->errorString()));
    }
}

void LauncherInterfacePrivate::handleProcessFinished()
{
    emit errorOccurred(QCoreApplication::translate("Utils::LauncherSocket",
                       "Process launcher closed unexpectedly: %1")
                       .arg(m_process->errorString()));
}

void LauncherInterfacePrivate::handleProcessStderr()
{
    qDebug() << "[launcher]" << m_process->readAllStandardError();
}

} // namespace Internal

using namespace Utils::Internal;

LauncherInterface::LauncherInterface()
    : m_private(new LauncherInterfacePrivate())
{
    m_private->moveToThread(&m_thread);
    connect(m_private, &LauncherInterfacePrivate::errorOccurred,
            this, &LauncherInterface::errorOccurred);
    connect(&m_thread, &QThread::finished, m_private, &QObject::deleteLater);
    m_thread.start();
}

LauncherInterface &LauncherInterface::instance()
{
    static LauncherInterface p;
    return p;
}

LauncherInterface::~LauncherInterface()
{
    m_thread.quit();
    m_thread.wait();
}

void LauncherInterface::startLauncher(const QString &pathToLauncher)
{
    // Call in launcher's thread.
    instance().m_private->setPathToLauncher(pathToLauncher);
    QMetaObject::invokeMethod(instance().m_private, &LauncherInterfacePrivate::doStart);
}

void LauncherInterface::stopLauncher()
{
    // Call in launcher's thread.
    QMetaObject::invokeMethod(instance().m_private, &LauncherInterfacePrivate::doStop);
}

Internal::LauncherSocket *LauncherInterface::socket()
{
    return instance().m_private->socket();
}


} // namespace Utils

#include "launcherinterface.moc"
