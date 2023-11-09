// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclientmanager.h"

#include "languageclientplugin.h"
#include "languageclientsymbolsupport.h"
#include "languageclienttr.h"
#include "locatorfilter.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/find/searchresultwindow.h>
#include <coreplugin/icore.h>
#include <coreplugin/navigationwidget.h>

#include <extensionsystem/pluginmanager.h>

#include <languageserverprotocol/messages.h>
#include <languageserverprotocol/progresssupport.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/textmark.h>

#include <utils/algorithm.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QTimer>

using namespace ExtensionSystem;
using namespace LanguageServerProtocol;

namespace LanguageClient {

static Q_LOGGING_CATEGORY(Log, "qtc.languageclient.manager", QtWarningMsg)

static LanguageClientManager *managerInstance = nullptr;

class LanguageClientManagerPrivate
{
    LanguageCurrentDocumentFilter m_currentDocumentFilter;
    LanguageAllSymbolsFilter m_allSymbolsFilter;
    LanguageClassesFilter m_classFilter;
    LanguageFunctionsFilter m_functionFilter;
};

LanguageClientManager::LanguageClientManager(QObject *parent)
    : QObject(parent)
{
    managerInstance = this;
    d.reset(new LanguageClientManagerPrivate);
    using namespace Core;
    using namespace ProjectExplorer;
    connect(EditorManager::instance(), &EditorManager::editorOpened,
            this, &LanguageClientManager::editorOpened);
    connect(EditorManager::instance(), &EditorManager::documentOpened,
            this, &LanguageClientManager::documentOpened);
    connect(EditorManager::instance(), &EditorManager::documentClosed,
            this, &LanguageClientManager::documentClosed);
    connect(ProjectManager::instance(), &ProjectManager::projectAdded,
            this, &LanguageClientManager::projectAdded);
    connect(ProjectManager::instance(), &ProjectManager::projectRemoved,
            this, [&](Project *project) { project->disconnect(this); });
}

LanguageClientManager::~LanguageClientManager()
{
    QTC_ASSERT(m_clients.isEmpty(), qDeleteAll(m_clients));
    qDeleteAll(m_currentSettings);
    managerInstance = nullptr;
}

void LanguageClientManager::init()
{
    if (managerInstance)
        return;
    QTC_ASSERT(LanguageClientPlugin::instance(), return);
    new LanguageClientManager(LanguageClientPlugin::instance());
}

void LanguageClient::LanguageClientManager::addClient(Client *client)
{
    QTC_ASSERT(managerInstance, return);
    QTC_ASSERT(client, return);

    if (managerInstance->m_clients.contains(client))
        return;

    qCDebug(Log) << "add client: " << client->name() << client;
    managerInstance->m_clients << client;
    connect(client, &Client::finished, managerInstance, [client]() { clientFinished(client); });
    connect(client,
            &Client::initialized,
            managerInstance,
            [client](const LanguageServerProtocol::ServerCapabilities &capabilities) {
                emit managerInstance->clientInitialized(client);
                managerInstance->m_inspector.clientInitialized(client->name(), capabilities);
            });
    connect(client,
            &Client::capabilitiesChanged,
            managerInstance,
            [client](const DynamicCapabilities &capabilities) {
                managerInstance->m_inspector.updateCapabilities(client->name(), capabilities);
            });
    connect(client,
            &Client::destroyed,
            managerInstance, [client]() {
                QTC_ASSERT(!managerInstance->m_clients.contains(client),
                           managerInstance->m_clients.removeAll(client));
                for (QList<Client *> &clients : managerInstance->m_clientsForSetting)
                    QTC_CHECK(clients.removeAll(client) == 0);
            });

    ProjectExplorer::Project *project = client->project();
    if (!project)
        project = ProjectExplorer::ProjectManager::startupProject();
    if (project)
        client->updateConfiguration(ProjectSettings(project).workspaceConfiguration());

    emit managerInstance->clientAdded(client);
}

void LanguageClientManager::restartClient(Client *client)
{
    QTC_ASSERT(managerInstance, return);
    if (!client)
        return;
    managerInstance->m_restartingClients.insert(client);
    if (client->reachable())
        client->shutdown();
}

void LanguageClientManager::clientStarted(Client *client)
{
    qCDebug(Log) << "client started: " << client->name() << client;
    QTC_ASSERT(managerInstance, return);
    QTC_ASSERT(client, return);
    if (client->state() != Client::Uninitialized) // do not proceed if we already received an error
        return;
    if (PluginManager::isShuttingDown()) {
        clientFinished(client);
        return;
    }
    client->initialize();
    const QList<TextEditor::TextDocument *> &clientDocs
        = managerInstance->m_clientForDocument.keys(client);
    for (TextEditor::TextDocument *document : clientDocs)
        client->openDocument(document);
}

void LanguageClientManager::clientFinished(Client *client)
{
    QTC_ASSERT(managerInstance, return);

    if (managerInstance->m_restartingClients.remove(client)) {
        client->resetRestartCounter();
        client->reset();
        client->start();
        return;
    }

    constexpr int restartTimeoutS = 5;
    const bool unexpectedFinish = client->state() != Client::Shutdown
                                  && client->state() != Client::ShutdownRequested;

    const QList<TextEditor::TextDocument *> &clientDocs
        = managerInstance->m_clientForDocument.keys(client);
    if (unexpectedFinish) {
        if (!PluginManager::isShuttingDown()) {
            const bool shouldRestart = client->state() > Client::FailedToInitialize;
            if (shouldRestart && client->reset()) {
                qCDebug(Log) << "restart unexpectedly finished client: " << client->name() << client;
                client->log(
                    Tr::tr("Unexpectedly finished. Restarting in %1 seconds.").arg(restartTimeoutS));
                QTimer::singleShot(restartTimeoutS * 1000, client, [client]() { client->start(); });
                for (TextEditor::TextDocument *document : clientDocs) {
                    client->deactivateDocument(document);
                    if (Core::EditorManager::currentEditor()->document() == document)
                        TextEditor::IOutlineWidgetFactory::updateOutline();
                }
                return;
            }
            qCDebug(Log) << "client finished unexpectedly: " << client->name() << client;
            client->log(Tr::tr("Unexpectedly finished."));
        }
    }

    if (unexpectedFinish || !QTC_GUARD(clientDocs.isEmpty())) {
        for (TextEditor::TextDocument *document : clientDocs)
            openDocumentWithClient(document, nullptr);
    }

    deleteClient(client);
    if (isShutdownFinished())
        emit managerInstance->shutdownFinished();
}

Client *LanguageClientManager::startClient(const BaseSettings *setting,
                                           ProjectExplorer::Project *project)
{
    QTC_ASSERT(managerInstance, return nullptr);
    QTC_ASSERT(setting, return nullptr);
    QTC_ASSERT(setting->isValid(), return nullptr);
    Client *client = setting->createClient(project);
    qCDebug(Log) << "start client: " << client->name() << client;
    QTC_ASSERT(client, return nullptr);
    client->start();
    managerInstance->m_clientsForSetting[setting->m_id].append(client);
    return client;
}

const QList<Client *> LanguageClientManager::clients()
{
    QTC_ASSERT(managerInstance, return {});
    return managerInstance->m_clients;
}

void LanguageClientManager::shutdownClient(Client *client)
{
    if (!client)
        return;
    qCDebug(Log) << "request client shutdown: " << client->name() << client;
    // reset and deactivate the documents for that client by assigning a null client already when
    // requesting the shutdown so they can get reassigned to another server right after this request
    for (TextEditor::TextDocument *document : managerInstance->m_clientForDocument.keys(client))
        openDocumentWithClient(document, nullptr);
    if (client->reachable())
        client->shutdown();
    else if (client->state() != Client::Shutdown && client->state() != Client::ShutdownRequested)
        deleteClient(client);
}

void LanguageClientManager::deleteClient(Client *client)
{
    QTC_ASSERT(managerInstance, return);
    QTC_ASSERT(client, return);
    qCDebug(Log) << "delete client: " << client->name() << client;
    client->disconnect(managerInstance);
    managerInstance->m_clients.removeAll(client);
    for (QList<Client *> &clients : managerInstance->m_clientsForSetting)
        clients.removeAll(client);

    // a deleteLater is not sufficient here as it pastes the delete later event at the end
    // of the main event loop and when the plugins are shutdown we spawn an additional eventloop
    // that will not handle the delete later event. Use invokeMethod with Qt::QueuedConnection
    // instead.
    QMetaObject::invokeMethod(client, [client] {delete client;}, Qt::QueuedConnection);
    managerInstance->trackClientDeletion(client);

    if (!PluginManager::isShuttingDown())
        emit instance()->clientRemoved(client);
}

void LanguageClientManager::shutdown()
{
    QTC_ASSERT(managerInstance, return);
    qCDebug(Log) << "shutdown manager";
    const auto clients = managerInstance->clients();
    for (Client *client : clients)
        shutdownClient(client);
    QTimer::singleShot(3000, managerInstance, [] {
        const auto clients = managerInstance->clients();
        for (Client *client : clients)
            deleteClient(client);
        emit managerInstance->shutdownFinished();
    });
}

LanguageClientManager *LanguageClientManager::instance()
{
    return managerInstance;
}

QList<Client *> LanguageClientManager::clientsSupportingDocument(const TextEditor::TextDocument *doc)
{
    QTC_ASSERT(managerInstance, return {});
    QTC_ASSERT(doc, return {};);
    return Utils::filtered(managerInstance->reachableClients(), [doc](Client *client) {
        return client->isSupportedDocument(doc);
    });
}

void LanguageClientManager::applySettings()
{
    QTC_ASSERT(managerInstance, return);
    qDeleteAll(managerInstance->m_currentSettings);
    managerInstance->m_currentSettings
        = Utils::transform(LanguageClientSettings::pageSettings(), &BaseSettings::copy);
    const QList<BaseSettings *> restarts = LanguageClientSettings::changedSettings();
    LanguageClientSettings::toSettings(Core::ICore::settings(), managerInstance->m_currentSettings);

    for (BaseSettings *setting : restarts) {
        QList<TextEditor::TextDocument *> documents;
        const QList<Client *> currentClients = clientsForSetting(setting);
        for (Client *client : currentClients) {
            documents << managerInstance->m_clientForDocument.keys(client);
            shutdownClient(client);
        }
        for (auto document : std::as_const(documents))
            managerInstance->m_clientForDocument.remove(document);
        if (!setting->isValid() || !setting->m_enabled)
            continue;
        switch (setting->m_startBehavior) {
        case BaseSettings::AlwaysOn: {
            Client *client = startClient(setting);
            for (TextEditor::TextDocument *document : std::as_const(documents))
                managerInstance->m_clientForDocument[document] = client;
            break;
        }
        case BaseSettings::RequiresFile: {
            const QList<Core::IDocument *> &openedDocuments = Core::DocumentModel::openedDocuments();
            for (Core::IDocument *document : openedDocuments) {
                if (auto textDocument = qobject_cast<TextEditor::TextDocument *>(document)) {
                    if (setting->m_languageFilter.isSupported(document))
                        documents << textDocument;
                }
            }
            if (!documents.isEmpty()) {
                Client *client = startClient(setting);
                for (TextEditor::TextDocument *document : std::as_const(documents))
                    client->openDocument(document);
            }
            break;
        }
        case BaseSettings::RequiresProject: {
            const QList<Core::IDocument *> &openedDocuments = Core::DocumentModel::openedDocuments();
            QHash<ProjectExplorer::Project *, Client *> clientForProject;
            for (Core::IDocument *document : openedDocuments) {
                auto textDocument = qobject_cast<TextEditor::TextDocument *>(document);
                if (!textDocument || !setting->m_languageFilter.isSupported(textDocument))
                    continue;
                const Utils::FilePath filePath = textDocument->filePath();
                for (ProjectExplorer::Project *project :
                     ProjectExplorer::ProjectManager::projects()) {
                    if (project->isKnownFile(filePath)) {
                        Client *client = clientForProject[project];
                        if (!client) {
                            client = startClient(setting, project);
                            if (!client)
                                continue;
                            clientForProject[project] = client;
                        }
                        client->openDocument(textDocument);
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }
}

QList<BaseSettings *> LanguageClientManager::currentSettings()
{
    QTC_ASSERT(managerInstance, return {});
    return managerInstance->m_currentSettings;
}

void LanguageClientManager::registerClientSettings(BaseSettings *settings)
{
    QTC_ASSERT(managerInstance, return);
    LanguageClientSettings::addSettings(settings);
    managerInstance->applySettings();
}

void LanguageClientManager::enableClientSettings(const QString &settingsId, bool enable)
{
    QTC_ASSERT(managerInstance, return);
    LanguageClientSettings::enableSettings(settingsId, enable);
    managerInstance->applySettings();
}

QList<Client *> LanguageClientManager::clientsForSetting(const BaseSettings *setting)
{
    QTC_ASSERT(managerInstance, return {});
    auto instance = managerInstance;
    return instance->m_clientsForSetting.value(setting->m_id);
}

const BaseSettings *LanguageClientManager::settingForClient(Client *client)
{
    QTC_ASSERT(managerInstance, return nullptr);
    for (auto it = managerInstance->m_clientsForSetting.cbegin();
         it != managerInstance->m_clientsForSetting.cend(); ++it) {
        const QString &id = it.key();
        for (const Client *settingClient : it.value()) {
            if (settingClient == client) {
                return Utils::findOrDefault(managerInstance->m_currentSettings,
                                            [id](BaseSettings *setting) {
                                                return setting->m_id == id;
                                            });
            }
        }
    }
    return nullptr;
}

void LanguageClientManager::updateWorkspaceConfiguration(const ProjectExplorer::Project *project,
                                                         const QJsonValue &json)
{
    for (Client *client : managerInstance->m_clients) {
        ProjectExplorer::Project *clientProject = client->project();
        if (!clientProject || clientProject == project)
            client->updateConfiguration(json);
    }
}

Client *LanguageClientManager::clientForDocument(TextEditor::TextDocument *document)
{
    QTC_ASSERT(managerInstance, return nullptr);
    return document == nullptr ? nullptr
                               : managerInstance->m_clientForDocument.value(document).data();
}

Client *LanguageClientManager::clientForFilePath(const Utils::FilePath &filePath)
{
    return clientForDocument(TextEditor::TextDocument::textDocumentForFilePath(filePath));
}

const QList<Client *> LanguageClientManager::clientsForProject(
        const ProjectExplorer::Project *project)
{
    return Utils::filtered(managerInstance->m_clients, [project](const Client *c) {
        return c->project() == project;
    });
}

void LanguageClientManager::openDocumentWithClient(TextEditor::TextDocument *document, Client *client)
{
    if (!document)
        return;
    Client *currentClient = clientForDocument(document);
    if (client == currentClient)
        return;
    managerInstance->m_clientForDocument.remove(document);
    if (currentClient)
        currentClient->deactivateDocument(document);
    managerInstance->m_clientForDocument[document] = client;
    if (client) {
        qCDebug(Log) << "open" << document->filePath() << "with" << client->name() << client;
        if (!client->documentOpen(document))
            client->openDocument(document);
        else
            client->activateDocument(document);
    } else if (Core::EditorManager::currentDocument() == document) {
        TextEditor::IOutlineWidgetFactory::updateOutline();
    }
}

void LanguageClientManager::logJsonRpcMessage(const LspLogMessage::MessageSender sender,
                                              const QString &clientName,
                                              const LanguageServerProtocol::JsonRpcMessage &message)
{
    instance()->m_inspector.log(sender, clientName, message);
}

void LanguageClientManager::showInspector()
{
    QString clientName;
    if (Client *client = clientForDocument(TextEditor::TextDocument::currentTextDocument()))
        clientName = client->name();
    instance()->m_inspector.show(clientName);
}

QList<Client *> LanguageClientManager::reachableClients()
{
    return Utils::filtered(m_clients, &Client::reachable);
}

void LanguageClientManager::editorOpened(Core::IEditor *editor)
{
    using namespace TextEditor;
    using namespace Core;

    if (auto *textEditor = qobject_cast<BaseTextEditor *>(editor)) {
        if (TextEditorWidget *widget = textEditor->editorWidget()) {
            connect(widget, &TextEditorWidget::requestLinkAt, this,
                    [document = textEditor->textDocument()]
                    (const QTextCursor &cursor, const Utils::LinkHandler &callback, bool resolveTarget) {
                        if (auto client = clientForDocument(document)) {
                            client->findLinkAt(document,
                                               cursor,
                                               callback,
                                               resolveTarget,
                                               LinkTarget::SymbolDef);
                        }
                    });
            connect(widget, &TextEditorWidget::requestTypeAt, this,
                    [document = textEditor->textDocument()]
                    (const QTextCursor &cursor, const Utils::LinkHandler &callback, bool resolveTarget) {
                        if (auto client = clientForDocument(document)) {
                            client->findLinkAt(document,
                                               cursor,
                                               callback,
                                               resolveTarget,
                                               LinkTarget::SymbolTypeDef);
                        }
                    });
            connect(widget, &TextEditorWidget::requestUsages, this,
                    [document = textEditor->textDocument()](const QTextCursor &cursor) {
                        if (auto client = clientForDocument(document))
                            client->symbolSupport().findUsages(document, cursor);
                    });
            connect(widget, &TextEditorWidget::requestRename, this,
                    [document = textEditor->textDocument()](const QTextCursor &cursor) {
                        if (auto client = clientForDocument(document))
                            client->symbolSupport().renameSymbol(document, cursor);
                    });
            connect(widget, &TextEditorWidget::requestCallHierarchy, this,
                    [this, document = textEditor->textDocument()]() {
                        if (clientForDocument(document)) {
                            emit openCallHierarchy();
                            NavigationWidget::activateSubWidget(Constants::CALL_HIERARCHY_FACTORY_ID,
                                                                Side::Left);
                        }
                    });
            connect(widget, &TextEditorWidget::cursorPositionChanged, this, [widget]() {
                if (Client *client = clientForDocument(widget->textDocument()))
                    if (client->reachable())
                        client->cursorPositionChanged(widget);
            });
            if (TextEditor::TextDocument *document = textEditor->textDocument()) {
                if (Client *client = m_clientForDocument[document])
                    client->activateEditor(editor);
            }
        }
    }
}

void LanguageClientManager::documentOpened(Core::IDocument *document)
{
    auto textDocument = qobject_cast<TextEditor::TextDocument *>(document);
    if (!textDocument)
        return;

    // check whether we have to start servers for this document
    const QList<BaseSettings *> settings = currentSettings();
    for (BaseSettings *setting : settings) {
        if (setting->isValid() && setting->m_enabled
            && setting->m_languageFilter.isSupported(document)) {
            QList<Client *> clients = clientsForSetting(setting);
            if (setting->m_startBehavior == BaseSettings::RequiresProject) {
                const Utils::FilePath &filePath = document->filePath();
                for (ProjectExplorer::Project *project :
                     ProjectExplorer::ProjectManager::projects()) {
                    // check whether file is part of this project
                    if (!project->isKnownFile(filePath))
                        continue;

                    // check whether we already have a client running for this project
                    Client *clientForProject = Utils::findOrDefault(clients,
                                                                    [project](Client *client) {
                                                                        return client->project()
                                                                               == project;
                                                                    });
                    if (!clientForProject)
                        clientForProject = startClient(setting, project);

                    QTC_ASSERT(clientForProject, continue);
                    openDocumentWithClient(textDocument, clientForProject);
                    // Since we already opened the document in this client we remove the client
                    // from the list of clients that receive the openDocument call
                    clients.removeAll(clientForProject);
                }
            } else if (setting->m_startBehavior == BaseSettings::RequiresFile && clients.isEmpty()) {
                clients << startClient(setting);
            }
            for (auto client : std::as_const(clients))
                client->openDocument(textDocument);
        }
    }
}

void LanguageClientManager::documentClosed(Core::IDocument *document)
{
    if (auto textDocument = qobject_cast<TextEditor::TextDocument *>(document))
        m_clientForDocument.remove(textDocument);
}

void LanguageClientManager::updateProject(ProjectExplorer::Project *project)
{
    for (BaseSettings *setting : std::as_const(m_currentSettings)) {
        if (setting->isValid()
            && setting->m_enabled
            && setting->m_startBehavior == BaseSettings::RequiresProject) {
            if (Utils::findOrDefault(clientsForSetting(setting),
                                     [project](const QPointer<Client> &client) {
                                         return client->project() == project;
                                     })
                == nullptr) {
                Client *newClient = nullptr;
                const QList<Core::IDocument *> &openedDocuments = Core::DocumentModel::openedDocuments();
                for (Core::IDocument *doc : openedDocuments) {
                    if (setting->m_languageFilter.isSupported(doc)
                        && project->isKnownFile(doc->filePath())) {
                        if (auto textDoc = qobject_cast<TextEditor::TextDocument *>(doc)) {
                            if (!newClient)
                                newClient = startClient(setting, project);
                            if (!newClient)
                                break;
                            newClient->openDocument(textDoc);
                        }
                    }
                }
            }
        }
    }
}

void LanguageClientManager::projectAdded(ProjectExplorer::Project *project)
{
    connect(project, &ProjectExplorer::Project::fileListChanged, this, [this, project]() {
        updateProject(project);
    });
    const QList<Client *> &clients = reachableClients();
    for (Client *client : clients)
        client->projectOpened(project);
}

void LanguageClientManager::trackClientDeletion(Client *client)
{
    QTC_ASSERT(!m_scheduledForDeletion.contains(client->id()), return);
    m_scheduledForDeletion.insert(client->id());
    connect(client, &QObject::destroyed, this, [this, id = client->id()] {
        m_scheduledForDeletion.remove(id);
        if (isShutdownFinished())
            emit shutdownFinished();
    });
}

bool LanguageClientManager::isShutdownFinished()
{
    if (!PluginManager::isShuttingDown())
        return false;
    QTC_ASSERT(managerInstance, return true);
    return managerInstance->m_clients.isEmpty()
           && managerInstance->m_scheduledForDeletion.isEmpty();
}

} // namespace LanguageClient
