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

#include "sshconnection.h"
#include "sshconnection_p.h"

#include "sftpchannel.h"
#include "sshcapabilities_p.h"
#include "sshchannelmanager_p.h"
#include "sshcryptofacility_p.h"
#include "sshexception_p.h"
#include "sshkeyexchange_p.h"

#include <utils/qtcassert.h>

#include <botan/exceptn.h>
#include <botan/init.h>

#include <QtCore/QFile>
#include <QtCore/QMutex>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QTcpSocket>

namespace Core {

namespace {
    const QByteArray ClientId("SSH-2.0-QtCreator\r\n");

    bool staticInitializationsDone = false;
    QMutex staticInitMutex;

    void doStaticInitializationsIfNecessary()
    {
        if (!staticInitializationsDone) {
            staticInitMutex.lock();
            if (!staticInitializationsDone) {
                Botan::LibraryInitializer::initialize("thread_safe=true");
                qRegisterMetaType<Core::SshError>("Core::SshError");
                qRegisterMetaType<Core::SftpJobId>("Core::SftpJobId");
                staticInitializationsDone = true;
            }
            staticInitMutex.unlock();
        }
    }
} // anonymous namespace


SshConnectionParameters::SshConnectionParameters(ProxyType proxyType) :
    timeout(0),  authType(AuthByKey), port(0), proxyType(proxyType)
{
}

static inline bool equals(const SshConnectionParameters &p1, const SshConnectionParameters &p2)
{
    return p1.host == p2.host && p1.uname == p2.uname
            && p1.authType == p2.authType
            && (p1.authType == SshConnectionParameters::AuthByPwd ?
                    p1.pwd == p2.pwd : p1.privateKeyFile == p2.privateKeyFile)
            && p1.timeout == p2.timeout && p1.port == p2.port;
}

CORE_EXPORT bool operator==(const SshConnectionParameters &p1, const SshConnectionParameters &p2)
{
    return equals(p1, p2);
}

CORE_EXPORT bool operator!=(const SshConnectionParameters &p1, const SshConnectionParameters &p2)
{
    return !equals(p1, p2);
}

// TODO: Mechanism for checking the host key. First connection to host: save, later: compare

SshConnection::Ptr SshConnection::create()
{
    doStaticInitializationsIfNecessary();
    return Ptr(new SshConnection);
}

SshConnection::SshConnection() : d(new Internal::SshConnectionPrivate(this))
{
    connect(d, SIGNAL(connected()), this, SIGNAL(connected()),
        Qt::QueuedConnection);
    connect(d, SIGNAL(dataAvailable(QString)), this,
        SIGNAL(dataAvailable(QString)), Qt::QueuedConnection);
    connect(d, SIGNAL(disconnected()), this, SIGNAL(disconnected()),
        Qt::QueuedConnection);
    connect(d, SIGNAL(error(Core::SshError)), this,
        SIGNAL(error(Core::SshError)), Qt::QueuedConnection);
}

void SshConnection::connectToHost(const SshConnectionParameters &serverInfo)
{
    d->connectToHost(serverInfo);
}

void SshConnection::disconnectFromHost()
{
    d->closeConnection(Internal::SSH_DISCONNECT_BY_APPLICATION, SshNoError, "",
        QString());
}

SshConnection::State SshConnection::state() const
{
    switch (d->state()) {
    case Internal::SocketUnconnected:
        return Unconnected;
    case Internal::ConnectionEstablished:
        return Connected;
    default:
        return Connecting;
    }
}

SshError SshConnection::errorState() const
{
    return d->error();
}

QString SshConnection::errorString() const
{
    return d->errorString();
}

SshConnectionParameters SshConnection::connectionParameters() const
{
    return d->m_connParams;
}

SshConnection::~SshConnection()
{
    disconnect();
    disconnectFromHost();
    delete d;
}

QSharedPointer<SshRemoteProcess> SshConnection::createRemoteProcess(const QByteArray &command)
{
    QTC_ASSERT(state() == Connected, return QSharedPointer<SshRemoteProcess>());
    return d->createRemoteProcess(command);
}

QSharedPointer<SftpChannel> SshConnection::createSftpChannel()
{
    QTC_ASSERT(state() == Connected, return QSharedPointer<SftpChannel>());
    return d->createSftpChannel();
}


namespace Internal {

SshConnectionPrivate::SshConnectionPrivate(SshConnection *conn)
    : m_socket(new QTcpSocket(this)), m_state(SocketUnconnected),
      m_sendFacility(m_socket),
      m_channelManager(new SshChannelManager(m_sendFacility, this)),
      m_connParams(SshConnectionParameters::DefaultProxy),
      m_error(SshNoError), m_ignoreNextPacket(false), m_conn(conn)
{
    setupPacketHandlers();
    m_timeoutTimer.setSingleShot(true);
    connect(m_channelManager, SIGNAL(timeout()), this, SLOT(handleTimeout()));
}

SshConnectionPrivate::~SshConnectionPrivate()
{
    disconnect();
}

void SshConnectionPrivate::setupPacketHandlers()
{
    typedef SshConnectionPrivate This;

    setupPacketHandler(SSH_MSG_KEXINIT, StateList() << SocketConnected,
        &This::handleKeyExchangeInitPacket);
    setupPacketHandler(SSH_MSG_KEXDH_REPLY, StateList() << KeyExchangeStarted,
        &This::handleKeyExchangeReplyPacket);

    setupPacketHandler(SSH_MSG_NEWKEYS, StateList() << KeyExchangeSuccess,
        &This::handleNewKeysPacket);
    setupPacketHandler(SSH_MSG_SERVICE_ACCEPT,
        StateList() << UserAuthServiceRequested,
        &This::handleServiceAcceptPacket);
    setupPacketHandler(SSH_MSG_USERAUTH_PASSWD_CHANGEREQ,
        StateList() << UserAuthRequested, &This::handlePasswordExpiredPacket);
    setupPacketHandler(SSH_MSG_GLOBAL_REQUEST,
        StateList() << ConnectionEstablished, &This::handleGlobalRequest);

    const StateList authReqList = StateList() << UserAuthRequested;
    setupPacketHandler(SSH_MSG_USERAUTH_BANNER, authReqList,
        &This::handleUserAuthBannerPacket);
    setupPacketHandler(SSH_MSG_USERAUTH_SUCCESS, authReqList,
        &This::handleUserAuthSuccessPacket);
    setupPacketHandler(SSH_MSG_USERAUTH_FAILURE, authReqList,
        &This::handleUserAuthFailurePacket);

    const StateList connectedList
        = StateList() << ConnectionEstablished;
    setupPacketHandler(SSH_MSG_CHANNEL_REQUEST, connectedList,
        &This::handleChannelRequest);
    setupPacketHandler(SSH_MSG_CHANNEL_OPEN, connectedList,
        &This::handleChannelOpen);
    setupPacketHandler(SSH_MSG_CHANNEL_OPEN_FAILURE, connectedList,
        &This::handleChannelOpenFailure);
    setupPacketHandler(SSH_MSG_CHANNEL_OPEN_CONFIRMATION, connectedList,
        &This::handleChannelOpenConfirmation);
    setupPacketHandler(SSH_MSG_CHANNEL_SUCCESS, connectedList,
        &This::handleChannelSuccess);
    setupPacketHandler(SSH_MSG_CHANNEL_FAILURE, connectedList,
        &This::handleChannelFailure);
    setupPacketHandler(SSH_MSG_CHANNEL_WINDOW_ADJUST, connectedList,
        &This::handleChannelWindowAdjust);
    setupPacketHandler(SSH_MSG_CHANNEL_DATA, connectedList,
        &This::handleChannelData);
    setupPacketHandler(SSH_MSG_CHANNEL_EXTENDED_DATA, connectedList,
        &This::handleChannelExtendedData);

    const StateList connectedOrClosedList
        = StateList() << SocketUnconnected << ConnectionEstablished;
    setupPacketHandler(SSH_MSG_CHANNEL_EOF, connectedOrClosedList,
        &This::handleChannelEof);
    setupPacketHandler(SSH_MSG_CHANNEL_CLOSE, connectedOrClosedList,
        &This::handleChannelClose);

    setupPacketHandler(SSH_MSG_DISCONNECT, StateList() << SocketConnected
        << KeyExchangeStarted << KeyExchangeSuccess
        << UserAuthServiceRequested << UserAuthRequested
        << ConnectionEstablished, &This::handleDisconnect);
}

void SshConnectionPrivate::setupPacketHandler(SshPacketType type,
    const SshConnectionPrivate::StateList &states,
    SshConnectionPrivate::PacketHandler handler)
{
    m_packetHandlers.insert(type, HandlerInStates(states, handler));
}

void SshConnectionPrivate::handleSocketConnected()
{
    m_state = SocketConnected;
    sendData(ClientId);
}

void SshConnectionPrivate::handleIncomingData()
{
    if (m_state == SocketUnconnected)
        return; // For stuff queued in the event loop after we've called closeConnection();

    try {
        if (!canUseSocket())
            return;
        m_incomingData += m_socket->readAll();
#ifdef CREATOR_SSH_DEBUG
        qDebug("state = %d, remote data size = %d", m_state,
            m_incomingData.count());
#endif
        if (m_state == SocketConnected)
            handleServerId();
        handlePackets();
    } catch (SshServerException &e) {
        closeConnection(e.error, SshProtocolError, e.errorStringServer,
            tr("SSH Protocol error: %1").arg(e.errorStringUser));
    } catch (SshClientException &e) {
        closeConnection(SSH_DISCONNECT_BY_APPLICATION, e.error, "",
            e.errorString);
    } catch (Botan::Exception &e) {
        closeConnection(SSH_DISCONNECT_BY_APPLICATION, SshInternalError, "",
            tr("Botan library exception: %1").arg(e.what()));
    }
}

void SshConnectionPrivate::handleServerId()
{
    const int idOffset = m_incomingData.indexOf("SSH-");
    if (idOffset == -1)
        return;
    m_incomingData.remove(0, idOffset);
    if (m_incomingData.size() < 7)
        return;
    const QByteArray &version = m_incomingData.mid(4, 3);
    if (version != "2.0") {
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_VERSION_NOT_SUPPORTED,
            "Invalid protocol version.",
            tr("Invalid protocol version: Expected '2.0', got '%1'.")
            .arg(SshPacketParser::asUserString(version)));
    }
    const int endOffset = m_incomingData.indexOf("\r\n");
    if (endOffset == -1)
        return;
    if (m_incomingData.at(7) != '-') {
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid server id.", tr("Invalid server id '%1'.")
            .arg(SshPacketParser::asUserString(m_incomingData)));
    }

    m_keyExchange.reset(new SshKeyExchange(m_sendFacility));
    m_keyExchange->sendKexInitPacket(m_incomingData.left(endOffset));
    m_incomingData.remove(0, endOffset + 2);
}

void SshConnectionPrivate::handlePackets()
{
    m_incomingPacket.consumeData(m_incomingData);
    while (m_incomingPacket.isComplete()) {
        handleCurrentPacket();
        m_incomingPacket.clear();
        m_incomingPacket.consumeData(m_incomingData);
    }
}

void SshConnectionPrivate::handleCurrentPacket()
{
    Q_ASSERT(m_incomingPacket.isComplete());
    Q_ASSERT(m_state == KeyExchangeStarted || !m_ignoreNextPacket);

    if (m_ignoreNextPacket) {
        m_ignoreNextPacket = false;
        return;
    }

    QHash<SshPacketType, HandlerInStates>::ConstIterator it
        = m_packetHandlers.find(m_incomingPacket.type());
    if (it == m_packetHandlers.end()) {
        m_sendFacility.sendMsgUnimplementedPacket(m_incomingPacket.serverSeqNr());
        return;
    }
    if (!it.value().first.contains(m_state)) {
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected packet.", tr("Unexpected packet of type %1.")
            .arg(m_incomingPacket.type()));
    }
    (this->*it.value().second)();
}

void SshConnectionPrivate::handleKeyExchangeInitPacket()
{
    // If the server sends a guessed packet, the guess must be wrong,
    // because the algorithms we support requires us to initiate the
    // key exchange.
    if (m_keyExchange->sendDhInitPacket(m_incomingPacket))
        m_ignoreNextPacket = true;
    m_state = KeyExchangeStarted;
}

void SshConnectionPrivate::handleKeyExchangeReplyPacket()
{
    m_keyExchange->sendNewKeysPacket(m_incomingPacket,
        ClientId.left(ClientId.size() - 2));
    m_sendFacility.recreateKeys(*m_keyExchange);
    m_state = KeyExchangeSuccess;
}

void SshConnectionPrivate::handleNewKeysPacket()
{
    m_incomingPacket.recreateKeys(*m_keyExchange);
    m_keyExchange.reset();
    m_sendFacility.sendUserAuthServiceRequestPacket();
    m_state = UserAuthServiceRequested;
}

void SshConnectionPrivate::handleServiceAcceptPacket()
{
    if (m_connParams.authType == SshConnectionParameters::AuthByPwd) {
        m_sendFacility.sendUserAuthByPwdRequestPacket(m_connParams.uname.toUtf8(),
            SshCapabilities::SshConnectionService, m_connParams.pwd.toUtf8());
    } else {
        QFile privKeyFile(m_connParams.privateKeyFile);
        bool couldOpen = privKeyFile.open(QIODevice::ReadOnly);
        QByteArray contents;
        if (couldOpen)
            contents = privKeyFile.readAll();
        if (!couldOpen || privKeyFile.error() != QFile::NoError) {
            throw SshClientException(SshKeyFileError,
                tr("Could not read private key file: %1")
                .arg(privKeyFile.errorString()));
        }

        m_sendFacility.createAuthenticationKey(contents);
        m_sendFacility.sendUserAuthByKeyRequestPacket(m_connParams.uname.toUtf8(),
            SshCapabilities::SshConnectionService);
    }
    m_state = UserAuthRequested;
}

void SshConnectionPrivate::handlePasswordExpiredPacket()
{
    if (m_connParams.authType == SshConnectionParameters::AuthByKey) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Got SSH_MSG_USERAUTH_PASSWD_CHANGEREQ, but did not use password.");
    }

    throw SshClientException(SshAuthenticationError, tr("Password expired."));
}

void SshConnectionPrivate::handleUserAuthBannerPacket()
{
    emit dataAvailable(m_incomingPacket.extractUserAuthBanner().message);
}

void SshConnectionPrivate::handleGlobalRequest()
{
    m_sendFacility.sendRequestFailurePacket();
}

void SshConnectionPrivate::handleUserAuthSuccessPacket()
{
    m_state = ConnectionEstablished;
    m_timeoutTimer.stop();
    emit connected();
}

void SshConnectionPrivate::handleUserAuthFailurePacket()
{
    m_timeoutTimer.stop();
    const QString errorMsg = m_connParams.authType == SshConnectionParameters::AuthByPwd
        ? tr("Server rejected password.") : tr("Server rejected key.");
    throw SshClientException(SshAuthenticationError, errorMsg);
}
void SshConnectionPrivate::handleDebugPacket()
{
    const SshDebug &msg = m_incomingPacket.extractDebug();
    if (msg.display)
        emit dataAvailable(msg.message);
}

void SshConnectionPrivate::handleChannelRequest()
{
    m_channelManager->handleChannelRequest(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelOpen()
{
    m_channelManager->handleChannelOpen(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelOpenFailure()
{
   m_channelManager->handleChannelOpenFailure(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelOpenConfirmation()
{
    m_channelManager->handleChannelOpenConfirmation(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelSuccess()
{
    m_channelManager->handleChannelSuccess(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelFailure()
{
    m_channelManager->handleChannelFailure(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelWindowAdjust()
{
   m_channelManager->handleChannelWindowAdjust(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelData()
{
   m_channelManager->handleChannelData(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelExtendedData()
{
   m_channelManager->handleChannelExtendedData(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelEof()
{
   m_channelManager->handleChannelEof(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelClose()
{
   m_channelManager->handleChannelClose(m_incomingPacket);
}

void SshConnectionPrivate::handleDisconnect()
{
    const SshDisconnect msg = m_incomingPacket.extractDisconnect();
    throw SshServerException(SSH_DISCONNECT_CONNECTION_LOST,
        "", tr("Server closed connection: %1").arg(msg.description));
}

void SshConnectionPrivate::sendData(const QByteArray &data)
{
    if (canUseSocket())
        m_socket->write(data);
}

void SshConnectionPrivate::handleSocketDisconnected()
{
    closeConnection(SSH_DISCONNECT_CONNECTION_LOST, SshClosedByServerError,
        "Connection closed unexpectedly.",
        tr("Connection closed unexpectedly."));
}

void SshConnectionPrivate::handleSocketError()
{
    if (m_error == SshNoError) {
        closeConnection(SSH_DISCONNECT_CONNECTION_LOST, SshSocketError,
            "Network error", m_socket->errorString());
    }
}

void SshConnectionPrivate::handleTimeout()
{
    closeConnection(SSH_DISCONNECT_BY_APPLICATION, SshTimeoutError, "",
        tr("Timeout waiting for reply from server."));
}

void SshConnectionPrivate::connectToHost(const SshConnectionParameters &serverInfo)
{
    m_incomingData.clear();
    m_incomingPacket.reset();
    m_sendFacility.reset();
    m_error = SshNoError;
    m_ignoreNextPacket = false;
    m_errorString.clear();
    connect(m_socket, SIGNAL(connected()), this, SLOT(handleSocketConnected()));
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(handleIncomingData()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this,
        SLOT(handleSocketError()));
    connect(m_socket, SIGNAL(disconnected()), this,
        SLOT(handleSocketDisconnected()));
    connect(&m_timeoutTimer, SIGNAL(timeout()), this, SLOT(handleTimeout()));
    this->m_connParams = serverInfo;
    m_state = SocketConnecting;
    m_timeoutTimer.start(m_connParams.timeout * 1000);
    m_socket->setProxy(m_connParams.proxyType == SshConnectionParameters::DefaultProxy
        ? QNetworkProxy::DefaultProxy : QNetworkProxy::NoProxy);
    m_socket->connectToHost(serverInfo.host, serverInfo.port);
}

void SshConnectionPrivate::closeConnection(SshErrorCode sshError,
    SshError userError, const QByteArray &serverErrorString,
    const QString &userErrorString)
{
    // Prevent endless loops by recursive exceptions.
    if (m_state == SocketUnconnected || m_error != SshNoError)
        return;

    m_error = userError;
    m_errorString = userErrorString;
    m_timeoutTimer.stop();
    disconnect(m_socket, 0, this, 0);
    disconnect(&m_timeoutTimer, 0, this, 0);
    try {
        m_channelManager->closeAllChannels();
        m_sendFacility.sendDisconnectPacket(sshError, serverErrorString);
    } catch (Botan::Exception &) {}  // Nothing sensible to be done here.
    if (m_error != SshNoError)
        emit error(userError);
    if (m_state == ConnectionEstablished)
        emit disconnected();
    if (canUseSocket())
        m_socket->disconnectFromHost();
    m_state = SocketUnconnected;
}

bool SshConnectionPrivate::canUseSocket() const
{
    return m_socket->isValid()
        && m_socket->state() == QAbstractSocket::ConnectedState;
}

QSharedPointer<SshRemoteProcess> SshConnectionPrivate::createRemoteProcess(const QByteArray &command)
{
    return m_channelManager->createRemoteProcess(command);
}

QSharedPointer<SftpChannel> SshConnectionPrivate::createSftpChannel()
{
    return m_channelManager->createSftpChannel();
}

} // namespace Internal
} // namespace Core
