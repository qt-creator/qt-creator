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

#ifndef SSHCHANNEL_P_H
#define SSHCHANNEL_P_H

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QString>

namespace Core {
namespace Internal {

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
    virtual void handleChannelRequest(const SshIncomingPacket &packet);

    virtual void closeHook()=0;

    void handleOpenSuccess(quint32 remoteChannelId, quint32 remoteWindowSize,
        quint32 remoteMaxPacketSize);
    void handleOpenFailure(const QString &reason);
    void handleWindowAdjust(quint32 bytesToAdd);
    void handleChannelEof();
    void handleChannelClose();
    void handleChannelData(const QByteArray &data);
    void handleChannelExtendedData(quint32 type, const QByteArray &data);

    void requestSessionStart();
    void sendData(const QByteArray &data);
    void closeChannel();

    virtual ~AbstractSshChannel();

signals:
    void timeout();

protected:
    AbstractSshChannel(quint32 channelId, SshSendFacility &sendFacility);

    quint32 maxDataSize() const;
    void checkChannelActive();

    SshSendFacility &m_sendFacility;

private:
    virtual void handleOpenSuccessInternal()=0;
    virtual void handleOpenFailureInternal()=0;
    virtual void handleChannelDataInternal(const QByteArray &data)=0;
    virtual void handleChannelExtendedDataInternal(quint32 type,
        const QByteArray &data)=0;

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
