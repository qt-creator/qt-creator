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

#include "pchmanagerclientproxy.h"

#include "cmbalivemessage.h"
#include "messageenvelop.h"
#include "pchmanagerserverinterface.h"
#include "precompiledheadersupdatedmessage.h"

#include <QDebug>
#include <QIODevice>

namespace ClangBackEnd {

PchManagerClientProxy::PchManagerClientProxy(PchManagerServerInterface *server, QIODevice *ioDevice)
    : writeMessageBlock(ioDevice),
      readMessageBlock(ioDevice),
      server(server),
      ioDevice(ioDevice)
{
    QObject::connect(ioDevice, &QIODevice::readyRead, [this] () {PchManagerClientProxy::readMessages();});
}

PchManagerClientProxy::PchManagerClientProxy(PchManagerClientProxy &&other)
    : writeMessageBlock(std::move(other.writeMessageBlock)),
      readMessageBlock(std::move(other.readMessageBlock)),
      server(std::move(other.server)),
      ioDevice(std::move(other.ioDevice))
{
}

PchManagerClientProxy &PchManagerClientProxy::operator=(PchManagerClientProxy &&other)
{
    writeMessageBlock = std::move(other.writeMessageBlock);
    readMessageBlock = std::move(other.readMessageBlock);
    server = std::move(other.server);
    ioDevice = std::move(other.ioDevice);

    return *this;
}

void PchManagerClientProxy::readMessages()
{
    for (const MessageEnvelop &message : readMessageBlock.readAll())
        server->dispatch(message);
}

void PchManagerClientProxy::alive()
{
    writeMessageBlock.write(AliveMessage());
}

void PchManagerClientProxy::precompiledHeadersUpdated(PrecompiledHeadersUpdatedMessage &&message)
{
    writeMessageBlock.write(message);
}

} // namespace ClangBackEnd
