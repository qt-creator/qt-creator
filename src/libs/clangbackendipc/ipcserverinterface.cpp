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

#include "ipcserverinterface.h"

#include "cmbcompletecodemessage.h"
#include "cmbregisterprojectsforeditormessage.h"
#include "cmbregistertranslationunitsforeditormessage.h"
#include "cmbunregisterprojectsforeditormessage.h"
#include "cmbunregistertranslationunitsforeditormessage.h"
#include "messageenvelop.h"
#include "registerunsavedfilesforeditormessage.h"
#include "requestdiagnosticsmessage.h"
#include "requesthighlightingmessage.h"
#include "unregisterunsavedfilesforeditormessage.h"
#include "updatetranslationunitsforeditormessage.h"
#include "updatevisibletranslationunitsmessage.h"

#include <QDebug>
#include <QVariant>

namespace ClangBackEnd {

void IpcServerInterface::dispatch(const MessageEnvelop &messageEnvelop)
{
    switch (messageEnvelop.messageType()) {
        case MessageType::EndMessage:
            end();
            break;
        case MessageType::RegisterTranslationUnitForEditorMessage:
            registerTranslationUnitsForEditor(messageEnvelop.message<RegisterTranslationUnitForEditorMessage>());
            break;
        case MessageType::UpdateTranslationUnitsForEditorMessage:
            updateTranslationUnitsForEditor(messageEnvelop.message<UpdateTranslationUnitsForEditorMessage>());
            break;
        case MessageType::UnregisterTranslationUnitsForEditorMessage:
            unregisterTranslationUnitsForEditor(messageEnvelop.message<UnregisterTranslationUnitsForEditorMessage>());
            break;
        case MessageType::RegisterProjectPartsForEditorMessage:
            registerProjectPartsForEditor(messageEnvelop.message<RegisterProjectPartsForEditorMessage>());
            break;
        case MessageType::UnregisterProjectPartsForEditorMessage:
            unregisterProjectPartsForEditor(messageEnvelop.message<UnregisterProjectPartsForEditorMessage>());
            break;
        case MessageType::RegisterUnsavedFilesForEditorMessage:
            registerUnsavedFilesForEditor(messageEnvelop.message<RegisterUnsavedFilesForEditorMessage>());
            break;
        case MessageType::UnregisterUnsavedFilesForEditorMessage:
            unregisterUnsavedFilesForEditor(messageEnvelop.message<UnregisterUnsavedFilesForEditorMessage>());
            break;
        case MessageType::CompleteCodeMessage:
            completeCode(messageEnvelop.message<CompleteCodeMessage>());
            break;
        case MessageType::RequestDiagnosticsMessage:
            requestDiagnostics(messageEnvelop.message<RequestDiagnosticsMessage>());
            break;
        case MessageType::RequestHighlightingMessage:
            requestHighlighting(messageEnvelop.message<RequestHighlightingMessage>());
            break;
        case MessageType::UpdateVisibleTranslationUnitsMessage:
            updateVisibleTranslationUnits(messageEnvelop.message<UpdateVisibleTranslationUnitsMessage>());
            break;
        default:
            qWarning() << "Unknown IpcServerMessage";
    }
}

void IpcServerInterface::addClient(IpcClientInterface *client)
{
    clientDispatcher.addClient(client);
}

void IpcServerInterface::removeClient(IpcClientInterface *client)
{
    clientDispatcher.removeClient(client);
}

IpcClientInterface *IpcServerInterface::client()
{
    return &clientDispatcher;
}

} // namespace ClangBackEnd

