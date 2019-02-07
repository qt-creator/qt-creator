/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#pragma once

#include "client.h"
#include "languageclientsettings.h"

#include <coreplugin/id.h>

#include <languageserverprotocol/diagnostics.h>
#include <languageserverprotocol/languagefeatures.h>
#include <languageserverprotocol/textsynchronization.h>

namespace Core {
class IEditor;
class IDocument;
}

namespace ProjectExplorer { class Project; }

namespace LanguageClient {

class LanguageClientMark;

class LanguageClientManager : public QObject
{
    Q_OBJECT
public:
    LanguageClientManager(const LanguageClientManager &other) = delete;
    LanguageClientManager(LanguageClientManager &&other) = delete;
    ~LanguageClientManager() override;

    static void init();

    static void startClient(Client *client);
    static QVector<Client *> clients();

    static void addExclusiveRequest(const LanguageServerProtocol::MessageId &id, Client *client);
    static void reportFinished(const LanguageServerProtocol::MessageId &id, Client *byClient);

    static void deleteClient(Client *client);

    static void shutdown();

    static LanguageClientManager *instance();

    static QList<Client *> clientsSupportingDocument(const TextEditor::TextDocument *doc);

signals:
    void shutdownFinished();

private:
    LanguageClientManager();

    void editorOpened(Core::IEditor *editor);
    void editorsClosed(const QList<Core::IEditor *> &editors);
    void documentContentsSaved(Core::IDocument *document);
    void documentWillSave(Core::IDocument *document);
    void findLinkAt(const Utils::FileName &filePath, const QTextCursor &cursor,
                    Utils::ProcessLinkCallback callback);
    void findUsages(const Utils::FileName &filePath, const QTextCursor &cursor);

    void projectAdded(ProjectExplorer::Project *project);
    void projectRemoved(ProjectExplorer::Project *project);

    QVector<Client *> reachableClients();
    void sendToAllReachableServers(const LanguageServerProtocol::IContent &content);

    void clientFinished(Client *client);

    bool m_shuttingDown = false;
    QVector<Client *> m_clients;
    QHash<LanguageServerProtocol::MessageId, QList<Client *>> m_exclusiveRequests;

    friend class LanguageClientPlugin;
};
} // namespace LanguageClient
