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

#include "refactoringserverproxy.h"

#include "messageenvelop.h"
#include "refactoringclientinterface.h"
#include "clangrefactoringservermessages.h"

#include <QIODevice>
#include <QVector>

namespace ClangBackEnd {

RefactoringServerProxy::RefactoringServerProxy(RefactoringClientInterface *client, QIODevice *ioDevice)
    : writeMessageBlock(ioDevice),
      readMessageBlock(ioDevice),
      client(client)
{
    QObject::connect(ioDevice, &QIODevice::readyRead, [this] () { readMessages(); });
}

void RefactoringServerProxy::end()
{
    writeMessageBlock.write(EndMessage());
}

void RefactoringServerProxy::requestSourceLocationsForRenamingMessage(RequestSourceLocationsForRenamingMessage &&message)
{
    writeMessageBlock.write(message);
}

void RefactoringServerProxy::requestSourceRangesAndDiagnosticsForQueryMessage(RequestSourceRangesAndDiagnosticsForQueryMessage &&message)
{
    writeMessageBlock.write(message);
}

void RefactoringServerProxy::requestSourceRangesForQueryMessage(RequestSourceRangesForQueryMessage &&message)
{
    writeMessageBlock.write(message);
}

void RefactoringServerProxy::updatePchProjectParts(UpdatePchProjectPartsMessage &&message)
{
    writeMessageBlock.write(message);
}

void RefactoringServerProxy::removePchProjectParts(RemovePchProjectPartsMessage &&message)
{
    writeMessageBlock.write(message);
}

void RefactoringServerProxy::cancel()
{
    writeMessageBlock.write(CancelMessage());
}

void RefactoringServerProxy::readMessages()
{
    for (const auto &message : readMessageBlock.readAll())
        client->dispatch(message);
}

void RefactoringServerProxy::resetCounter()
{
    writeMessageBlock.resetCounter();
    readMessageBlock.resetCounter();
}

} // namespace ClangBackEnd
