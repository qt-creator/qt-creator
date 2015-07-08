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

#include "cmbalivecommand.h"
#include "cmbcodecompletedcommand.h"
#include "cmbechocommand.h"
#include "cmbregistertranslationunitsforcodecompletioncommand.h"
#include "ipcserverinterface.h"
#include "projectpartsdonotexistcommand.h"
#include "translationunitdoesnotexistcommand.h"

#include <QDebug>
#include <QIODevice>
#include <QVariant>
#include <QVector>

namespace ClangBackEnd {

IpcClientProxy::IpcClientProxy(IpcServerInterface *server, QIODevice *ioDevice)
    : writeCommandBlock(ioDevice),
      readCommandBlock(ioDevice),
      server(server),
      ioDevice(ioDevice)
{
    QObject::connect(ioDevice, &QIODevice::readyRead, [this] () {IpcClientProxy::readCommands();});
}

IpcClientProxy::IpcClientProxy(IpcClientProxy &&other)
    : writeCommandBlock(std::move(other.writeCommandBlock)),
      readCommandBlock(std::move(other.readCommandBlock)),
      server(std::move(other.server)),
      ioDevice(std::move(other.ioDevice))
{

}

IpcClientProxy &IpcClientProxy::operator=(IpcClientProxy &&other)
{
    writeCommandBlock = std::move(other.writeCommandBlock);
    readCommandBlock = std::move(other.readCommandBlock);
    server = std::move(other.server);
    ioDevice = std::move(other.ioDevice);

    return *this;
}

void IpcClientProxy::alive()
{
    writeCommandBlock.write(QVariant::fromValue(AliveCommand()));
}

void IpcClientProxy::echo(const EchoCommand &command)
{
    writeCommandBlock.write(QVariant::fromValue(command));
}

void IpcClientProxy::codeCompleted(const CodeCompletedCommand &command)
{
    writeCommandBlock.write(QVariant::fromValue(command));
}

void IpcClientProxy::translationUnitDoesNotExist(const TranslationUnitDoesNotExistCommand &command)
{
    writeCommandBlock.write(QVariant::fromValue(command));
}

void IpcClientProxy::projectPartsDoNotExist(const ProjectPartsDoNotExistCommand &command)
{
    writeCommandBlock.write(QVariant::fromValue(command));
}

void IpcClientProxy::readCommands()
{
    for (const QVariant &command : readCommandBlock.readAll())
        server->dispatch(command);
}

bool IpcClientProxy::isUsingThatIoDevice(QIODevice *ioDevice) const
{
    return this->ioDevice == ioDevice;
}

} // namespace ClangBackEnd

