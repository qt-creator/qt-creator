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

#ifndef SSHDELAYEDSIGNAL_P_H
#define SSHDELAYEDSIGNAL_P_H

#include "sftpdefs.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QWeakPointer>

namespace Core {
namespace Internal {
class SftpChannelPrivate;
class SshRemoteProcessPrivate;

class SshDelayedSignal : public QObject
{
    Q_OBJECT
public:
    SshDelayedSignal(const QWeakPointer<QObject> &checkObject);

private:
    Q_SLOT void handleTimeout();
    virtual void emitSignal()=0;

    const QWeakPointer<QObject> m_checkObject;
};


class SftpDelayedSignal : public SshDelayedSignal
{
public:
    SftpDelayedSignal(SftpChannelPrivate *privChannel,
        const QWeakPointer<QObject> &checkObject);

protected:
    SftpChannelPrivate * const m_privChannel;
};

class SftpInitializationFailedSignal : public SftpDelayedSignal
{
public:
    SftpInitializationFailedSignal(SftpChannelPrivate *privChannel,
        const QWeakPointer<QObject> &checkObject, const QString &reason);

private:
    virtual void emitSignal();

    const QString m_reason;
};

class SftpInitializedSignal : public SftpDelayedSignal
{
public:
    SftpInitializedSignal(SftpChannelPrivate *privChannel,
        const QWeakPointer<QObject> &checkObject);

private:
    virtual void emitSignal();
};

class SftpJobFinishedSignal : public SftpDelayedSignal
{
public:
    SftpJobFinishedSignal(SftpChannelPrivate *privChannel,
        const QWeakPointer<QObject> &checkObject, SftpJobId jobId,
        const QString &error);

private:
    virtual void emitSignal();

    const SftpJobId m_jobId;
    const QString m_error;
};

class SftpDataAvailableSignal : public SftpDelayedSignal
{
public:
    SftpDataAvailableSignal(SftpChannelPrivate *privChannel,
        const QWeakPointer<QObject> &checkObject, SftpJobId jobId,
        const QString &data);

private:
    virtual void emitSignal();

    const SftpJobId m_jobId;
    const QString m_data;
};

class SftpClosedSignal : public SftpDelayedSignal
{
public:
    SftpClosedSignal(SftpChannelPrivate *privChannel,
        const QWeakPointer<QObject> &checkObject);

private:
    virtual void emitSignal();
};


class SshRemoteProcessDelayedSignal : public SshDelayedSignal
{
public:
    SshRemoteProcessDelayedSignal(SshRemoteProcessPrivate *privChannel,
        const QWeakPointer<QObject> &checkObject);

protected:
    SshRemoteProcessPrivate * const m_privChannel;
};

class SshRemoteProcessStartedSignal : public SshRemoteProcessDelayedSignal
{
public:
    SshRemoteProcessStartedSignal(SshRemoteProcessPrivate *privChannel,
        const QWeakPointer<QObject> &checkObject);

private:
    virtual void emitSignal();
};

class SshRemoteProcessOutputAvailableSignal
    : public SshRemoteProcessDelayedSignal
{
public:
    SshRemoteProcessOutputAvailableSignal(SshRemoteProcessPrivate *privChannel,
        const QWeakPointer<QObject> &checkObject, const QByteArray &output);

private:
    virtual void emitSignal();

    const QByteArray m_output;
};

class SshRemoteProcessErrorOutputAvailableSignal
    : public SshRemoteProcessDelayedSignal
{
public:
    SshRemoteProcessErrorOutputAvailableSignal(SshRemoteProcessPrivate *privChannel,
        const QWeakPointer<QObject> &checkObject, const QByteArray &output);

private:
    virtual void emitSignal();

    const QByteArray m_output;
};

class SshRemoteProcessClosedSignal : public SshRemoteProcessDelayedSignal
{
public:
    SshRemoteProcessClosedSignal(SshRemoteProcessPrivate *privChannel,
        const QWeakPointer<QObject> &checkObject, int exitStatus);

private:
    virtual void emitSignal();

    const int m_exitStatus;
};

} // namespace Internal
} // namespace Core

#endif // SSHDELAYEDSIGNAL_P_H
