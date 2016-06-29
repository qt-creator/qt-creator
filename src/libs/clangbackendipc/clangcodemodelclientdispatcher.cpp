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

#include "clangcodemodelclientdispatcher.h"

#include <QDebug>

namespace ClangBackEnd {

void ClangCodeModelClientDispatcher::addClient(ClangCodeModelClientInterface *client)
{
    clients.append(client);
}

void ClangCodeModelClientDispatcher::removeClient(ClangCodeModelClientInterface *client)
{
    clients.removeOne(client);
}

void ClangCodeModelClientDispatcher::alive()
{
    for (auto *client : clients)
        client->alive();
}

void ClangCodeModelClientDispatcher::echo(const EchoMessage &message)
{
    for (auto *client : clients)
        client->echo(message);
}

void ClangCodeModelClientDispatcher::codeCompleted(const CodeCompletedMessage &message)
{
    for (auto *client : clients)
        client->codeCompleted(message);
}

void ClangCodeModelClientDispatcher::translationUnitDoesNotExist(const TranslationUnitDoesNotExistMessage &message)
{
    for (auto *client : clients)
        client->translationUnitDoesNotExist(message);
}

void ClangCodeModelClientDispatcher::projectPartsDoNotExist(const ProjectPartsDoNotExistMessage &message)
{
    for (auto *client : clients)
        client->projectPartsDoNotExist(message);
}

void ClangCodeModelClientDispatcher::diagnosticsChanged(const DiagnosticsChangedMessage &message)
{
    for (auto *client : clients)
        client->diagnosticsChanged(message);
}

void ClangCodeModelClientDispatcher::highlightingChanged(const HighlightingChangedMessage &message)
{
    for (auto *client : clients)
        client->highlightingChanged(message);
}

} // namespace ClangBackEnd

