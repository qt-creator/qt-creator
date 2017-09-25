/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "clangcodemodelclientproxy.h"

#include "clangcodemodelclientmessages.h"
#include "clangcodemodelserverinterface.h"
#include "ipcserverinterface.h"
#include "messageenvelop.h"

#include <QDebug>
#include <QIODevice>
#include <QVariant>
#include <QVector>

namespace ClangBackEnd {

ClangCodeModelClientProxy::ClangCodeModelClientProxy(ClangCodeModelServerInterface *server, QIODevice *ioDevice)
    : m_writeMessageBlock(ioDevice),
      m_readMessageBlock(ioDevice),
      m_server(server),
      m_ioDevice(ioDevice)
{
    QObject::connect(m_ioDevice, &QIODevice::readyRead, [this] () {ClangCodeModelClientProxy::readMessages();});
}

ClangCodeModelClientProxy::ClangCodeModelClientProxy(ClangCodeModelClientProxy &&other)
    : m_writeMessageBlock(std::move(other.m_writeMessageBlock)),
      m_readMessageBlock(std::move(other.m_readMessageBlock)),
      m_server(std::move(other.m_server)),
      m_ioDevice(std::move(other.m_ioDevice))
{

}

ClangCodeModelClientProxy &ClangCodeModelClientProxy::operator=(ClangCodeModelClientProxy &&other)
{
    m_writeMessageBlock = std::move(other.m_writeMessageBlock);
    m_readMessageBlock = std::move(other.m_readMessageBlock);
    m_server = std::move(other.m_server);
    m_ioDevice = std::move(other.m_ioDevice);

    return *this;
}

void ClangCodeModelClientProxy::alive()
{
    m_writeMessageBlock.write(AliveMessage());
}

void ClangCodeModelClientProxy::echo(const EchoMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelClientProxy::codeCompleted(const CodeCompletedMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelClientProxy::documentAnnotationsChanged(const DocumentAnnotationsChangedMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelClientProxy::references(const ReferencesMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelClientProxy::followSymbol(const FollowSymbolMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelClientProxy::readMessages()
{
    for (const MessageEnvelop &message : m_readMessageBlock.readAll())
        m_server->dispatch(message);
}

bool ClangCodeModelClientProxy::isUsingThatIoDevice(QIODevice *m_ioDevice) const
{
    return this->m_ioDevice == m_ioDevice;
}

} // namespace ClangBackEnd

