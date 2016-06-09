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

#include "cmbalivemessage.h"
#include "cmbcodecompletedmessage.h"
#include "cmbechomessage.h"
#include "cmbregistertranslationunitsforeditormessage.h"
#include "documentannotationschangedmessage.h"
#include "clangcodemodelserverinterface.h"
#include "ipcserverinterface.h"
#include "messageenvelop.h"
#include "projectpartsdonotexistmessage.h"
#include "translationunitdoesnotexistmessage.h"

#include <QDebug>
#include <QIODevice>
#include <QVariant>
#include <QVector>

namespace ClangBackEnd {

ClangCodeModelClientProxy::ClangCodeModelClientProxy(ClangCodeModelServerInterface *server, QIODevice *ioDevice)
    : writeMessageBlock(ioDevice),
      readMessageBlock(ioDevice),
      server(server),
      ioDevice(ioDevice)
{
    QObject::connect(ioDevice, &QIODevice::readyRead, [this] () {ClangCodeModelClientProxy::readMessages();});
}

ClangCodeModelClientProxy::ClangCodeModelClientProxy(ClangCodeModelClientProxy &&other)
    : writeMessageBlock(std::move(other.writeMessageBlock)),
      readMessageBlock(std::move(other.readMessageBlock)),
      server(std::move(other.server)),
      ioDevice(std::move(other.ioDevice))
{

}

ClangCodeModelClientProxy &ClangCodeModelClientProxy::operator=(ClangCodeModelClientProxy &&other)
{
    writeMessageBlock = std::move(other.writeMessageBlock);
    readMessageBlock = std::move(other.readMessageBlock);
    server = std::move(other.server);
    ioDevice = std::move(other.ioDevice);

    return *this;
}

void ClangCodeModelClientProxy::alive()
{
    writeMessageBlock.write(AliveMessage());
}

void ClangCodeModelClientProxy::echo(const EchoMessage &message)
{
    writeMessageBlock.write(message);
}

void ClangCodeModelClientProxy::codeCompleted(const CodeCompletedMessage &message)
{
    writeMessageBlock.write(message);
}

void ClangCodeModelClientProxy::translationUnitDoesNotExist(const TranslationUnitDoesNotExistMessage &message)
{
    writeMessageBlock.write(message);
}

void ClangCodeModelClientProxy::projectPartsDoNotExist(const ProjectPartsDoNotExistMessage &message)
{
    writeMessageBlock.write(message);
}

void ClangCodeModelClientProxy::documentAnnotationsChanged(const DocumentAnnotationsChangedMessage &message)
{
    writeMessageBlock.write(message);
}

void ClangCodeModelClientProxy::readMessages()
{
    for (const MessageEnvelop &message : readMessageBlock.readAll())
        server->dispatch(message);
}

bool ClangCodeModelClientProxy::isUsingThatIoDevice(QIODevice *ioDevice) const
{
    return this->ioDevice == ioDevice;
}

} // namespace ClangBackEnd

