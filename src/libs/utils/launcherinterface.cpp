// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "launcherinterface.h"

#include "filepath.h"
#include "launchersocket.h"
#include "qtcassert.h"
#include "temporarydirectory.h"
#include "utilstr.h"

#include <QDebug>
#include <QDir>
#include <QLocalServer>
#include <QProcess>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

namespace Utils {
namespace Internal {

static QString launcherSocketName()
{
    return TemporaryDirectory::masterDirectoryPath()
           + QStringLiteral("/launcher-%1").arg(QString::number(qApp->applicationPid()));
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
    QProcess *m_process = nullptr;
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
    m_process = new QProcess(this);
    connect(m_process, &QProcess::errorOccurred, this, &LauncherInterfacePrivate::handleProcessError);
    connect(m_process, &QProcess::finished,
            this, &LauncherInterfacePrivate::handleProcessFinished);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &LauncherInterfacePrivate::handleProcessStderr);
#ifdef Q_OS_UNIX
#  if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    m_process->setUnixProcessParameters(QProcess::UnixProcessFlag::CreateNewSession);
#  else
    m_process->setChildProcessModifier([] {
        setpgid(0, 0);
    });
#  endif
#endif

    m_process->start(launcherFilePath(), QStringList(m_server->fullServerName()));
}

void LauncherInterfacePrivate::doStop()
{
    m_server->close();
    QTC_ASSERT(m_process, return);
    m_socket->shutdown();
    m_process->waitForFinished(-1); // Let the process interface finish so that it finishes
                                    // reaping any possible processes it has started.
    delete m_process;
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
        emit errorOccurred(Tr::tr("Failed to start process launcher at \"%1\": %2")
                           .arg(launcherPathForUser, m_process->errorString()));
    }
}

void LauncherInterfacePrivate::handleProcessFinished()
{
    emit errorOccurred(Tr::tr("Process launcher closed unexpectedly: %1")
                       .arg(m_process->errorString()));
}

void LauncherInterfacePrivate::handleProcessStderr()
{
    qDebug() << "[launcher]" << m_process->readAllStandardError();
}

} // namespace Internal

using namespace Utils::Internal;

static QMutex s_instanceMutex;
static QString s_pathToLauncher;
static std::atomic_bool s_started = false;

LauncherInterface::LauncherInterface()
    : m_private(new LauncherInterfacePrivate())
{
    m_private->moveToThread(&m_thread);
    QObject::connect(&m_thread, &QThread::finished, m_private, &QObject::deleteLater);
    m_thread.start();
    m_thread.moveToThread(qApp->thread());

    m_private->setPathToLauncher(s_pathToLauncher);
    const FilePath launcherFilePath = FilePath::fromString(m_private->launcherFilePath())
            .cleanPath().withExecutableSuffix();
    auto launcherIsNotExecutable = [&launcherFilePath] {
        qWarning() << "The Creator's process launcher"
                   << launcherFilePath << "is not executable.";
    };
    QTC_ASSERT(launcherFilePath.isExecutableFile(), launcherIsNotExecutable(); return);
    s_started = true;
    // Call in launcher's thread.
    QMetaObject::invokeMethod(m_private, &LauncherInterfacePrivate::doStart);
}

LauncherInterface::~LauncherInterface()
{
    QMutexLocker locker(&s_instanceMutex);
    LauncherInterfacePrivate *p = instance()->m_private;
    // Call in launcher's thread.
    QMetaObject::invokeMethod(p, &LauncherInterfacePrivate::doStop, Qt::BlockingQueuedConnection);
    m_thread.quit();
    m_thread.wait();
}

void LauncherInterface::setPathToLauncher(const QString &pathToLauncher)
{
    s_pathToLauncher = pathToLauncher;
}

bool LauncherInterface::isStarted()
{
    return s_started;
}

void LauncherInterface::sendData(const QByteArray &data)
{
    QMutexLocker locker(&s_instanceMutex);
    instance()->m_private->socket()->sendData(data);
}

Internal::CallerHandle *LauncherInterface::registerHandle(QObject *parent, quintptr token)
{
    QMutexLocker locker(&s_instanceMutex);
    return instance()->m_private->socket()->registerHandle(parent, token);
}

void LauncherInterface::unregisterHandle(quintptr token)
{
    QMutexLocker locker(&s_instanceMutex);
    instance()->m_private->socket()->unregisterHandle(token);
}

} // namespace Utils

#include "launcherinterface.moc"
