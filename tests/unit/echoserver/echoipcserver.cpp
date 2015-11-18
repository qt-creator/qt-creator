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

void EchoIpcServer::dispatch(const QVariant &message)
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
    echoMessage(QVariant::fromValue(message));
}

void EchoIpcServer::updateTranslationUnitsForEditor(const UpdateTranslationUnitsForEditorMessage &message)
{
    echoMessage(QVariant::fromValue(message));
}

void EchoIpcServer::unregisterTranslationUnitsForEditor(const UnregisterTranslationUnitsForEditorMessage &message)
{
    echoMessage(QVariant::fromValue(message));
}

void EchoIpcServer::registerProjectPartsForEditor(const RegisterProjectPartsForEditorMessage &message)
{
    echoMessage(QVariant::fromValue(message));
}

void EchoIpcServer::unregisterProjectPartsForEditor(const UnregisterProjectPartsForEditorMessage &message)
{
    echoMessage(QVariant::fromValue(message));
}

void EchoIpcServer::registerUnsavedFilesForEditor(const RegisterUnsavedFilesForEditorMessage &message)
{
    echoMessage(QVariant::fromValue(message));
}

void EchoIpcServer::unregisterUnsavedFilesForEditor(const UnregisterUnsavedFilesForEditorMessage &message)
{
    echoMessage(QVariant::fromValue(message));
}

void EchoIpcServer::completeCode(const CompleteCodeMessage &message)
{
    echoMessage(QVariant::fromValue(message));
}

void EchoIpcServer::requestDiagnostics(const RequestDiagnosticsMessage &message)
{
    echoMessage(QVariant::fromValue(message));
}

void EchoIpcServer::requestHighlighting(const RequestHighlightingMessage &message)
{
    echoMessage(QVariant::fromValue(message));
}

void EchoIpcServer::updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &message)
{
    echoMessage(QVariant::fromValue(message));
}

void EchoIpcServer::echoMessage(const QVariant &message)
{
    client()->echo(EchoMessage(message));
}

} // namespace ClangBackEnd

