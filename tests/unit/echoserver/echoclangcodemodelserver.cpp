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

#include "echoclangcodemodelserver.h"

#include <clangsupport/clangcodemodelservermessages.h>
#include <clangsupport/connectionserver.h>

#include <QCoreApplication>
#include <QDebug>


namespace ClangBackEnd {

void EchoClangCodeModelServer::dispatch(const MessageEnvelop &message)
{
    ClangCodeModelServerInterface::dispatch(message);
}

void EchoClangCodeModelServer::end()
{
    ConnectionServer<EchoClangCodeModelServer, ClangCodeModelClientProxy>::removeServer();
    QCoreApplication::quit();
}

void EchoClangCodeModelServer::registerTranslationUnitsForEditor(const RegisterTranslationUnitForEditorMessage &message)
{
    echoMessage(message);
}

void EchoClangCodeModelServer::updateTranslationUnitsForEditor(const UpdateTranslationUnitsForEditorMessage &message)
{
    echoMessage(message);
}

void EchoClangCodeModelServer::unregisterTranslationUnitsForEditor(const UnregisterTranslationUnitsForEditorMessage &message)
{
    echoMessage(message);
}

void EchoClangCodeModelServer::registerProjectPartsForEditor(const RegisterProjectPartsForEditorMessage &message)
{
    echoMessage(message);
}

void EchoClangCodeModelServer::unregisterProjectPartsForEditor(const UnregisterProjectPartsForEditorMessage &message)
{
    echoMessage(message);
}

void EchoClangCodeModelServer::registerUnsavedFilesForEditor(const RegisterUnsavedFilesForEditorMessage &message)
{
    echoMessage(message);
}

void EchoClangCodeModelServer::unregisterUnsavedFilesForEditor(const UnregisterUnsavedFilesForEditorMessage &message)
{
    echoMessage(message);
}

void EchoClangCodeModelServer::completeCode(const CompleteCodeMessage &message)
{
    echoMessage(message);
}

void EchoClangCodeModelServer::requestDocumentAnnotations(const RequestDocumentAnnotationsMessage &message)
{
    echoMessage(message);
}

void EchoClangCodeModelServer::requestReferences(const RequestReferencesMessage &message)
{
    echoMessage(message);
}

void EchoClangCodeModelServer::requestFollowSymbol(const RequestFollowSymbolMessage &message)
{
    echoMessage(message);
}

void EchoClangCodeModelServer::updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &message)
{
    echoMessage(message);
}

void EchoClangCodeModelServer::echoMessage(const MessageEnvelop &message)
{
    client()->echo(EchoMessage(message));
}

} // namespace ClangBackEnd

