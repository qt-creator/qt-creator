/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef SSHREMOTECOMMAND_H
#define SSHREMOTECOMMAND_H

#include <coreplugin/core_global.h>

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QByteArray;
QT_END_NAMESPACE

namespace Core {
namespace Internal {
class SshChannelManager;
class SshRemoteProcessPrivate;
class SshSendFacility;
} // namespace Internal


/*
 * This class implements an SSH channel for running a remote process.
 * Objects are created via SshConnection::createRemoteProcess.
 * The process is started via the start() member function.
 * A closeChannel() function is provided, but rarely useful, because
 *    a) when the process ends, the channel is closed automatically, and
 *    b) closing a channel will not necessarily kill the remote process.
 * Therefore, the only sensible use case for calling closeChannel() is to
 * get rid of an SshRemoteProces object before the process is actually started.
 * Note that the process does not have a terminal, so you can't use it
 * for applications that require one.
 */
class CORE_EXPORT SshRemoteProcess : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SshRemoteProcess)

    friend class Internal::SshChannelManager;
    friend class Internal::SshRemoteProcessPrivate;

public:
    typedef QSharedPointer<SshRemoteProcess> Ptr;
    enum ExitStatus { FailedToStart, KilledBySignal, ExitedNormally };

    static const QByteArray AbrtSignal;
    static const QByteArray AlrmSignal;
    static const QByteArray FpeSignal;
    static const QByteArray HupSignal;
    static const QByteArray IllSignal;
    static const QByteArray IntSignal;
    static const QByteArray KillSignal;
    static const QByteArray PipeSignal;
    static const QByteArray QuitSignal;
    static const QByteArray SegvSignal;
    static const QByteArray TermSignal;
    static const QByteArray Usr1Signal;
    static const QByteArray Usr2Signal;

    ~SshRemoteProcess();

    /*
     * Note that this is of limited value in practice, because servers are
     * usually configured to ignore such requests for security reasons.
     */
    void addToEnvironment(const QByteArray &var, const QByteArray &value);

    void start();
    void closeChannel();

    bool isRunning() const;
    QString errorString() const;
    int exitCode() const;
    QByteArray exitSignal() const;

    // Note: This is ignored by the OpenSSH server.
    void sendSignal(const QByteArray &signal);
    void kill() { sendSignal(KillSignal); }

    void sendInput(const QByteArray &data); // Should usually have a trailing newline.

signals:
    void started();
    void outputAvailable(const QByteArray &output);
    void errorOutputAvailable(const QByteArray &output);

    /*
     * Parameter is of type ExitStatus, but we use int because of
     * signal/slot awkwardness (full namespace required).
     */
    void closed(int exitStatus);

private:
    SshRemoteProcess(const QByteArray &command, quint32 channelId,
        Internal::SshSendFacility &sendFacility);

    Internal::SshRemoteProcessPrivate *d;
};

} // namespace Core

#endif // SSHREMOTECOMMAND_H
