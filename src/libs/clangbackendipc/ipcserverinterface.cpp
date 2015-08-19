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
#include "cmbregisterprojectsforcodecompletionmessage.h"
#include "cmbregistertranslationunitsforcodecompletionmessage.h"
#include "cmbunregisterprojectsforcodecompletionmessage.h"
#include "cmbunregistertranslationunitsforcodecompletionmessage.h"

#include <QDebug>
#include <QVariant>

namespace ClangBackEnd {

void IpcServerInterface::dispatch(const QVariant &message)
{
    static const int endMessageType = QMetaType::type("ClangBackEnd::EndMessage");
    static const int registerTranslationUnitsForCodeCompletionMessageType = QMetaType::type("ClangBackEnd::RegisterTranslationUnitForCodeCompletionMessage");
    static const int unregisterTranslationUnitsForCodeCompletionMessageType = QMetaType::type("ClangBackEnd::UnregisterTranslationUnitsForCodeCompletionMessage");
    static const int registerProjectPartsForCodeCompletionMessageType = QMetaType::type("ClangBackEnd::RegisterProjectPartsForCodeCompletionMessage");
    static const int unregisterProjectPartsForCodeCompletionMessageType = QMetaType::type("ClangBackEnd::UnregisterProjectPartsForCodeCompletionMessage");
    static const int completeCodeMessageType = QMetaType::type("ClangBackEnd::CompleteCodeMessage");

    int type = message.userType();

    if (type == endMessageType)
        end();
    else if (type == registerTranslationUnitsForCodeCompletionMessageType)
        registerTranslationUnitsForCodeCompletion(message.value<RegisterTranslationUnitForCodeCompletionMessage>());
    else if (type == unregisterTranslationUnitsForCodeCompletionMessageType)
        unregisterTranslationUnitsForCodeCompletion(message.value<UnregisterTranslationUnitsForCodeCompletionMessage>());
    else if (type == registerProjectPartsForCodeCompletionMessageType)
        registerProjectPartsForCodeCompletion(message.value<RegisterProjectPartsForCodeCompletionMessage>());
    else if (type == unregisterProjectPartsForCodeCompletionMessageType)
        unregisterProjectPartsForCodeCompletion(message.value<UnregisterProjectPartsForCodeCompletionMessage>());
    else if (type == completeCodeMessageType)
        completeCode(message.value<CompleteCodeMessage>());
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

