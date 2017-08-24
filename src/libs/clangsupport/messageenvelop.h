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

#pragma once

#include "clangsupport_global.h"

#include <QByteArray>
#include <QDataStream>
#include <QDebug>

namespace ClangBackEnd {

class MessageEnvelop
{
public:
    MessageEnvelop() = default;

    template <class Message>
    MessageEnvelop(const Message &message)
        : messageType_(MessageTrait<Message>::enumeration)
    {
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream << message;
    }

    template <class Message>
    Message message() const
    {
        Message message;

        QDataStream stream(&data, QIODevice::ReadOnly);
        stream >> message;

        return message;
    }

    MessageType messageType() const
    {
        return messageType_;
    }

    bool isValid() const
    {
        return messageType_ != MessageType::InvalidMessage;
    }

    friend
    QDataStream &operator >>(QDataStream& in, MessageType& messageType)
    {
        quint32 messageTypeAsInt;
        in >> messageTypeAsInt;
        messageType = MessageType(messageTypeAsInt);

        return in;
    }

    friend
    QDataStream &operator<<(QDataStream &out, const MessageEnvelop &messageEnvelop)
    {
        out << static_cast<const quint8>(messageEnvelop.messageType_);
        out << messageEnvelop.data;

        return out;
    }

    friend
    QDataStream &operator>>(QDataStream &in, MessageEnvelop &messageEnvelop)
    {
        quint8 messageType;

        in >> messageType;
        in >> messageEnvelop.data;

        messageEnvelop.messageType_ = static_cast<MessageType>(messageType);

        return in;
    }

    friend
    bool operator==(const MessageEnvelop &first, const MessageEnvelop &second)
    {
        return first.messageType_ == second.messageType_
            && first.data == second.data;
    }

    friend QDebug operator<<(QDebug debug, const MessageEnvelop &messageEnvelop);

private:
    mutable QByteArray data;
    MessageType messageType_ = MessageType::InvalidMessage;
};

} // namespace ClangBackEnd
