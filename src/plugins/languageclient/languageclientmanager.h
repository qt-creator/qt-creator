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
#include "languageclient_global.h"
#include "languageclientsettings.h"
#include "locatorfilter.h"
#include "lsplogger.h"

#include <utils/id.h>

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

class LANGUAGECLIENT_EXPORT LanguageClientManager : public QObject
{
    Q_OBJECT
public:
    LanguageClientManager(const LanguageClientManager &other) = delete;
    LanguageClientManager(LanguageClientManager &&other) = delete;
    ~LanguageClientManager() override;

    static void init();

    static void startClient(Client *client);
    static Client *startClient(BaseSettings *setting, ProjectExplorer::Project *project = nullptr);
    static QVector<Client *> clients();

    static void addExclusiveRequest(const LanguageServerProtocol::MessageId &id, Client *client);
    static void reportFinished(const LanguageServerProtocol::MessageId &id, Client *byClient);

    static void shutdownClient(Client *client);
    static void deleteClient(Client *client);

    static void shutdown();

    static LanguageClientManager *instance();

    static QList<Client *> clientsSupportingDocument(const TextEditor::TextDocument *doc);

    static void applySettings();
    static QList<BaseSettings *> currentSettings();
    static void registerClientSettings(BaseSettings *settings);
    static void enableClientSettings(const QString &settingsId);
    static QVector<Client *> clientForSetting(const BaseSettings *setting);
    static const BaseSettings *settingForClient(Client *setting);
    static Client *clientForDocument(TextEditor::TextDocument *document);
    static Client *clientForFilePath(const Utils::FilePath &filePath);
    static Client *clientForUri(const LanguageServerProtocol::DocumentUri &uri);

    ///
    /// \brief openDocumentWithClient
    /// makes sure the document is opened and activated with the client and
    /// deactivates the document for a potential previous active client
    ///
    static void openDocumentWithClient(TextEditor::TextDocument *document, Client *client);

    static void logBaseMessage(const LspLogMessage::MessageSender sender,
                               const QString &clientName,
                               const LanguageServerProtocol::BaseMessage &message);
    static void showLogger();

signals:
    void shutdownFinished();

private:
    LanguageClientManager(QObject *parent);

    void editorOpened(Core::IEditor *editor);
    void documentOpened(Core::IDocument *document);
    void documentClosed(Core::IDocument *document);
    void documentContentsSaved(Core::IDocument *document);
    void documentWillSave(Core::IDocument *document);

    void updateProject(ProjectExplorer::Project *project);
    void projectAdded(ProjectExplorer::Project *project);
    void projectRemoved(ProjectExplorer::Project *project);

    QVector<Client *> reachableClients();
    void sendToAllReachableServers(const LanguageServerProtocol::IContent &content);

    void clientFinished(Client *client);

    bool m_shuttingDown = false;
    QVector<Client *> m_clients;
    QList<BaseSettings *>  m_currentSettings; // owned
    QMap<QString, QVector<Client *>> m_clientsForSetting;
    QHash<TextEditor::TextDocument *, QPointer<Client>> m_clientForDocument;
    QHash<LanguageServerProtocol::MessageId, QList<Client *>> m_exclusiveRequests;
    DocumentLocatorFilter m_currentDocumentLocatorFilter;
    WorkspaceLocatorFilter m_workspaceLocatorFilter;
    WorkspaceClassLocatorFilter m_workspaceClassLocatorFilter;
    WorkspaceMethodLocatorFilter m_workspaceMethodLocatorFilter;
    LspLogger m_logger;
};
} // namespace LanguageClient
