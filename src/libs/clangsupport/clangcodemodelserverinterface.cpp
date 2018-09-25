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

#include "clangcodemodelserverinterface.h"
#include "clangcodemodelservermessages.h"
#include "messageenvelop.h"

#include <QDebug>
#include <QVariant>

namespace ClangBackEnd {

void ClangCodeModelServerInterface::dispatch(const MessageEnvelop &messageEnvelop)
{
    switch (messageEnvelop.messageType()) {
        case MessageType::EndMessage:
            end();
            break;
        case MessageType::DocumentsOpenedMessage:
            documentsOpened(messageEnvelop.message<DocumentsOpenedMessage>());
            break;
        case MessageType::DocumentsChangedMessage:
            documentsChanged(messageEnvelop.message<DocumentsChangedMessage>());
            break;
        case MessageType::DocumentsClosedMessage:
            documentsClosed(messageEnvelop.message<DocumentsClosedMessage>());
            break;
        case MessageType::DocumentVisibilityChangedMessage:
            documentVisibilityChanged(messageEnvelop.message<DocumentVisibilityChangedMessage>());
            break;
        case MessageType::UnsavedFilesUpdatedMessage:
            unsavedFilesUpdated(messageEnvelop.message<UnsavedFilesUpdatedMessage>());
            break;
        case MessageType::UnsavedFilesRemovedMessage:
            unsavedFilesRemoved(messageEnvelop.message<UnsavedFilesRemovedMessage>());
            break;
        case MessageType::RequestCompletionsMessage:
            requestCompletions(messageEnvelop.message<RequestCompletionsMessage>());
            break;
        case MessageType::RequestAnnotationsMessage:
            requestAnnotations(messageEnvelop.message<RequestAnnotationsMessage>());
            break;
        case MessageType::RequestReferencesMessage:
            requestReferences(messageEnvelop.message<RequestReferencesMessage>());
            break;
        case MessageType::RequestFollowSymbolMessage:
            requestFollowSymbol(messageEnvelop.message<RequestFollowSymbolMessage>());
            break;
        case MessageType::RequestToolTipMessage:
            requestToolTip(messageEnvelop.message<RequestToolTipMessage>());
            break;
        default:
            qWarning() << "Unknown ClangCodeModelServerMessage";
    }
}

} // namespace ClangBackEnd

