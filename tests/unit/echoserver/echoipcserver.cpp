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

#include "echoipcserver.h"

#include "cmbcodecompletedmessage.h"
#include "cmbcompletecodemessage.h"
#include "cmbechomessage.h"
#include "cmbendmessage.h"
#include "cmbregisterprojectsforeditormessage.h"
#include "cmbregistertranslationunitsforeditormessage.h"
#include "cmbunregisterprojectsforeditormessage.h"
#include "cmbunregistertranslationunitsforeditormessage.h"
#include "connectionserver.h"
#include "registerunsavedfilesforeditormessage.h"
#include "requestdiagnosticsmessage.h"
#include "requesthighlightingmessage.h"
#include "unregisterunsavedfilesforeditormessage.h"
#include "updatetranslationunitsforeditormessage.h"
#include "updatevisibletranslationunitsmessage.h"

#include <QCoreApplication>
#include <QDebug>


namespace ClangBackEnd {

void EchoIpcServer::dispatch(const MessageEnvelop &message)
{
    IpcServerInterface::dispatch(message);
}

void EchoIpcServer::end()
{
    ConnectionServer::removeServer();
    QCoreApplication::quit();
}

void EchoIpcServer::registerTranslationUnitsForEditor(const RegisterTranslationUnitForEditorMessage &message)
{
    echoMessage(message);
}

void EchoIpcServer::updateTranslationUnitsForEditor(const UpdateTranslationUnitsForEditorMessage &message)
{
    echoMessage(message);
}

void EchoIpcServer::unregisterTranslationUnitsForEditor(const UnregisterTranslationUnitsForEditorMessage &message)
{
    echoMessage(message);
}

void EchoIpcServer::registerProjectPartsForEditor(const RegisterProjectPartsForEditorMessage &message)
{
    echoMessage(message);
}

void EchoIpcServer::unregisterProjectPartsForEditor(const UnregisterProjectPartsForEditorMessage &message)
{
    echoMessage(message);
}

void EchoIpcServer::registerUnsavedFilesForEditor(const RegisterUnsavedFilesForEditorMessage &message)
{
    echoMessage(message);
}

void EchoIpcServer::unregisterUnsavedFilesForEditor(const UnregisterUnsavedFilesForEditorMessage &message)
{
    echoMessage(message);
}

void EchoIpcServer::completeCode(const CompleteCodeMessage &message)
{
    echoMessage(message);
}

void EchoIpcServer::requestDiagnostics(const RequestDiagnosticsMessage &message)
{
    echoMessage(message);
}

void EchoIpcServer::requestHighlighting(const RequestHighlightingMessage &message)
{
    echoMessage(message);
}

void EchoIpcServer::updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &message)
{
    echoMessage(message);
}

void EchoIpcServer::echoMessage(const MessageEnvelop &message)
{
    client()->echo(EchoMessage(message));
}

} // namespace ClangBackEnd

