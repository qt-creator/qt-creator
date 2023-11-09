// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"
#include "languageclientsymbolsupport.h"
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

namespace Utils { namespace Text { class Range; } }

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
class LanguageClientOutlineItem;
class LanguageClientQuickFixProvider;
class LanguageFilter;
class ProgressManager;

class LANGUAGECLIENT_EXPORT Client : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Client)

public:
    explicit Client(BaseClientInterface *clientInterface, const Utils::Id &id = {}); // takes ownership
     ~Client() override;

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
        FailedToInitialize,
        Initialized,
        ShutdownRequested,
        Shutdown,
        Error
    };
    State state() const;
    QString stateString() const;
    bool reachable() const;
    void resetRestartCounter();

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
    void closeDocument(TextEditor::TextDocument *document,
                       const std::optional<Utils::FilePath> &overwriteFilePath = {});
    void activateDocument(TextEditor::TextDocument *document);
    void activateEditor(Core::IEditor *editor);
    void deactivateDocument(TextEditor::TextDocument *document);
    bool documentOpen(const TextEditor::TextDocument *document) const;
    TextEditor::TextDocument *documentForFilePath(const Utils::FilePath &file) const;
    void setShadowDocument(const Utils::FilePath &filePath, const QString &contents);
    void removeShadowDocument(const Utils::FilePath &filePath);
    void documentContentsSaved(TextEditor::TextDocument *document);
    void documentWillSave(Core::IDocument *document);
    void documentContentsChanged(TextEditor::TextDocument *document,
                                 int position,
                                 int charsRemoved,
                                 int charsAdded);
    void cursorPositionChanged(TextEditor::TextEditorWidget *widget);
    bool documentUpdatePostponed(const Utils::FilePath &fileName) const;
    int documentVersion(const Utils::FilePath &filePath) const;
    int documentVersion(const LanguageServerProtocol::DocumentUri &uri) const;
    void setDocumentChangeUpdateThreshold(int msecs);

    // workspace control
    virtual void setCurrentProject(ProjectExplorer::Project *project);
    ProjectExplorer::Project *project() const;
    virtual void projectOpened(ProjectExplorer::Project *project);
    virtual void projectClosed(ProjectExplorer::Project *project);
    virtual bool canOpenProject(ProjectExplorer::Project *project);
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
    // In contrast to the findLinkAt of symbol support this find link makes sure that there is only
    // one request running at a time and cancels the running request if the document changes, cursor
    // moves or another link is requested
    void findLinkAt(TextEditor::TextDocument *document,
                    const QTextCursor &cursor,
                    Utils::LinkHandler callback,
                    const bool resolveTarget,
                    LinkTarget target);
    DocumentSymbolCache *documentSymbolCache();
    HoverHandler *hoverHandler();
    QList<LanguageServerProtocol::Diagnostic> diagnosticsAt(const Utils::FilePath &filePath,
                                                            const QTextCursor &cursor) const;
    bool hasDiagnostic(const Utils::FilePath &filePath,
                       const LanguageServerProtocol::Diagnostic &diag) const;
    bool hasDiagnostics(const TextEditor::TextDocument *document) const;
    void setSemanticTokensHandler(const SemanticTokensHandler &handler);
    void setSnippetsGroup(const QString &group);
    void setCompletionAssistProvider(LanguageClientCompletionAssistProvider *provider);
    void setQuickFixAssistProvider(LanguageClientQuickFixProvider *provider);
    virtual bool supportsDocumentSymbols(const TextEditor::TextDocument *doc) const;
    virtual bool fileBelongsToProject(const Utils::FilePath &filePath) const;
    virtual LanguageClientOutlineItem *createOutlineItem(
        const LanguageServerProtocol::DocumentSymbol &symbol);

    LanguageServerProtocol::DocumentUri::PathMapper hostPathMapper() const;
    Utils::FilePath serverUriToHostPath(const LanguageServerProtocol::DocumentUri &uri) const;
    LanguageServerProtocol::DocumentUri hostPathToServerUri(const Utils::FilePath &path) const;
    Utils::OsType osType() const;

    // custom methods
    using CustomMethodHandler = std::function<void(
        const LanguageServerProtocol::JsonRpcMessage &message)>;
    void registerCustomMethod(const QString &method, const CustomMethodHandler &handler);

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

    void setCompletionResultsLimit(int limit);
    int completionResultsLimit() const;

signals:
    void initialized(const LanguageServerProtocol::ServerCapabilities &capabilities);
    void capabilitiesChanged(const DynamicCapabilities &capabilities);
    void documentUpdated(TextEditor::TextDocument *document);
    void workDone(const LanguageServerProtocol::ProgressToken &token);
    void shadowDocumentSwitched(const Utils::FilePath &filePath);
    void finished();

protected:
    void setError(const QString &message);
    ProgressManager *progressManager();
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
    virtual bool referencesShadowFile(const TextEditor::TextDocument *doc,
                                      const Utils::FilePath &candidate);
    virtual QList<Utils::Text::Range> additionalDocumentHighlights(
        TextEditor::TextEditorWidget *, const QTextCursor &) { return {}; }
};

} // namespace LanguageClient
