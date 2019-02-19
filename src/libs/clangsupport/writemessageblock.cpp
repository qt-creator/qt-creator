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

#include "writemessageblock.h"

#include "messageenvelop.h"

#include <QDataStream>
#include <QDebug>
#include <QLocalSocket>
#include <QVariant>

namespace ClangBackEnd {

WriteMessageBlock::WriteMessageBlock(QIODevice *ioDevice)
    : m_ioDevice(ioDevice)
{}

WriteMessageBlock::WriteMessageBlock(QLocalSocket *localSocket)
    : m_ioDevice(localSocket)
    , m_localSocket(localSocket)
{}

void WriteMessageBlock::write(const MessageEnvelop &message)
{
    QDataStream out(&m_block, QIODevice::WriteOnly | QIODevice::Append);

    int startOffset = m_block.size();
    const qint32 dummyBockSize = 0;
    out << dummyBockSize;

    out << m_messageCounter;

    out << message;

    out.device()->seek(startOffset);
    out << qint32(m_block.size() - startOffset - sizeof(qint32));

    ++m_messageCounter;

    flushBlock();
}

qint64 WriteMessageBlock::counter() const
{
    return m_messageCounter;
}

void WriteMessageBlock::resetState()
{
    m_block.clear();
    m_messageCounter = 0;
}

void WriteMessageBlock::setIoDevice(QIODevice *ioDevice)
{
    m_ioDevice = ioDevice;
    if (m_localSocket != ioDevice)
        m_localSocket = nullptr;

    flushBlock();
}

void WriteMessageBlock::setLocalSocket(QLocalSocket *localSocket)
{
    m_localSocket = localSocket;

    setIoDevice(localSocket);
}

void WriteMessageBlock::flushBlock()
{
    if (m_ioDevice) {
        const qint64 bytesWritten = m_ioDevice->write(m_block);
        m_block.clear();
        if (bytesWritten == -1)
            qWarning() << "Failed to write data:" << m_ioDevice->errorString();
        if (m_localSocket)
            m_localSocket->flush();
    }
}

} // namespace ClangBackEnd

