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
    friend class Core::SftpChannel;
public:

    enum SftpState { Inactive, SubsystemRequested, InitSent, Initialized };

    virtual void handleChannelSuccess();
    virtual void handleChannelFailure();

    virtual void closeHook();

    void emitInitializationFailedSignal(const QString &reason);
    void emitInitialized();
    void emitJobFinished(SftpJobId jobId, const QString &error);
    void emitDataAvailable(SftpJobId jobId, const QString &data);
    void emitClosed();

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

    void handleCurrentPacket();
    void handleServerVersion();
    void handleHandle();
    void handleStatus();
    void handleName();
    void handleReadData();
    void handleAttrs();

    void handleStatusGeneric(const JobMap::Iterator &it,
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

    void createDelayedInitFailedSignal(const QString &reason);
    void createDelayedInitializedSignal();
    void createDelayedJobFinishedSignal(SftpJobId jobId,
        const QString &error = QString());
    void createDelayedDataAvailableSignal(SftpJobId jobId, const QString &data);
    void createClosedSignal();

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
