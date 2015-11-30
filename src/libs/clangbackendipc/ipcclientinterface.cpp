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

#include "ipcclientinterface.h"

#include "cmbcodecompletedmessage.h"
#include "cmbechomessage.h"
#include "projectpartsdonotexistmessage.h"
#include "translationunitdoesnotexistmessage.h"
#include "diagnosticschangedmessage.h"

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
    else
        qWarning() << "Unknown IpcClientMessage";
}

} // namespace ClangBackEnd

