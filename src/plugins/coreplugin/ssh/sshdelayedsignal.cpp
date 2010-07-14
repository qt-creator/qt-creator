/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "sshdelayedsignal_p.h"

#include "sftpchannel_p.h"
#include "sshremoteprocess_p.h"

#include <QtCore/QTimer>

namespace Core {
namespace Internal {

SshDelayedSignal::SshDelayedSignal(const QWeakPointer<QObject> &checkObject)
    : m_checkObject(checkObject)
{
    QTimer::singleShot(0, this, SLOT(handleTimeout()));
}

void SshDelayedSignal::handleTimeout()
{
    if (!m_checkObject.isNull())
        emitSignal();
    deleteLater();
}


SftpDelayedSignal::SftpDelayedSignal(SftpChannelPrivate *privChannel,
    const QWeakPointer<QObject> &checkObject)
    : SshDelayedSignal(checkObject), m_privChannel(privChannel) {}


SftpInitializationFailedSignal::SftpInitializationFailedSignal(SftpChannelPrivate *privChannel,
    const QWeakPointer<QObject> &checkObject, const QString &reason)
    : SftpDelayedSignal(privChannel, checkObject), m_reason(reason) {}

void SftpInitializationFailedSignal::emitSignal()
{
    m_privChannel->emitInitializationFailedSignal(m_reason);
}


SftpInitializedSignal::SftpInitializedSignal(SftpChannelPrivate *privChannel,
    const QWeakPointer<QObject> &checkObject)
    : SftpDelayedSignal(privChannel, checkObject) {}

void SftpInitializedSignal::emitSignal()
{
    m_privChannel->emitInitialized();
}


SftpJobFinishedSignal::SftpJobFinishedSignal(SftpChannelPrivate *privChannel,
    const QWeakPointer<QObject> &checkObject, SftpJobId jobId,
    const QString &error)
    : SftpDelayedSignal(privChannel, checkObject), m_jobId(jobId), m_error(error)
{
}

void SftpJobFinishedSignal::emitSignal()
{
    m_privChannel->emitJobFinished(m_jobId, m_error);
}


SftpDataAvailableSignal::SftpDataAvailableSignal(SftpChannelPrivate *privChannel,
    const QWeakPointer<QObject> &checkObject, SftpJobId jobId,
    const QString &data)
    : SftpDelayedSignal(privChannel, checkObject), m_jobId(jobId), m_data(data) {}

void SftpDataAvailableSignal::emitSignal()
{
    m_privChannel->emitDataAvailable(m_jobId, m_data);
}


SftpClosedSignal::SftpClosedSignal(SftpChannelPrivate *privChannel,
    const QWeakPointer<QObject> &checkObject)
    : SftpDelayedSignal(privChannel, checkObject) {}

void SftpClosedSignal::emitSignal()
{
    m_privChannel->emitClosed();
}


SshRemoteProcessDelayedSignal::SshRemoteProcessDelayedSignal(SshRemoteProcessPrivate *privChannel,
    const QWeakPointer<QObject> &checkObject)
    : SshDelayedSignal(checkObject), m_privChannel(privChannel) {}


SshRemoteProcessStartedSignal::SshRemoteProcessStartedSignal(SshRemoteProcessPrivate *privChannel,
    const QWeakPointer<QObject> &checkObject)
    : SshRemoteProcessDelayedSignal(privChannel, checkObject) {}

void SshRemoteProcessStartedSignal::emitSignal()
{
    m_privChannel->emitStartedSignal();
}


SshRemoteProcessOutputAvailableSignal::SshRemoteProcessOutputAvailableSignal(SshRemoteProcessPrivate *privChannel,
    const QWeakPointer<QObject> &checkObject, const QByteArray &output)
    : SshRemoteProcessDelayedSignal(privChannel, checkObject), m_output(output)
{
}

void SshRemoteProcessOutputAvailableSignal::emitSignal()
{
    m_privChannel->emitOutputAvailableSignal(m_output);
}


SshRemoteProcessErrorOutputAvailableSignal::SshRemoteProcessErrorOutputAvailableSignal(SshRemoteProcessPrivate *privChannel,
    const QWeakPointer<QObject> &checkObject, const QByteArray &output)
    : SshRemoteProcessDelayedSignal(privChannel, checkObject), m_output(output)
{
}

void SshRemoteProcessErrorOutputAvailableSignal::emitSignal()
{
    m_privChannel->emitErrorOutputAvailableSignal(m_output);
}


SshRemoteProcessClosedSignal::SshRemoteProcessClosedSignal(SshRemoteProcessPrivate *privChannel,
    const QWeakPointer<QObject> &checkObject, int exitStatus)
    : SshRemoteProcessDelayedSignal(privChannel, checkObject),
      m_exitStatus(exitStatus)
{
}

void SshRemoteProcessClosedSignal::emitSignal()
{
    m_privChannel->emitClosedSignal(m_exitStatus);
}

} // namespace Internal
} // namespace Core
