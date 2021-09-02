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

#include "filepath.h"
#include "launcherpackets.h"
#include "launchersocket.h"
#include "qtcassert.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QLocalServer>
#include <QProcess>

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
    QString launcherFilePath() const { return m_pathToLauncher + QLatin1String("/qtcreator_processlauncher"); }
signals:
    void errorOccurred(const QString &error);

private:
    QLocalServer * const m_server;
    Internal::LauncherSocket *const m_socket;
    Internal::LauncherProcess *m_process = nullptr;
    QString m_pathToLauncher;
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
    m_process->start(launcherFilePath(), QStringList(m_server->fullServerName()));
}

void LauncherInterfacePrivate::doStop()
{
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

static QMutex s_instanceMutex;
static LauncherInterface *s_instance = nullptr;

LauncherInterface::LauncherInterface()
    : m_private(new LauncherInterfacePrivate())
{
    m_private->moveToThread(&m_thread);
    connect(m_private, &LauncherInterfacePrivate::errorOccurred,
            this, &LauncherInterface::errorOccurred);
    connect(&m_thread, &QThread::finished, m_private, &QObject::deleteLater);
    m_thread.start();
}

LauncherInterface::~LauncherInterface()
{
    m_thread.quit();
    m_thread.wait();
}

// Called from main thread
void LauncherInterface::startLauncher(const QString &pathToLauncher)
{
    QMutexLocker locker(&s_instanceMutex);
    QTC_ASSERT(s_instance == nullptr, return);
    s_instance = new LauncherInterface();
    LauncherInterfacePrivate *p = s_instance->m_private;
    p->setPathToLauncher(pathToLauncher);
    const FilePath launcherFilePath = FilePath::fromString(p->launcherFilePath())
            .cleanPath().withExecutableSuffix();
    auto launcherIsNotExecutable = [&launcherFilePath]() {
        qWarning() << "The Creator's process launcher"
                   << launcherFilePath << "is not executable.";
    };
    QTC_ASSERT(launcherFilePath.isExecutableFile(), launcherIsNotExecutable(); return);
    // Call in launcher's thread.
    QMetaObject::invokeMethod(p, &LauncherInterfacePrivate::doStart);
}

// Called from main thread
void LauncherInterface::stopLauncher()
{
    QMutexLocker locker(&s_instanceMutex);
    QTC_ASSERT(s_instance != nullptr, return);
    LauncherInterfacePrivate *p = s_instance->m_private;
    // Call in launcher's thread.
    QMetaObject::invokeMethod(p, &LauncherInterfacePrivate::doStop, Qt::BlockingQueuedConnection);
    delete s_instance;
    s_instance = nullptr;
}

bool LauncherInterface::isStarted()
{
    QMutexLocker locker(&s_instanceMutex);
    return s_instance != nullptr;
}

bool LauncherInterface::isReady()
{
    QMutexLocker locker(&s_instanceMutex);
    QTC_ASSERT(s_instance != nullptr, return false);

    return s_instance->m_private->socket()->isReady();
}

void LauncherInterface::sendData(const QByteArray &data)
{
    QMutexLocker locker(&s_instanceMutex);
    QTC_ASSERT(s_instance != nullptr, return);

    s_instance->m_private->socket()->sendData(data);
}

Utils::Internal::CallerHandle *LauncherInterface::registerHandle(QObject *parent, quintptr token,
                                                                 ProcessMode mode)
{
    QMutexLocker locker(&s_instanceMutex);
    QTC_ASSERT(s_instance != nullptr, return nullptr);

    return s_instance->m_private->socket()->registerHandle(parent, token, mode);
}

void LauncherInterface::unregisterHandle(quintptr token)
{
    QMutexLocker locker(&s_instanceMutex);
    QTC_ASSERT(s_instance != nullptr, return);

    s_instance->m_private->socket()->unregisterHandle(token);
}

} // namespace Utils

#include "launcherinterface.moc"
