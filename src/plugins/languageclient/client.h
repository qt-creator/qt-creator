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

#include "dynamiccapabilities.h"
#include "languageclientcompletionassist.h"
#include "languageclientquickfix.h"
#include "languageclientsettings.h"

#include <coreplugin/id.h>
#include <coreplugin/messagemanager.h>
#include <utils/link.h>

#include <languageserverprotocol/client.h>
#include <languageserverprotocol/diagnostics.h>
#include <languageserverprotocol/initializemessages.h>
#include <languageserverprotocol/languagefeatures.h>
#include <languageserverprotocol/messages.h>
#include <languageserverprotocol/shutdownmessages.h>
#include <languageserverprotocol/textsynchronization.h>

#include <QBuffer>
#include <QHash>
#include <QProcess>
#include <QJsonDocument>
#include <QTextCursor>

namespace Core { class IDocument; }
namespace ProjectExplorer { class Project; }
namespace TextEditor
{
class TextDocument;
class TextEditorWidget;
class TextMark;
}

namespace LanguageClient {

class BaseClientInterface;
class TextMark;

class Client : public QObject
{
    Q_OBJECT

public:
    explicit Client(BaseClientInterface *clientInterface); // takes ownership
     ~Client() override;

    Client(const Client &) = delete;
    Client(Client &&) = delete;
    Client &operator=(const Client &) = delete;
    Client &operator=(Client &&) = delete;

    enum State {
        Uninitialized,
        InitializeRequested,
        Initialized,
        ShutdownRequested,
        Shutdown,
        Error
    };

    void initialize();
    void shutdown();
    State state() const;
    bool reachable() const { return m_state == Initialized; }

    // document synchronization
    void openDocument(Core::IDocument *document);
    void closeDocument(const LanguageServerProtocol::DidCloseTextDocumentParams &params);
    bool documentOpen(const LanguageServerProtocol::DocumentUri &uri) const;
    void documentContentsSaved(Core::IDocument *document);
    void documentWillSave(Core::IDocument *document);
    void documentContentsChanged(Core::IDocument *document);
    void registerCapabilities(const QList<LanguageServerProtocol::Registration> &registrations);
    void unregisterCapabilities(const QList<LanguageServerProtocol::Unregistration> &unregistrations);
    bool findLinkAt(LanguageServerProtocol::GotoDefinitionRequest &request);
    bool findUsages(LanguageServerProtocol::FindReferencesRequest &request);
    void requestDocumentSymbols(TextEditor::TextDocument *document);
    void cursorPositionChanged(TextEditor::TextEditorWidget *widget);

    void requestCodeActions(const LanguageServerProtocol::DocumentUri &uri,
                            const QList<LanguageServerProtocol::Diagnostic> &diagnostics);
    void requestCodeActions(const LanguageServerProtocol::CodeActionRequest &request);
    void handleCodeActionResponse(const LanguageServerProtocol::CodeActionRequest::Response &response,
                                  const LanguageServerProtocol::DocumentUri &uri);
    void executeCommand(const LanguageServerProtocol::Command &command);

    // workspace control
    void projectOpened(ProjectExplorer::Project *project);
    void projectClosed(ProjectExplorer::Project *project);

    void sendContent(const LanguageServerProtocol::IContent &content);
    void sendContent(const LanguageServerProtocol::DocumentUri &uri,
                     const LanguageServerProtocol::IContent &content);
    void cancelRequest(const LanguageServerProtocol::MessageId &id);

    void setSupportedLanguage(const LanguageFilter &filter);
    bool isSupportedDocument(const Core::IDocument *document) const;
    bool isSupportedFile(const Utils::FileName &filePath, const QString &mimeType) const;
    bool isSupportedUri(const LanguageServerProtocol::DocumentUri &uri) const;

    void setName(const QString &name) { m_displayName = name; }
    QString name() const { return m_displayName; }

    Core::Id id() const { return m_id; }

    bool needsRestart(const BaseSettings *) const;

    QList<LanguageServerProtocol::Diagnostic> diagnosticsAt(
        const LanguageServerProtocol::DocumentUri &uri,
        const LanguageServerProtocol::Range &range) const;

    bool start();
    bool reset();

    void log(const QString &message,
             Core::MessageManager::PrintToOutputPaneFlag flag = Core::MessageManager::NoModeSwitch);
    template<typename Error>
    void log(const LanguageServerProtocol::ResponseError<Error> &responseError,
             Core::MessageManager::PrintToOutputPaneFlag flag = Core::MessageManager::NoModeSwitch)
    { log(responseError.toString(), flag); }

    const LanguageServerProtocol::ServerCapabilities &capabilities() const;
    const DynamicCapabilities &dynamicCapabilities() const;
    const BaseClientInterface *clientInterface() const;

signals:
    void initialized(LanguageServerProtocol::ServerCapabilities capabilities);
    void finished();

protected:
    void setError(const QString &message);
    void handleMessage(const LanguageServerProtocol::BaseMessage &message);

private:
    void handleResponse(const LanguageServerProtocol::MessageId &id, const QByteArray &content,
                        QTextCodec *codec);
    void handleMethod(const QString &method, LanguageServerProtocol::MessageId id,
                      const LanguageServerProtocol::IContent *content);

    void handleDiagnostics(const LanguageServerProtocol::PublishDiagnosticsParams &params);

    void intializeCallback(const LanguageServerProtocol::InitializeRequest::Response &initResponse);
    void shutDownCallback(const LanguageServerProtocol::ShutdownRequest::Response &shutdownResponse);
    bool sendWorkspceFolderChanges() const;
    void log(const LanguageServerProtocol::ShowMessageParams &message,
             Core::MessageManager::PrintToOutputPaneFlag flag = Core::MessageManager::NoModeSwitch);

    void showMessageBox(const LanguageServerProtocol::ShowMessageRequestParams &message,
                        const LanguageServerProtocol::MessageId &id);

    void showDiagnostics(const LanguageServerProtocol::DocumentUri &uri);
    void removeDiagnostics(const LanguageServerProtocol::DocumentUri &uri);

    using ContentHandler = std::function<void(const QByteArray &, QTextCodec *, QString &,
                                              LanguageServerProtocol::ResponseHandlers,
                                              LanguageServerProtocol::MethodHandler)>;

    State m_state = Uninitialized;
    QHash<LanguageServerProtocol::MessageId, LanguageServerProtocol::ResponseHandler> m_responseHandlers;
    QHash<QByteArray, ContentHandler> m_contentHandler;
    QString m_displayName;
    LanguageFilter m_languagFilter;
    QList<Utils::FileName> m_openedDocument;
    Core::Id m_id;
    LanguageServerProtocol::ServerCapabilities m_serverCapabilities;
    DynamicCapabilities m_dynamicCapabilities;
    LanguageClientCompletionAssistProvider m_completionProvider;
    LanguageClientQuickFixProvider m_quickFixProvider;
    QSet<TextEditor::TextDocument *> m_resetAssistProvider;
    QHash<LanguageServerProtocol::DocumentUri, LanguageServerProtocol::MessageId> m_highlightRequests;
    int m_restartsLeft = 5;
    QScopedPointer<BaseClientInterface> m_clientInterface;
    QMap<LanguageServerProtocol::DocumentUri, QList<TextMark *>> m_diagnostics;
};

} // namespace LanguageClient
