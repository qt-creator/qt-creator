/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "maemoremotemounter.h"

#include "maemoglobal.h"
#include "maddedevice.h"

#include <projectexplorer/profileinformation.h>
#include <projectexplorer/target.h>
#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocess.h>
#include <remotelinux/remotelinuxusedportsgatherer.h>
#include <utils/qtcassert.h>

#include <QTimer>

using namespace ProjectExplorer;
using namespace QSsh;
using namespace RemoteLinux;
using namespace Utils;

namespace Madde {
namespace Internal {

MaemoRemoteMounter::MaemoRemoteMounter(QObject *parent)
    : QObject(parent), m_utfsServerTimer(new QTimer(this)), m_state(Inactive)
{
    connect(m_utfsServerTimer, SIGNAL(timeout()), this,
        SLOT(handleUtfsServerTimeout()));
    m_utfsServerTimer->setSingleShot(true);
}

MaemoRemoteMounter::~MaemoRemoteMounter()
{
    killAllUtfsServers();
}

void MaemoRemoteMounter::setConnection(SshConnection *connection,
    const IDevice::ConstPtr &devConf)
{
    QTC_ASSERT(m_state == Inactive, return);

    m_connection = connection;
    m_devConf = devConf;
}

void MaemoRemoteMounter::setProfile(const Profile *profile)
{
    QTC_ASSERT(m_state == Inactive, return);

    Core::Id typeId = DeviceTypeProfileInformation::deviceTypeId(profile);
    m_remoteMountsAllowed = MaddeDevice::allowsRemoteMounts(typeId);
    m_maddeRoot = SysRootProfileInformation::sysRoot(profile).parentDir().parentDir();
}

void MaemoRemoteMounter::addMountSpecification(const MaemoMountSpecification &mountSpec,
    bool mountAsRoot)
{
    QTC_ASSERT(m_state == Inactive, return);

    if (m_remoteMountsAllowed && mountSpec.isValid())
        m_mountSpecs << MountInfo(mountSpec, mountAsRoot);
}

bool MaemoRemoteMounter::hasValidMountSpecifications() const
{
    return !m_mountSpecs.isEmpty();
}

void MaemoRemoteMounter::mount(PortList *freePorts,
    const RemoteLinuxUsedPortsGatherer *portsGatherer)
{
    QTC_ASSERT(m_state == Inactive, return);

    Q_ASSERT(m_utfsServers.isEmpty());
    Q_ASSERT(m_connection);

    if (m_mountSpecs.isEmpty()) {
        setState(Inactive);
        emit reportProgress(tr("No directories to mount"));
        emit mounted();
    } else {
        m_freePorts = freePorts;
        m_portsGatherer = portsGatherer;
        startUtfsClients();
    }
}

void MaemoRemoteMounter::unmount()
{
    QTC_ASSERT(m_state == Inactive, return);

    if (m_mountSpecs.isEmpty()) {
        emit reportProgress(tr("No directories to unmount"));
        emit unmounted();
        return;
    }

    QString remoteCall;
    const QString remoteSudo = MaemoGlobal::remoteSudo(m_devConf->type(),
        m_connection->connectionParameters().userName);
    for (int i = 0; i < m_mountSpecs.count(); ++i) {
        remoteCall += QString::fromLatin1("%1 umount %2 && %1 rmdir %2;")
            .arg(remoteSudo, m_mountSpecs.at(i).mountSpec.remoteMountPoint);
    }

    m_unmountProcess = m_connection->createRemoteProcess(remoteCall.toUtf8());
    connect(m_unmountProcess.data(), SIGNAL(closed(int)), this,
        SLOT(handleUnmountProcessFinished(int)));
    setState(Unmounting);
    m_unmountProcess->start();
}

void MaemoRemoteMounter::handleUnmountProcessFinished(int exitStatus)
{
    QTC_ASSERT(m_state == Unmounting || m_state == Inactive, return);

    if (m_state == Inactive)
        return;
    setState(Inactive);

    QString errorMsg;
    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        errorMsg = tr("Could not execute unmount request.");
        break;
    case SshRemoteProcess::CrashExit:
        errorMsg = tr("Failure unmounting: %1")
            .arg(m_unmountProcess->errorString());
        break;
    case SshRemoteProcess::NormalExit:
        break;
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO,
            "Impossible SshRemoteProcess exit status.");
    }

    killAllUtfsServers();

    if (errorMsg.isEmpty()) {
        emit reportProgress(tr("Finished unmounting."));
        emit unmounted();
    } else {
        const QByteArray &umountStderr = m_unmountProcess->readAllStandardError();
        if (!umountStderr.isEmpty())
            errorMsg += tr("\nstderr was: '%1'").arg(QString::fromUtf8(umountStderr));
        emit error(errorMsg);
    }
}

void MaemoRemoteMounter::stop()
{
    setState(Inactive);
}

void MaemoRemoteMounter::startUtfsClients()
{
    const QString userName = m_connection->connectionParameters().userName;
    const QString chmodFuse = MaemoGlobal::remoteSudo(m_devConf->type(),
        userName) + QLatin1String(" chmod a+r+w /dev/fuse");
    const QString chmodUtfsClient
        = QLatin1String("chmod a+x ") + utfsClientOnDevice();
    const QLatin1String andOp(" && ");
    QString remoteCall = chmodFuse + andOp + chmodUtfsClient;
    for (int i = 0; i < m_mountSpecs.count(); ++i) {
        MountInfo &mountInfo = m_mountSpecs[i];
        mountInfo.remotePort
            = m_portsGatherer->getNextFreePort(m_freePorts);
        if (mountInfo.remotePort == -1) {
            setState(Inactive);
            emit error(tr("Error: Not enough free ports on device to fulfill all mount requests."));
            return;
        }

        const QString remoteSudo
            = MaemoGlobal::remoteSudo(m_devConf->type(), userName);
        const MaemoMountSpecification &mountSpec = mountInfo.mountSpec;
        const QString mkdir = QString::fromLatin1("%1 mkdir -p %2")
            .arg(remoteSudo, mountSpec.remoteMountPoint);
        const QString chmod = QString::fromLatin1("%1 chmod a+r+w+x %2")
            .arg(remoteSudo, mountSpec.remoteMountPoint);
        QString utfsClient
            = QString::fromLatin1("%1 -l %2 -r %2 -b %2 %4 -o nonempty")
                  .arg(utfsClientOnDevice()).arg(mountInfo.remotePort)
                  .arg(mountSpec.remoteMountPoint);
        if (mountInfo.mountAsRoot) {
            utfsClient.prepend(MaemoGlobal::remoteSudo(m_devConf->type(),
                userName) + QLatin1Char(' '));
        }
        QLatin1String seqOp("; ");
        remoteCall += seqOp + MaemoGlobal::remoteSourceProfilesCommand()
            + seqOp + mkdir + andOp + chmod + andOp + utfsClient;
    }

    emit reportProgress(tr("Starting remote UTFS clients..."));
    m_mountProcess = m_connection->createRemoteProcess(remoteCall.toUtf8());
    connect(m_mountProcess.data(), SIGNAL(started()), this,
        SLOT(handleUtfsClientsStarted()));
    connect(m_mountProcess.data(), SIGNAL(closed(int)), this,
        SLOT(handleUtfsClientsFinished(int)));
    m_mountProcess->start();

    setState(UtfsClientsStarting);
}

void MaemoRemoteMounter::handleUtfsClientsStarted()
{
    QTC_ASSERT(m_state == UtfsClientsStarting || m_state == Inactive, return);

    if (m_state == UtfsClientsStarting) {
        setState(UtfsClientsStarted);
        QTimer::singleShot(250, this, SLOT(startUtfsServers()));
    }
}

void MaemoRemoteMounter::handleUtfsClientsFinished(int exitStatus)
{
    QTC_ASSERT(m_state == UtfsClientsStarting || m_state == UtfsClientsStarted
        || m_state == UtfsServersStarted || m_state == Inactive, return);

    if (m_state == Inactive)
        return;

    setState(Inactive);
    if (exitStatus == SshRemoteProcess::NormalExit
            && m_mountProcess->exitCode() == 0) {
        emit reportProgress(tr("Mount operation succeeded."));
        emit mounted();
    } else {
        QString errMsg = tr("Failure running UTFS client: %1")
            .arg(m_mountProcess->errorString());
        const QByteArray &mountStderr = m_mountProcess->readAllStandardError();
        if (!mountStderr.isEmpty())
            errMsg += tr("\nstderr was: '%1'").arg(QString::fromUtf8(mountStderr));
        emit error(errMsg);
    }
}

void MaemoRemoteMounter::startUtfsServers()
{
    QTC_ASSERT(m_state == UtfsClientsStarted || m_state == Inactive, return);

    if (m_state == Inactive)
        return;

    emit reportProgress(tr("Starting UTFS servers..."));
    m_utfsServerTimer->start(30000);
    for (int i = 0; i < m_mountSpecs.count(); ++i) {
        const MountInfo &mountInfo = m_mountSpecs.at(i);
        const MaemoMountSpecification &mountSpec = mountInfo.mountSpec;
        const ProcPtr utfsServerProc(new QProcess);
        const QString port = QString::number(mountInfo.remotePort);
        const QString localSecretOpt = QLatin1String("-l");
        const QString remoteSecretOpt = QLatin1String("-r");
        const QStringList utfsServerArgs = QStringList() << localSecretOpt
            << port << remoteSecretOpt << port << QLatin1String("-c")
            << (m_connection->connectionParameters().host + QLatin1Char(':') + port)
            << mountSpec.localDir;
        connect(utfsServerProc.data(),
            SIGNAL(finished(int,QProcess::ExitStatus)), this,
            SLOT(handleUtfsServerFinished(int,QProcess::ExitStatus)));
        connect(utfsServerProc.data(), SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(handleUtfsServerError(QProcess::ProcessError)));
        connect(utfsServerProc.data(), SIGNAL(readyReadStandardError()), this,
            SLOT(handleUtfsServerStderr()));
        m_utfsServers << utfsServerProc;
        utfsServerProc->start(utfsServer().toString(), utfsServerArgs);
    }

    setState(UtfsServersStarted);
}

void MaemoRemoteMounter::handleUtfsServerStderr()
{
    if (m_state != Inactive) {
        QProcess * const proc = static_cast<QProcess *>(sender());
        const QByteArray &output = proc->readAllStandardError();
        emit debugOutput(QString::fromLocal8Bit(output));
    }
}

void MaemoRemoteMounter::handleUtfsServerError(QProcess::ProcessError)
{
    if (m_state == Inactive || m_utfsServers.isEmpty())
        return;

    QProcess * const proc = static_cast<QProcess *>(sender());
    QString errorString = proc->errorString();
    const QByteArray &errorOutput = proc->readAllStandardError();
    if (!errorOutput.isEmpty()) {
        errorString += tr("\nstderr was: %1")
            .arg(QString::fromLocal8Bit(errorOutput));
    }
    killAllUtfsServers();
    emit error(tr("Error running UTFS server: %1").arg(errorString));

    setState(Inactive);
}

void MaemoRemoteMounter::handleUtfsServerFinished(int /* exitCode */,
    QProcess::ExitStatus exitStatus)
{
    if (m_state != Inactive && exitStatus != QProcess::NormalExit)
        handleUtfsServerError(static_cast<QProcess *>(sender())->error());
}

QString MaemoRemoteMounter::utfsClientOnDevice() const
{
    return QLatin1String("/usr/lib/mad-developer/utfs-client");
}

Utils::FileName MaemoRemoteMounter::utfsServer() const
{
    Utils::FileName result = m_maddeRoot;
    return result.appendPath(QLatin1String("madlib/utfs-server"));
}

void MaemoRemoteMounter::killAllUtfsServers()
{
    foreach (const ProcPtr &proc, m_utfsServers)
        killUtfsServer(proc.data());
    m_utfsServers.clear();
}

void MaemoRemoteMounter::killUtfsServer(QProcess *proc)
{
    disconnect(proc, 0, this, 0);
    proc->terminate();
    proc->waitForFinished(1000);
    proc->kill();
}

void MaemoRemoteMounter::handleUtfsServerTimeout()
{
    QTC_ASSERT(m_state == UtfsServersStarted || m_state == Inactive, return);

    if (m_state == Inactive)
        return;

    killAllUtfsServers();
    emit error(tr("Timeout waiting for UTFS servers to connect."));

    setState(Inactive);
}

void MaemoRemoteMounter::setState(State newState)
{
    if (newState == Inactive) {
        m_utfsServerTimer->stop();
        if (m_mountProcess) {
            disconnect(m_mountProcess.data(), 0, this, 0);
            m_mountProcess->close();
        }
        if (m_unmountProcess) {
            disconnect(m_unmountProcess.data(), 0, this, 0);
            m_unmountProcess->close();
        }
    }
    m_state = newState;
}

} // namespace Internal
} // namespace Madde
