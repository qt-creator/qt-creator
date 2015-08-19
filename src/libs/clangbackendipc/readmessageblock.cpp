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

#include "readmessageblock.h"

#include <QDataStream>
#include <QDebug>
#include <QIODevice>
#include <QVariant>

namespace ClangBackEnd {

ReadMessageBlock::ReadMessageBlock(QIODevice *ioDevice)
    : ioDevice(ioDevice),
      messageCounter(0),
      blockSize(0)
{
}

void ReadMessageBlock::checkIfMessageIsLost(QDataStream &in)
{
    qint64 currentMessageCounter;

    in >> currentMessageCounter;

#ifndef DONT_CHECK_MESSAGE_COUNTER
    bool messageLost = !((currentMessageCounter == 0 && messageCounter == 0) || (messageCounter + 1 == currentMessageCounter));
    if (messageLost)
        qWarning() << "client message lost: " << messageCounter <<  currentMessageCounter;
#endif

    messageCounter = currentMessageCounter;
}

QVariant ReadMessageBlock::read()
{
    QDataStream in(ioDevice);

    QVariant message;

    if (isTheWholeMessageReadable(in)) {
        checkIfMessageIsLost(in);
        in >> message;
    }

    return message;
}

QVector<QVariant> ReadMessageBlock::readAll()
{
    QVector<QVariant> messages;

    while (true) {
        const QVariant message = read();
        if (message.isValid())
            messages.append(message);
        else
            return messages;
    }

    Q_UNREACHABLE();
}

void ReadMessageBlock::resetCounter()
{
    messageCounter = 0;
}

bool ReadMessageBlock::isTheWholeMessageReadable(QDataStream &in)
{
    if (ioDevice->bytesAvailable() == 0)
        return false;

    if (blockSize == 0)
        in >> blockSize;

    if (ioDevice->bytesAvailable() < blockSize)
        return false;

    blockSize = 0;

    return true;
}

} // namespace ClangBackEnd

