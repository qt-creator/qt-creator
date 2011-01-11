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

#ifndef SSHCHANNEL_P_H
#define SSHCHANNEL_P_H

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QString>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace Core {
namespace Internal {

struct SshChannelExitSignal;
struct SshChannelExitStatus;
class SshIncomingPacket;
class SshSendFacility;

class AbstractSshChannel : public QObject
{
    Q_OBJECT
public:
    enum ChannelState {
        Inactive, SessionRequested, SessionEstablished, CloseRequested, Closed
    };

    ChannelState channelState() const { return m_state; }
    void setChannelState(ChannelState state);

    void setError(const QString &error) { m_errorString = error; }
    QString errorString() const { return m_errorString; }

    quint32 localChannelId() const { return m_localChannel; }
    quint32 remoteChannel() const { return m_remoteChannel; }

    virtual void handleChannelSuccess()=0;
    virtual void handleChannelFailure()=0;

    virtual void closeHook()=0;

    void handleOpenSuccess(quint32 remoteChannelId, quint32 remoteWindowSize,
        quint32 remoteMaxPacketSize);
    void handleOpenFailure(const QString &reason);
    void handleWindowAdjust(quint32 bytesToAdd);
    void handleChannelEof();
    void handleChannelClose();
    void handleChannelData(const QByteArray &data);
    void handleChannelExtendedData(quint32 type, const QByteArray &data);
    void handleChannelRequest(const SshIncomingPacket &packet);

    void requestSessionStart();
    void sendData(const QByteArray &data);
    void closeChannel();

    virtual ~AbstractSshChannel();

    static const int ReplyTimeout = 10000; // milli seconds

signals:
    void timeout();

protected:
    AbstractSshChannel(quint32 channelId, SshSendFacility &sendFacility);

    quint32 maxDataSize() const;
    void checkChannelActive();

    SshSendFacility &m_sendFacility;
    QTimer * const m_timeoutTimer;

private:
    virtual void handleOpenSuccessInternal()=0;
    virtual void handleOpenFailureInternal()=0;
    virtual void handleChannelDataInternal(const QByteArray &data)=0;
    virtual void handleChannelExtendedDataInternal(quint32 type,
        const QByteArray &data)=0;
    virtual void handleExitStatus(const SshChannelExitStatus &exitStatus)=0;
    virtual void handleExitSignal(const SshChannelExitSignal &signal)=0;

    void setState(ChannelState newState);
    void flushSendBuffer();
    int handleChannelOrExtendedChannelData(const QByteArray &data);

    const quint32 m_localChannel;
    quint32 m_remoteChannel;
    quint32 m_localWindowSize;
    quint32 m_remoteWindowSize;
    quint32 m_remoteMaxPacketSize;
    ChannelState m_state;
    QByteArray m_sendBuffer;
    QString m_errorString;
};

} // namespace Internal
} // namespace Core

#endif // SSHCHANNEL_P_H
