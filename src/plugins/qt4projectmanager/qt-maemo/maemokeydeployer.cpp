/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "maemokeydeployer.h"

#include <coreplugin/ssh/sshremoteprocessrunner.h>

#include <QtCore/QFile>

using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

MaemoKeyDeployer::MaemoKeyDeployer(QObject *parent)
    : QObject(parent)
{
}

MaemoKeyDeployer::~MaemoKeyDeployer()
{
    cleanup();
}

void MaemoKeyDeployer::deployPublicKey(const SshConnectionParameters &sshParams,
    const QString &keyFilePath)
{
    cleanup();
    m_deployProcess = SshRemoteProcessRunner::create(sshParams);

    QFile keyFile(keyFilePath);
    QByteArray key;
    const bool keyFileAccessible = keyFile.open(QIODevice::ReadOnly);
    if (keyFileAccessible)
        key = keyFile.readAll();
    if (!keyFileAccessible || keyFile.error() != QFile::NoError) {
        emit error(tr("Could not read public key file '%1'.").arg(keyFilePath));
        return;
    }

    connect(m_deployProcess.data(), SIGNAL(connectionError(Core::SshError)), this,
        SLOT(handleConnectionFailure()));
    connect(m_deployProcess.data(), SIGNAL(processClosed(int)), this,
        SLOT(handleKeyUploadFinished(int)));
    const QByteArray command = "test -d .ssh "
        "|| mkdir .ssh && chmod 0700 .ssh && echo '"
        + key + "' >> .ssh/authorized_keys && chmod 0600 .ssh/authorized_keys";
    m_deployProcess->run(command);
}

void MaemoKeyDeployer::handleConnectionFailure()
{
    if (!m_deployProcess)
        return;
    const QString errorMsg = m_deployProcess->connection()->errorString();
    cleanup();
    emit error(tr("Connection failed: %1").arg(errorMsg));
}

void MaemoKeyDeployer::handleKeyUploadFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);

    if (!m_deployProcess)
        return;

    const int exitCode = m_deployProcess->process()->exitCode();
    const QString errorMsg = m_deployProcess->process()->errorString();
    cleanup();
    if (exitStatus == SshRemoteProcess::ExitedNormally && exitCode == 0)
        emit finishedSuccessfully();
    else
        emit error(tr("Key deployment failed: %1.").arg(errorMsg));
}

void MaemoKeyDeployer::stopDeployment()
{
    cleanup();
}

void MaemoKeyDeployer::cleanup()
{
    if (m_deployProcess) {
        disconnect(m_deployProcess.data(), 0, this, 0);
        m_deployProcess.clear();
    }
}


} // namespace Internal
} // namespace Qt4ProjectManager
