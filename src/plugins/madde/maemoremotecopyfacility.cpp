/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "maemoremotecopyfacility.h"

#include "maemoglobal.h"

#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/hostosinfo.h>

#include <QDir>

using namespace ProjectExplorer;
using namespace QSsh;

namespace Madde {
namespace Internal {

MaemoRemoteCopyFacility::MaemoRemoteCopyFacility(QObject *parent) :
    QObject(parent), m_copyRunner(0), m_killProcess(0), m_isCopying(false)
{
}

MaemoRemoteCopyFacility::~MaemoRemoteCopyFacility() {}

void MaemoRemoteCopyFacility::copyFiles(SshConnection *connection,
    const IDevice::ConstPtr &device,
    const QList<DeployableFile> &deployables, const QString &mountPoint)
{
    Q_ASSERT(connection->state() == SshConnection::Connected);
    Q_ASSERT(!m_isCopying);

    m_devConf = device;
    m_deployables = deployables;
    m_mountPoint = mountPoint;

    if (!m_copyRunner)
        m_copyRunner = new SshRemoteProcessRunner(this);
    connect(m_copyRunner, SIGNAL(connectionError()), SLOT(handleConnectionError()));
    connect(m_copyRunner, SIGNAL(readyReadStandardOutput()), SLOT(handleRemoteStdout()));
    connect(m_copyRunner, SIGNAL(readyReadStandardError()), SLOT(handleRemoteStderr()));
    connect(m_copyRunner, SIGNAL(processClosed(int)), SLOT(handleCopyFinished(int)));

    m_isCopying = true;
    copyNextFile();
}

void MaemoRemoteCopyFacility::cancel()
{
    Q_ASSERT(m_isCopying);

    if (!m_killProcess)
        m_killProcess = new SshRemoteProcessRunner(this);
    m_killProcess->run("pkill cp", m_devConf->sshParameters());
    setFinished();
}

void MaemoRemoteCopyFacility::handleConnectionError()
{
    setFinished();
    emit finished(tr("Connection failed: %1").arg(m_copyRunner->lastConnectionErrorString()));
}

void MaemoRemoteCopyFacility::handleRemoteStdout()
{
    emit stdoutData(QString::fromUtf8(m_copyRunner->readAllStandardOutput()));
}

void MaemoRemoteCopyFacility::handleRemoteStderr()
{
    emit stderrData(QString::fromUtf8(m_copyRunner->readAllStandardError()));
}

void MaemoRemoteCopyFacility::handleCopyFinished(int exitStatus)
{
    if (!m_isCopying)
        return;

    if (exitStatus != SshRemoteProcess::NormalExit
            || m_copyRunner->processExitCode() != 0) {
        setFinished();
        emit finished(tr("Error: Copy command failed."));
    } else {
        emit fileCopied(m_deployables.takeFirst());
        copyNextFile();
    }
}

void MaemoRemoteCopyFacility::copyNextFile()
{
    Q_ASSERT(m_isCopying);

    if (m_deployables.isEmpty()) {
        setFinished();
        emit finished();
        return;
    }

    const DeployableFile &d = m_deployables.first();
    QString sourceFilePath = m_mountPoint;
    if (Utils::HostOsInfo::isWindowsHost()) {
        const QString localFilePath = QDir::fromNativeSeparators(d.localFilePath().toString());
        sourceFilePath += QLatin1Char('/') + localFilePath.at(0).toLower()
                + localFilePath.mid(2);
    } else {
        sourceFilePath += d.localFilePath().toString();
    }

    QString command = QString::fromLatin1("%1 mkdir -p %3 && %1 cp -a %2 %3")
        .arg(MaemoGlobal::remoteSudo(m_devConf->type(), m_devConf->sshParameters().userName),
            sourceFilePath, d.remoteDirectory());
    emit progress(tr("Copying file '%1' to directory '%2' on the device...")
        .arg(d.localFilePath().toString(), d.remoteDirectory()));
    m_copyRunner->run(command.toUtf8(), m_devConf->sshParameters());
}

void MaemoRemoteCopyFacility::setFinished()
{
    disconnect(m_copyRunner, 0, this, 0);
    m_deployables.clear();
    m_isCopying = false;
}

} // namespace Internal
} // namespace Madde
