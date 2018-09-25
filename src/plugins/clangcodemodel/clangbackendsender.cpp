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

#define qCDebugIpc() qCDebug(ipcLog) << "====>"

using namespace ClangBackEnd;

namespace ClangCodeModel {
namespace Internal {

BackendSender::BackendSender(ClangCodeModelConnectionClient *connectionClient)
    : m_connection(connectionClient)
{}

void BackendSender::end()
{
    QTC_CHECK(m_connection->isConnected());
    qCDebugIpc() << ClangBackEnd::EndMessage();
     m_connection->sendEndMessage();
}

void BackendSender::documentsOpened(const DocumentsOpenedMessage &message)
{
     QTC_CHECK(m_connection->isConnected());
     qCDebugIpc() << message;
     m_connection->serverProxy().documentsOpened(message);
}

void BackendSender::documentsChanged(const DocumentsChangedMessage &message)
{
    QTC_CHECK(m_connection->isConnected());
    qCDebugIpc() << message;
    m_connection->serverProxy().documentsChanged(message);
}

void BackendSender::documentsClosed(const DocumentsClosedMessage &message)
{
     QTC_CHECK(m_connection->isConnected());
     qCDebugIpc() << message;
     m_connection->serverProxy().documentsClosed(message);
}

void BackendSender::unsavedFilesUpdated(const UnsavedFilesUpdatedMessage &message)
{
    QTC_CHECK(m_connection->isConnected());
    qCDebugIpc() << message;
    m_connection->serverProxy().unsavedFilesUpdated(message);
}

void BackendSender::unsavedFilesRemoved(const UnsavedFilesRemovedMessage &message)
{
    QTC_CHECK(m_connection->isConnected());
    qCDebugIpc() << message;
    m_connection->serverProxy().unsavedFilesRemoved(message);
}

void BackendSender::requestCompletions(const RequestCompletionsMessage &message)
{
     QTC_CHECK(m_connection->isConnected());
     qCDebugIpc() << message;
     m_connection->serverProxy().requestCompletions(message);
}

void BackendSender::requestAnnotations(const RequestAnnotationsMessage &message)
{
    QTC_CHECK(m_connection->isConnected());
    qCDebugIpc() << message;
    m_connection->serverProxy().requestAnnotations(message);
}

void BackendSender::requestReferences(const RequestReferencesMessage &message)
{
    QTC_CHECK(m_connection->isConnected());
    qCDebugIpc() << message;
    m_connection->serverProxy().requestReferences(message);
}

void BackendSender::requestToolTip(const RequestToolTipMessage &message)
{
    QTC_CHECK(m_connection->isConnected());
    qCDebug(ipcLog) << ">>>" << message;
    m_connection->serverProxy().requestToolTip(message);
}

void BackendSender::requestFollowSymbol(const RequestFollowSymbolMessage &message)
{
    QTC_CHECK(m_connection->isConnected());
    qCDebugIpc() << message;
    m_connection->serverProxy().requestFollowSymbol(message);
}

void BackendSender::documentVisibilityChanged(const DocumentVisibilityChangedMessage &message)
{
    QTC_CHECK(m_connection->isConnected());
    qCDebugIpc() << message;
    m_connection->serverProxy().documentVisibilityChanged(message);
}

} // namespace Internal
} // namespace ClangCodeModel
