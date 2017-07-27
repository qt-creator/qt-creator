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

#include "clangcodemodelserverproxy.h"

#include <clangcodemodelclientinterface.h>
#include <clangcodemodelservermessages.h>
#include <messageenvelop.h>

#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>

namespace ClangBackEnd {

ClangCodeModelServerProxy::ClangCodeModelServerProxy(ClangCodeModelClientInterface *client, QIODevice *ioDevice)
    : m_writeMessageBlock(ioDevice),
      m_readMessageBlock(ioDevice),
      m_client(client)
{
    QObject::connect(ioDevice, &QIODevice::readyRead, [this] () {ClangCodeModelServerProxy::readMessages();});
}

void ClangCodeModelServerProxy::readMessages()
{
    for (const auto &message : m_readMessageBlock.readAll())
        m_client->dispatch(message);
}

void ClangCodeModelServerProxy::resetCounter()
{
    m_writeMessageBlock.resetCounter();
    m_readMessageBlock.resetCounter();
}

void ClangCodeModelServerProxy::end()
{
    m_writeMessageBlock.write(EndMessage());
}

void ClangCodeModelServerProxy::registerTranslationUnitsForEditor(const RegisterTranslationUnitForEditorMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::updateTranslationUnitsForEditor(const ClangBackEnd::UpdateTranslationUnitsForEditorMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::unregisterTranslationUnitsForEditor(const UnregisterTranslationUnitsForEditorMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::registerProjectPartsForEditor(const RegisterProjectPartsForEditorMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::unregisterProjectPartsForEditor(const UnregisterProjectPartsForEditorMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangBackEnd::ClangCodeModelServerProxy::registerUnsavedFilesForEditor(const ClangBackEnd::RegisterUnsavedFilesForEditorMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangBackEnd::ClangCodeModelServerProxy::unregisterUnsavedFilesForEditor(const ClangBackEnd::UnregisterUnsavedFilesForEditorMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::completeCode(const CompleteCodeMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::requestDocumentAnnotations(const RequestDocumentAnnotationsMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::requestReferences(const RequestReferencesMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::requestFollowSymbol(const RequestFollowSymbolMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &message)
{
    m_writeMessageBlock.write(message);
}

} // namespace ClangBackEnd

