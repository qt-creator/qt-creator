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

#include "languageclient_global.h"
#include "languageclientutils.h"
#include "semantichighlightsupport.h"

namespace Core { class IDocument; }
namespace ProjectExplorer { class Project; }
namespace TextEditor
{
class IAssistProcessor;
class TextDocument;
class TextEditorWidget;
}

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace LanguageServerProtocol {
class ClientCapabilities;
class ClientInfo;
class ProgressToken;
class PublishDiagnosticsParams;
class Registration;
class ServerCapabilities;
class Unregistration;
} // namespace LanguageServerProtocol

namespace LanguageClient {

class BaseClientInterface;
class ClientPrivate;
class DiagnosticManager;
class DocumentSymbolCache;
class DynamicCapabilities;
class HoverHandler;
class InterfaceController;
class LanguageClientCompletionAssistProvider;
class LanguageClientQuickFixProvider;
class LanguageFilter;
class SymbolSupport;

class LANGUAGECLIENT_EXPORT Client : public QObject
{
    Q_OBJECT

public:
    explicit Client(BaseClientInterface *clientInterface); // takes ownership
     ~Client() override;

    Client(const Client &) = delete;
    Client(Client &&) = delete;
    Client &operator=(const Client &) = delete;
    Client &operator=(Client &&) = delete;

    // basic properties
    Utils::Id id() const;
    void setName(const QString &name);
    QString name() const;

    enum class SendDocUpdates { Send, Ignore };
    void sendMessage(const LanguageServerProtocol::JsonRpcMessage &message,
                     SendDocUpdates sendUpdates = SendDocUpdates::Send,
                     Schedule semanticTokensSchedule = Schedule::Delayed);

    void cancelRequest(const LanguageServerProtocol::MessageId &id);

    // server state handling
    void start();
    void setInitializationOptions(const QJsonObject& initializationOptions);
    void initialize();
    bool reset();
    void shutdown();
    enum State {
        Uninitialized,
        InitializeRequested,
        Initialized,
        ShutdownRequested,
        Shutdown,
        Error
    };
    State state() const;
    QString stateString() const;
    bool reachable() const;

    void setClientInfo(const LanguageServerProtocol::ClientInfo &clientInfo);
    // capabilities
    static LanguageServerProtocol::ClientCapabilities defaultClientCapabilities();
    void setClientCapabilities(const LanguageServerProtocol::ClientCapabilities &caps);
    const LanguageServerProtocol::ServerCapabilities &capabilities() const;
    QString serverName() const;
    QString serverVersion() const;
    const DynamicCapabilities &dynamicCapabilities() const;
    void registerCapabilities(const QList<LanguageServerProtocol::Registration> &registrations);
    void unregisterCapabilities(const QList<LanguageServerProtocol::Unregistration> &unregistrations);

    void setLocatorsEnabled(bool enabled);
    bool locatorsEnabled() const;
    void setAutoRequestCodeActions(bool enabled);

    // document synchronization
    void setSupportedLanguage(const LanguageFilter &filter);
    void setActivateDocumentAutomatically(bool enabled);
    bool isSupportedDocument(const TextEditor::TextDocument *document) const;
    bool isSupportedFile(const Utils::FilePath &filePath, const QString &mimeType) const;
    bool isSupportedUri(const LanguageServerProtocol::DocumentUri &uri) const;
    virtual void openDocument(TextEditor::TextDocument *document);
    void closeDocument(TextEditor::TextDocument *document);
    void activateDocument(TextEditor::TextDocument *document);
    void deactivateDocument(TextEditor::TextDocument *document);
    bool documentOpen(const TextEditor::TextDocument *document) const;
    TextEditor::TextDocument *documentForFilePath(const Utils::FilePath &file) const;
    void documentContentsSaved(TextEditor::TextDocument *document);
    void documentWillSave(Core::IDocument *document);
    void documentContentsChanged(TextEditor::TextDocument *document,
                                 int position,
                                 int charsRemoved,
                                 int charsAdded);
    void cursorPositionChanged(TextEditor::TextEditorWidget *widget);
    bool documentUpdatePostponed(const Utils::FilePath &fileName) const;
    int documentVersion(const Utils::FilePath &filePath) const;
    void setDocumentChangeUpdateThreshold(int msecs);

    // workspace control
    virtual void setCurrentProject(ProjectExplorer::Project *project);
    ProjectExplorer::Project *project() const;
    virtual void projectOpened(ProjectExplorer::Project *project);
    virtual void projectClosed(ProjectExplorer::Project *project);
    void updateConfiguration(const QJsonValue &configuration);

    // commands
    void requestCodeActions(const LanguageServerProtocol::DocumentUri &uri,
                            const LanguageServerProtocol::Diagnostic &diagnostic);
    void requestCodeActions(const LanguageServerProtocol::DocumentUri &uri,
                            const QList<LanguageServerProtocol::Diagnostic> &diagnostics);
    void requestCodeActions(const LanguageServerProtocol::CodeActionRequest &request);
    void handleCodeActionResponse(const LanguageServerProtocol::CodeActionRequest::Response &response,
                                  const LanguageServerProtocol::DocumentUri &uri);
    virtual void executeCommand(const LanguageServerProtocol::Command &command);

    // language support
    void addAssistProcessor(TextEditor::IAssistProcessor *processor);
    void removeAssistProcessor(TextEditor::IAssistProcessor *processor);
    SymbolSupport &symbolSupport();
    DocumentSymbolCache *documentSymbolCache();
    HoverHandler *hoverHandler();
    QList<LanguageServerProtocol::Diagnostic> diagnosticsAt(
        const LanguageServerProtocol::DocumentUri &uri,
        const QTextCursor &cursor) const;
    bool hasDiagnostic(const LanguageServerProtocol::DocumentUri &uri,
                       const LanguageServerProtocol::Diagnostic &diag) const;
    bool hasDiagnostics(const TextEditor::TextDocument *document) const;
    void setSemanticTokensHandler(const SemanticTokensHandler &handler);
    void setSymbolStringifier(const LanguageServerProtocol::SymbolStringifier &stringifier);
    LanguageServerProtocol::SymbolStringifier symbolStringifier() const;
    void setSnippetsGroup(const QString &group);
    void setCompletionAssistProvider(LanguageClientCompletionAssistProvider *provider);
    void setQuickFixAssistProvider(LanguageClientQuickFixProvider *provider);
    virtual bool supportsDocumentSymbols(const TextEditor::TextDocument *doc) const;

    // logging
    enum class LogTarget { Console, Ui };
    void setLogTarget(LogTarget target);
    void log(const QString &message) const;
    template<typename Error>
    void log(const LanguageServerProtocol::ResponseError<Error> &responseError) const
    { log(responseError.toString()); }

    // Caller takes ownership.
    using CustomInspectorTab = std::pair<QWidget *, QString>;
    using CustomInspectorTabs = QList<CustomInspectorTab>;
    virtual const CustomInspectorTabs createCustomInspectorTabs() { return {}; }

    // Caller takes ownership
    virtual TextEditor::RefactoringChangesData *createRefactoringChangesBackend() const;

signals:
    void initialized(const LanguageServerProtocol::ServerCapabilities &capabilities);
    void capabilitiesChanged(const DynamicCapabilities &capabilities);
    void documentUpdated(TextEditor::TextDocument *document);
    void workDone(const LanguageServerProtocol::ProgressToken &token);
    void finished();

protected:
    void setError(const QString &message);
    void setProgressTitleForToken(const LanguageServerProtocol::ProgressToken &token,
                                  const QString &message);
    void handleMessage(const LanguageServerProtocol::JsonRpcMessage &message);
    virtual void handleDiagnostics(const LanguageServerProtocol::PublishDiagnosticsParams &params);
    virtual DiagnosticManager *createDiagnosticManager();

private:
    friend class ClientPrivate;
    ClientPrivate *d = nullptr;

    virtual void handleDocumentClosed(TextEditor::TextDocument *) {}
    virtual void handleDocumentOpened(TextEditor::TextDocument *) {}
    virtual QTextCursor adjustedCursorForHighlighting(const QTextCursor &cursor,
                                                      TextEditor::TextDocument *doc);
};

} // namespace LanguageClient
