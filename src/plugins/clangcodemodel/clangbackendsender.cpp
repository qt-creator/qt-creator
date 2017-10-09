/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "clangbackendsender.h"

#include "clangbackendlogging.h"

#include <clangsupport/clangcodemodelconnectionclient.h>
#include <clangsupport/clangcodemodelservermessages.h>

#include <utils/qtcassert.h>

using namespace ClangBackEnd;

namespace ClangCodeModel {
namespace Internal {

BackendSender::BackendSender(ClangCodeModelConnectionClient *connectionClient)
    : m_connection(connectionClient)
{}

void BackendSender::end()
{
    QTC_CHECK(m_connection->isConnected());
    qCDebug(ipcLog) << ">>>" << ClangBackEnd::EndMessage();
     m_connection->sendEndMessage();
}

void BackendSender::registerTranslationUnitsForEditor(const RegisterTranslationUnitForEditorMessage &message)
{
     QTC_CHECK(m_connection->isConnected());
     qCDebug(ipcLog) << ">>>" << message;
     m_connection->serverProxy().registerTranslationUnitsForEditor(message);
}

void BackendSender::updateTranslationUnitsForEditor(const UpdateTranslationUnitsForEditorMessage &message)
{
    QTC_CHECK(m_connection->isConnected());
    qCDebug(ipcLog) << ">>>" << message;
    m_connection->serverProxy().updateTranslationUnitsForEditor(message);
}

void BackendSender::unregisterTranslationUnitsForEditor(const UnregisterTranslationUnitsForEditorMessage &message)
{
     QTC_CHECK(m_connection->isConnected());
     qCDebug(ipcLog) << ">>>" << message;
     m_connection->serverProxy().unregisterTranslationUnitsForEditor(message);
}

void BackendSender::registerProjectPartsForEditor(const RegisterProjectPartsForEditorMessage &message)
{
     QTC_CHECK(m_connection->isConnected());
     qCDebug(ipcLog) << ">>>" << message;
     m_connection->serverProxy().registerProjectPartsForEditor(message);
}

void BackendSender::unregisterProjectPartsForEditor(const UnregisterProjectPartsForEditorMessage &message)
{
     QTC_CHECK(m_connection->isConnected());
     qCDebug(ipcLog) << ">>>" << message;
     m_connection->serverProxy().unregisterProjectPartsForEditor(message);
}

void BackendSender::registerUnsavedFilesForEditor(const RegisterUnsavedFilesForEditorMessage &message)
{
    QTC_CHECK(m_connection->isConnected());
    qCDebug(ipcLog) << ">>>" << message;
    m_connection->serverProxy().registerUnsavedFilesForEditor(message);
}

void BackendSender::unregisterUnsavedFilesForEditor(const UnregisterUnsavedFilesForEditorMessage &message)
{
    QTC_CHECK(m_connection->isConnected());
    qCDebug(ipcLog) << ">>>" << message;
    m_connection->serverProxy().unregisterUnsavedFilesForEditor(message);
}

void BackendSender::completeCode(const CompleteCodeMessage &message)
{
     QTC_CHECK(m_connection->isConnected());
     qCDebug(ipcLog) << ">>>" << message;
     m_connection->serverProxy().completeCode(message);
}

void BackendSender::requestDocumentAnnotations(const RequestDocumentAnnotationsMessage &message)
{
    QTC_CHECK(m_connection->isConnected());
    qCDebug(ipcLog) << ">>>" << message;
    m_connection->serverProxy().requestDocumentAnnotations(message);
}

void BackendSender::requestReferences(const RequestReferencesMessage &message)
{
    QTC_CHECK(m_connection->isConnected());
    qCDebug(ipcLog) << ">>>" << message;
    m_connection->serverProxy().requestReferences(message);
}

void BackendSender::requestFollowSymbol(const RequestFollowSymbolMessage &message)
{
    QTC_CHECK(m_connection->isConnected());
    qCDebug(ipcLog) << ">>>" << message;
    m_connection->serverProxy().requestFollowSymbol(message);
}

void BackendSender::updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &message)
{
    QTC_CHECK(m_connection->isConnected());
    qCDebug(ipcLog) << ">>>" << message;
    m_connection->serverProxy().updateVisibleTranslationUnits(message);
}

bool BackendSender::isConnected() const
{
    return m_connection && m_connection->isConnected();
}

} // namespace Internal
} // namespace ClangCodeModel
