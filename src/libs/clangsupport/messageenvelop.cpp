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

#include "messageenvelop.h"

#include "clangcodemodelclientmessages.h"
#include "clangcodemodelservermessages.h"

namespace ClangBackEnd {

QDebug operator<<(QDebug debug, const MessageEnvelop &messageEnvelop)
{
    debug.nospace() << "MessageEnvelop(";

    switch (messageEnvelop.messageType()) {
        case MessageType::EndMessage:
            qDebug() << "EndMessage()";
            break;
        case MessageType::DocumentsOpenedMessage:
            qDebug() << messageEnvelop.message<DocumentsOpenedMessage>();
            break;
        case MessageType::DocumentsChangedMessage:
            qDebug() << messageEnvelop.message<DocumentsChangedMessage>();
            break;
        case MessageType::DocumentsClosedMessage:
            qDebug() << messageEnvelop.message<DocumentsClosedMessage>();
            break;
        case MessageType::DocumentVisibilityChangedMessage:
            qDebug() << messageEnvelop.message<DocumentVisibilityChangedMessage>();
            break;
        case MessageType::UnsavedFilesUpdatedMessage:
            qDebug() << messageEnvelop.message<UnsavedFilesUpdatedMessage>();
            break;
        case MessageType::UnsavedFilesRemovedMessage:
            qDebug() << messageEnvelop.message<UnsavedFilesRemovedMessage>();
            break;
        case MessageType::RequestCompletionsMessage:
            qDebug() << messageEnvelop.message<RequestCompletionsMessage>();
            break;
        case MessageType::RequestAnnotationsMessage:
            qDebug() << messageEnvelop.message<RequestAnnotationsMessage>();
            break;
        case MessageType::RequestReferencesMessage:
            qDebug() << messageEnvelop.message<RequestReferencesMessage>();
            break;
        case MessageType::RequestToolTipMessage:
            qDebug() << messageEnvelop.message<RequestToolTipMessage>();
            break;
        case MessageType::AliveMessage:
            qDebug() << "AliveMessage()";
            break;
        case MessageType::EchoMessage:
            qDebug() << messageEnvelop.message<EchoMessage>();
            break;
        case MessageType::CompletionsMessage:
            qDebug() << messageEnvelop.message<CompletionsMessage>();
            break;
        case MessageType::ReferencesMessage:
            qDebug() << messageEnvelop.message<ReferencesMessage>();
            break;
        case MessageType::ToolTipMessage:
            qDebug() << messageEnvelop.message<ToolTipMessage>();
            break;
        case MessageType::AnnotationsMessage:
            qDebug() << messageEnvelop.message<AnnotationsMessage>();
            break;
        default:
            qWarning() << "Unknown Message";
    }

    debug.nospace() << ")";

    return debug;
}

} // namespace ClangBackEnd
