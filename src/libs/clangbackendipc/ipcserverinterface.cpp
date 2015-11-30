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

#include "ipcserverinterface.h"

#include "cmbcompletecodemessage.h"
#include "cmbregisterprojectsforeditormessage.h"
#include "cmbregistertranslationunitsforeditormessage.h"
#include "cmbunregisterprojectsforeditormessage.h"
#include "cmbunregistertranslationunitsforeditormessage.h"
#include "registerunsavedfilesforeditormessage.h"
#include "requestdiagnosticsmessage.h"
#include "unregisterunsavedfilesforeditormessage.h"
#include "updatetranslationunitsforeditormessage.h"
#include "updatevisibletranslationunitsmessage.h"

#include <QDebug>
#include <QVariant>

namespace ClangBackEnd {

void IpcServerInterface::dispatch(const QVariant &message)
{
    static const int endMessageType = QMetaType::type("ClangBackEnd::EndMessage");
    static const int registerTranslationUnitsForEditorMessageType = QMetaType::type("ClangBackEnd::RegisterTranslationUnitForEditorMessage");
    static const int updateTranslationUnitsForEditorMessageType = QMetaType::type("ClangBackEnd::UpdateTranslationUnitsForEditorMessage");
    static const int unregisterTranslationUnitsForEditorMessageType = QMetaType::type("ClangBackEnd::UnregisterTranslationUnitsForEditorMessage");
    static const int registerProjectPartsForEditorMessageType = QMetaType::type("ClangBackEnd::RegisterProjectPartsForEditorMessage");
    static const int unregisterProjectPartsForEditorMessageType = QMetaType::type("ClangBackEnd::UnregisterProjectPartsForEditorMessage");
    static const int registerUnsavedFilesForEditorMessageType = QMetaType::type("ClangBackEnd::RegisterUnsavedFilesForEditorMessage");
    static const int unregisterUnsavedFilesForEditorMessageType = QMetaType::type("ClangBackEnd::UnregisterUnsavedFilesForEditorMessage");
    static const int completeCodeMessageType = QMetaType::type("ClangBackEnd::CompleteCodeMessage");
    static const int requestDiagnosticsMessageType = QMetaType::type("ClangBackEnd::RequestDiagnosticsMessage");
    static const int updateVisibleTranslationUnitsMessageType = QMetaType::type("ClangBackEnd::UpdateVisibleTranslationUnitsMessage");


    int type = message.userType();

    if (type == endMessageType)
        end();
    else if (type == registerTranslationUnitsForEditorMessageType)
        registerTranslationUnitsForEditor(message.value<RegisterTranslationUnitForEditorMessage>());
    else if (type == updateTranslationUnitsForEditorMessageType)
        updateTranslationUnitsForEditor(message.value<UpdateTranslationUnitsForEditorMessage>());
    else if (type == unregisterTranslationUnitsForEditorMessageType)
        unregisterTranslationUnitsForEditor(message.value<UnregisterTranslationUnitsForEditorMessage>());
    else if (type == registerProjectPartsForEditorMessageType)
        registerProjectPartsForEditor(message.value<RegisterProjectPartsForEditorMessage>());
    else if (type == unregisterProjectPartsForEditorMessageType)
        unregisterProjectPartsForEditor(message.value<UnregisterProjectPartsForEditorMessage>());
    else if (type == registerUnsavedFilesForEditorMessageType)
        registerUnsavedFilesForEditor(message.value<RegisterUnsavedFilesForEditorMessage>());
    else if (type == unregisterUnsavedFilesForEditorMessageType)
        unregisterUnsavedFilesForEditor(message.value<UnregisterUnsavedFilesForEditorMessage>());
    else if (type == completeCodeMessageType)
        completeCode(message.value<CompleteCodeMessage>());
    else if (type == requestDiagnosticsMessageType)
        requestDiagnostics(message.value<RequestDiagnosticsMessage>());
    else if (type == updateVisibleTranslationUnitsMessageType)
        updateVisibleTranslationUnits(message.value<UpdateVisibleTranslationUnitsMessage>());
    else
        qWarning() << "Unknown IpcServerMessage";
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

