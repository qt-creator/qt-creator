/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "ipcclientproxy.h"

#include "cmbalivemessage.h"
#include "cmbcodecompletedmessage.h"
#include "cmbechomessage.h"
#include "cmbregistertranslationunitsforcodecompletionmessage.h"
#include "ipcserverinterface.h"
#include "projectpartsdonotexistmessage.h"
#include "translationunitdoesnotexistmessage.h"

#include <QDebug>
#include <QIODevice>
#include <QVariant>
#include <QVector>

namespace ClangBackEnd {

IpcClientProxy::IpcClientProxy(IpcServerInterface *server, QIODevice *ioDevice)
    : writeMessageBlock(ioDevice),
      readMessageBlock(ioDevice),
      server(server),
      ioDevice(ioDevice)
{
    QObject::connect(ioDevice, &QIODevice::readyRead, [this] () {IpcClientProxy::readMessages();});
}

IpcClientProxy::IpcClientProxy(IpcClientProxy &&other)
    : writeMessageBlock(std::move(other.writeMessageBlock)),
      readMessageBlock(std::move(other.readMessageBlock)),
      server(std::move(other.server)),
      ioDevice(std::move(other.ioDevice))
{

}

IpcClientProxy &IpcClientProxy::operator=(IpcClientProxy &&other)
{
    writeMessageBlock = std::move(other.writeMessageBlock);
    readMessageBlock = std::move(other.readMessageBlock);
    server = std::move(other.server);
    ioDevice = std::move(other.ioDevice);

    return *this;
}

void IpcClientProxy::alive()
{
    writeMessageBlock.write(QVariant::fromValue(AliveMessage()));
}

void IpcClientProxy::echo(const EchoMessage &message)
{
    writeMessageBlock.write(QVariant::fromValue(message));
}

void IpcClientProxy::codeCompleted(const CodeCompletedMessage &message)
{
    writeMessageBlock.write(QVariant::fromValue(message));
}

void IpcClientProxy::translationUnitDoesNotExist(const TranslationUnitDoesNotExistMessage &message)
{
    writeMessageBlock.write(QVariant::fromValue(message));
}

void IpcClientProxy::projectPartsDoNotExist(const ProjectPartsDoNotExistMessage &message)
{
    writeMessageBlock.write(QVariant::fromValue(message));
}

void IpcClientProxy::readMessages()
{
    for (const QVariant &message : readMessageBlock.readAll())
        server->dispatch(message);
}

bool IpcClientProxy::isUsingThatIoDevice(QIODevice *ioDevice) const
{
    return this->ioDevice == ioDevice;
}

} // namespace ClangBackEnd

