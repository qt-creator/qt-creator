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

#include "ipcclientdispatcher.h"

#include <QDebug>

namespace ClangBackEnd {

void IpcClientDispatcher::addClient(IpcClientInterface *client)
{
    clients.append(client);
}

void IpcClientDispatcher::removeClient(IpcClientInterface *client)
{
    clients.removeOne(client);
}

void IpcClientDispatcher::alive()
{
    for (auto *client : clients)
        client->alive();
}

void IpcClientDispatcher::echo(const EchoMessage &message)
{
    for (auto *client : clients)
        client->echo(message);
}

void IpcClientDispatcher::codeCompleted(const CodeCompletedMessage &message)
{
    for (auto *client : clients)
        client->codeCompleted(message);
}

void IpcClientDispatcher::translationUnitDoesNotExist(const TranslationUnitDoesNotExistMessage &message)
{
    for (auto *client : clients)
        client->translationUnitDoesNotExist(message);
}

void IpcClientDispatcher::projectPartsDoNotExist(const ProjectPartsDoNotExistMessage &message)
{
    for (auto *client : clients)
        client->projectPartsDoNotExist(message);
}

void IpcClientDispatcher::diagnosticsChanged(const DiagnosticsChangedMessage &message)
{
    for (auto *client : clients)
        client->diagnosticsChanged(message);
}

void IpcClientDispatcher::highlightingChanged(const HighlightingChangedMessage &message)
{
    for (auto *client : clients)
        client->highlightingChanged(message);
}

} // namespace ClangBackEnd

