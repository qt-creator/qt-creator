/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "baseserverproxy.h"
#include "messageenvelop.h"

#include <QLocalSocket>

namespace ClangBackEnd {

BaseServerProxy::BaseServerProxy(IpcClientInterface *client, QLocalSocket *localSocket)
    : m_writeMessageBlock(localSocket)
    , m_readMessageBlock(localSocket)
    , m_client(client)
{
    if (localSocket)
        QObject::connect(localSocket, &QIODevice::readyRead, [this]() {
            BaseServerProxy::readMessages();
        });
}

BaseServerProxy::BaseServerProxy(IpcClientInterface *client, QIODevice *ioDevice)
    : m_writeMessageBlock(ioDevice)
    , m_readMessageBlock(ioDevice)
    , m_client(client)
{
    if (ioDevice)
        QObject::connect(ioDevice, &QIODevice::readyRead, [this] () { BaseServerProxy::readMessages(); });
}

void BaseServerProxy::readMessages()
{
    for (const auto &message : m_readMessageBlock.readAll())
        m_client->dispatch(message);
}

void BaseServerProxy::resetState()
{
    m_writeMessageBlock.resetState();
    m_readMessageBlock.resetState();
}

void BaseServerProxy::setLocalSocket(QLocalSocket *localSocket)
{
    QObject::connect(localSocket, &QIODevice::readyRead, [this]() {
        BaseServerProxy::readMessages();
    });
    m_writeMessageBlock.setLocalSocket(localSocket);
    m_readMessageBlock.setIoDevice(localSocket);
}

} // namespace ClangBackEnd
