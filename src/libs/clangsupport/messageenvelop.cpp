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
        case MessageType::RegisterTranslationUnitForEditorMessage:
            qDebug() << messageEnvelop.message<RegisterTranslationUnitForEditorMessage>();
            break;
        case MessageType::UpdateTranslationUnitsForEditorMessage:
            qDebug() << messageEnvelop.message<UpdateTranslationUnitsForEditorMessage>();
            break;
        case MessageType::UnregisterTranslationUnitsForEditorMessage:
            qDebug() << messageEnvelop.message<UnregisterTranslationUnitsForEditorMessage>();
            break;
        case MessageType::RegisterProjectPartsForEditorMessage:
            qDebug() << messageEnvelop.message<RegisterProjectPartsForEditorMessage>();
            break;
        case MessageType::UnregisterProjectPartsForEditorMessage:
            qDebug() << messageEnvelop.message<UnregisterProjectPartsForEditorMessage>();
            break;
        case MessageType::RegisterUnsavedFilesForEditorMessage:
            qDebug() << messageEnvelop.message<RegisterUnsavedFilesForEditorMessage>();
            break;
        case MessageType::UnregisterUnsavedFilesForEditorMessage:
            qDebug() << messageEnvelop.message<UnregisterUnsavedFilesForEditorMessage>();
            break;
        case MessageType::CompleteCodeMessage:
            qDebug() << messageEnvelop.message<CompleteCodeMessage>();
            break;
        case MessageType::RequestDocumentAnnotationsMessage:
            qDebug() << messageEnvelop.message<RequestDocumentAnnotationsMessage>();
            break;
        case MessageType::RequestReferencesMessage:
            qDebug() << messageEnvelop.message<RequestReferencesMessage>();
            break;
        case MessageType::UpdateVisibleTranslationUnitsMessage:
            qDebug() << messageEnvelop.message<UpdateVisibleTranslationUnitsMessage>();
            break;
        case MessageType::AliveMessage:
            qDebug() << "AliveMessage()";
            break;
        case MessageType::EchoMessage:
            qDebug() << messageEnvelop.message<EchoMessage>();
            break;
        case MessageType::CodeCompletedMessage:
            qDebug() << messageEnvelop.message<CodeCompletedMessage>();
            break;
        case MessageType::ReferencesMessage:
            qDebug() << messageEnvelop.message<ReferencesMessage>();
            break;
        case MessageType::DocumentAnnotationsChangedMessage:
            qDebug() << messageEnvelop.message<DocumentAnnotationsChangedMessage>();
            break;
        default:
            qWarning() << "Unknown Message";
    }

    debug.nospace() << ")";

    return debug;
}

} // namespace ClangBackEnd
