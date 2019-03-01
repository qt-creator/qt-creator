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

#include "alivemessage.h"
#include "messageenvelop.h"
#include "pchmanagerserverinterface.h"
#include "precompiledheadersupdatedmessage.h"
#include "progressmessage.h"

#include <QDebug>
#include <QLocalSocket>

namespace ClangBackEnd {

PchManagerClientProxy::PchManagerClientProxy(PchManagerServerInterface *server,
                                             QLocalSocket *localSocket)
    : writeMessageBlock(localSocket)
    , readMessageBlock(localSocket)
    , server(server)
{
    QObject::connect(localSocket, &QIODevice::readyRead, [this]() {
        PchManagerClientProxy::readMessages();
    });
}

PchManagerClientProxy::PchManagerClientProxy(PchManagerServerInterface *server, QIODevice *ioDevice)
    : writeMessageBlock(ioDevice)
    , readMessageBlock(ioDevice)
    , server(server)
{
    QObject::connect(ioDevice, &QIODevice::readyRead, [this]() {
        PchManagerClientProxy::readMessages();
    });
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

void PchManagerClientProxy::progress(ProgressMessage &&message)
{
    writeMessageBlock.write(message);
}

} // namespace ClangBackEnd
