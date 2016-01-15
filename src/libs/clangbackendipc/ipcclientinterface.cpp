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

#include "ipcclientinterface.h"

#include "cmbcodecompletedmessage.h"
#include "cmbechomessage.h"
#include "projectpartsdonotexistmessage.h"
#include "translationunitdoesnotexistmessage.h"
#include "diagnosticschangedmessage.h"
#include "highlightingchangedmessage.h"

#include <QDebug>
#include <QVariant>

namespace ClangBackEnd {


void IpcClientInterface::dispatch(const QVariant &message)
{
    static const int aliveMessageType = QMetaType::type("ClangBackEnd::AliveMessage");
    static const int echoMessageType = QMetaType::type("ClangBackEnd::EchoMessage");
    static const int codeCompletedMessageType = QMetaType::type("ClangBackEnd::CodeCompletedMessage");
    static const int translationUnitDoesNotExistMessage = QMetaType::type("ClangBackEnd::TranslationUnitDoesNotExistMessage");
    static const int projectPartsDoNotExistMessage = QMetaType::type("ClangBackEnd::ProjectPartsDoNotExistMessage");
    static const int diagnosticsChangedMessage = QMetaType::type("ClangBackEnd::DiagnosticsChangedMessage");
    static const int highlightingChangedMessage = QMetaType::type("ClangBackEnd::HighlightingChangedMessage");

    int type = message.userType();

    if (type == aliveMessageType)
        alive();
    else if (type == echoMessageType)
        echo(message.value<EchoMessage>());
    else if (type == codeCompletedMessageType)
        codeCompleted(message.value<CodeCompletedMessage>());
    else if (type == translationUnitDoesNotExistMessage)
        translationUnitDoesNotExist(message.value<TranslationUnitDoesNotExistMessage>());
    else if (type == projectPartsDoNotExistMessage)
        projectPartsDoNotExist(message.value<ProjectPartsDoNotExistMessage>());
    else if (type == diagnosticsChangedMessage)
        diagnosticsChanged(message.value<DiagnosticsChangedMessage>());
    else if (type == highlightingChangedMessage)
        highlightingChanged(message.value<HighlightingChangedMessage>());
    else
        qWarning() << "Unknown IpcClientMessage";
}

} // namespace ClangBackEnd

