/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SFTCHANNEL_P_H
#define SFTCHANNEL_P_H

#include "sftpdefs.h"
#include "sftpincomingpacket_p.h"
#include "sftpoperation_p.h"
#include "sftpoutgoingpacket_p.h"
#include "sshchannel_p.h"

#include <QtCore/QByteArray>
#include <QtCore/QMap>

namespace Core {
class SftpChannel;
namespace Internal {

class SftpChannelPrivate : public AbstractSshChannel
{
    Q_OBJECT
    friend class Core::SftpChannel;
public:

    enum SftpState { Inactive, SubsystemRequested, InitSent, Initialized };

    virtual void handleChannelSuccess();
    virtual void handleChannelFailure();

    virtual void closeHook();

signals:
    void initialized();
    void initializationFailed(const QString &reason);
    void closed();
    void finished(Core::SftpJobId job, const QString &error = QString());
    void dataAvailable(Core::SftpJobId job, const QString &data);

private:
    typedef QMap<SftpJobId, AbstractSftpOperation::Ptr> JobMap;

    SftpChannelPrivate(quint32 channelId, SshSendFacility &sendFacility,
        SftpChannel *sftp);
    SftpJobId createJob(const AbstractSftpOperation::Ptr &job);

    virtual void handleOpenSuccessInternal();
    virtual void handleOpenFailureInternal();
    virtual void handleChannelDataInternal(const QByteArray &data);
    virtual void handleChannelExtendedDataInternal(quint32 type,
        const QByteArray &data);
    virtual void handleExitStatus(const SshChannelExitStatus &exitStatus);
    virtual void handleExitSignal(const SshChannelExitSignal &signal);

    void handleCurrentPacket();
    void handleServerVersion();
    void handleHandle();
    void handleStatus();
    void handleName();
    void handleReadData();
    void handleAttrs();

    void handleStatusGeneric(const JobMap::Iterator &it,
        const SftpStatusResponse &response);
    void handleMkdirStatus(const JobMap::Iterator &it,
        const SftpStatusResponse &response);
    void handleLsStatus(const JobMap::Iterator &it,
        const SftpStatusResponse &response);
    void handleGetStatus(const JobMap::Iterator &it,
        const SftpStatusResponse &response);
    void handlePutStatus(const JobMap::Iterator &it,
        const SftpStatusResponse &response);

    void handleLsHandle(const JobMap::Iterator &it);
    void handleCreateFileHandle(const JobMap::Iterator &it);
    void handleGetHandle(const JobMap::Iterator &it);
    void handlePutHandle(const JobMap::Iterator &it);

    void spawnReadRequests(const SftpDownload::Ptr &job);
    void spawnWriteRequests(const JobMap::Iterator &it);
    void sendReadRequest(const SftpDownload::Ptr &job, quint32 requestId);
    void sendWriteRequest(const JobMap::Iterator &it);
    void finishTransferRequest(const JobMap::Iterator &it);
    void removeTransferRequest(const JobMap::Iterator &it);
    void reportRequestError(const AbstractSftpOperationWithHandle::Ptr &job,
        const QString &error);
    void sendTransferCloseHandle(const AbstractSftpTransfer::Ptr &job,
        quint32 requestId);

    JobMap::Iterator lookupJob(SftpJobId id);
    JobMap m_jobs;
    SftpOutgoingPacket m_outgoingPacket;
    SftpIncomingPacket m_incomingPacket;
    QByteArray m_incomingData;
    SftpJobId m_nextJobId;
    SftpState m_sftpState;
    SftpChannel *m_sftp;
};

} // namespace Internal
} // namespace Core

#endif // SFTPCHANNEL_P_H
