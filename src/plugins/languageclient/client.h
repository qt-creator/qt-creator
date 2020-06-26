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

#include "documentsymbolcache.h"
#include "dynamiccapabilities.h"
#include "languageclient_global.h"
#include "languageclientcompletionassist.h"
#include "languageclientformatter.h"
#include "languageclientfunctionhint.h"
#include "languageclienthoverhandler.h"
#include "languageclientquickfix.h"
#include "languageclientsettings.h"
#include "languageclientsymbolsupport.h"

#include <coreplugin/messagemanager.h>

#include <utils/id.h>
#include <utils/link.h>

#include <languageserverprotocol/client.h>
#include <languageserverprotocol/diagnostics.h>
#include <languageserverprotocol/initializemessages.h>
#include <languageserverprotocol/languagefeatures.h>
#include <languageserverprotocol/messages.h>
#include <languageserverprotocol/shutdownmessages.h>
#include <languageserverprotocol/textsynchronization.h>

#include <texteditor/semantichighlighter.h>

#include <QBuffer>
#include <QHash>
#include <QProcess>
#include <QJsonDocument>
#include <QTextCursor>

namespace Core { class IDocument; }
namespace ProjectExplorer { class Project; }
namespace TextEditor
{
class IAssistProcessor;
class TextDocument;
class TextEditorWidget;
class TextMark;
}

namespace LanguageClient {

class BaseClientInterface;
class TextMark;

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
    void openDocument(TextEditor::TextDocument *document);
    void closeDocument(TextEditor::TextDocument *document);
    void activateDocument(TextEditor::TextDocument *document);
    void deactivateDocument(TextEditor::TextDocument *document);
    bool documentOpen(const TextEditor::TextDocument *document) const;
    void documentContentsSaved(TextEditor::TextDocument *document);
    void documentWillSave(Core::IDocument *document);
    void documentContentsChanged(TextEditor::TextDocument *document,
                                 int position,
                                 int charsRemoved,
                                 int charsAdded);
    void registerCapabilities(const QList<LanguageServerProtocol::Registration> &registrations);
    void unregisterCapabilities(const QList<LanguageServerProtocol::Unregistration> &unregistrations);
    void cursorPositionChanged(TextEditor::TextEditorWidget *widget);

    SymbolSupport &symbolSupport();

    void requestCodeActions(const LanguageServerProtocol::DocumentUri &uri,
                            const QList<LanguageServerProtocol::Diagnostic> &diagnostics);
    void requestCodeActions(const LanguageServerProtocol::CodeActionRequest &request);
    void handleCodeActionResponse(const LanguageServerProtocol::CodeActionRequest::Response &response,
                                  const LanguageServerProtocol::DocumentUri &uri);
    void executeCommand(const LanguageServerProtocol::Command &command);

    void formatFile(const TextEditor::TextDocument *document);
    void formatRange(const TextEditor::TextDocument *document, const QTextCursor &cursor);

    // workspace control
    void setCurrentProject(ProjectExplorer::Project *project);
    const ProjectExplorer::Project *project() const;
    void projectOpened(ProjectExplorer::Project *project);
    void projectClosed(ProjectExplorer::Project *project);
    void projectFileListChanged();

    void sendContent(const LanguageServerProtocol::IContent &content);
    void sendContent(const LanguageServerProtocol::DocumentUri &uri,
                     const LanguageServerProtocol::IContent &content);
    void cancelRequest(const LanguageServerProtocol::MessageId &id);

    void setSupportedLanguage(const LanguageFilter &filter);
    void setInitializationOptions(const QJsonObject& initializationOptions);
    bool isSupportedDocument(const TextEditor::TextDocument *document) const;
    bool isSupportedFile(const Utils::FilePath &filePath, const QString &mimeType) const;
    bool isSupportedUri(const LanguageServerProtocol::DocumentUri &uri) const;

    void addAssistProcessor(TextEditor::IAssistProcessor *processor);
    void removeAssistProcessor(TextEditor::IAssistProcessor *processor);

    void setName(const QString &name) { m_displayName = name; }
    QString name() const { return m_displayName; }

    Utils::Id id() const { return m_id; }

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

    void showDiagnostics(Core::IDocument *doc);
    void hideDiagnostics(TextEditor::TextDocument *doc);

    const LanguageServerProtocol::ServerCapabilities &capabilities() const;
    const DynamicCapabilities &dynamicCapabilities() const;
    const BaseClientInterface *clientInterface() const;
    DocumentSymbolCache *documentSymbolCache();
    HoverHandler *hoverHandler();
    void rehighlight();

    bool documentUpdatePostponed(const QString &fileName) const;

signals:
    void initialized(LanguageServerProtocol::ServerCapabilities capabilities);
    void documentUpdated(TextEditor::TextDocument *document);
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
    void handleSemanticHighlight(const LanguageServerProtocol::SemanticHighlightingParams &params);

    void initializeCallback(const LanguageServerProtocol::InitializeRequest::Response &initResponse);
    void shutDownCallback(const LanguageServerProtocol::ShutdownRequest::Response &shutdownResponse);
    bool sendWorkspceFolderChanges() const;
    void log(const LanguageServerProtocol::ShowMessageParams &message,
             Core::MessageManager::PrintToOutputPaneFlag flag = Core::MessageManager::NoModeSwitch);

    void showMessageBox(const LanguageServerProtocol::ShowMessageRequestParams &message,
                        const LanguageServerProtocol::MessageId &id);

    void showDiagnostics(const LanguageServerProtocol::DocumentUri &uri);
    void removeDiagnostics(const LanguageServerProtocol::DocumentUri &uri);
    void resetAssistProviders(TextEditor::TextDocument *document);
    void sendPostponedDocumentUpdates();

    using ContentHandler = std::function<void(const QByteArray &, QTextCodec *, QString &,
                                              LanguageServerProtocol::ResponseHandlers,
                                              LanguageServerProtocol::MethodHandler)>;

    State m_state = Uninitialized;
    QHash<LanguageServerProtocol::MessageId, LanguageServerProtocol::ResponseHandler> m_responseHandlers;
    QHash<QByteArray, ContentHandler> m_contentHandler;
    QString m_displayName;
    LanguageFilter m_languagFilter;
    QJsonObject m_initializationOptions;
    QMap<TextEditor::TextDocument *, QString> m_openedDocument;
    QMap<TextEditor::TextDocument *,
         QList<LanguageServerProtocol::DidChangeTextDocumentParams::TextDocumentContentChangeEvent>>
        m_documentsToUpdate;
    QTimer m_documentUpdateTimer;
    Utils::Id m_id;
    LanguageServerProtocol::ServerCapabilities m_serverCapabilities;
    DynamicCapabilities m_dynamicCapabilities;
    struct AssistProviders
    {
        QPointer<TextEditor::CompletionAssistProvider> completionAssistProvider;
        QPointer<TextEditor::CompletionAssistProvider> functionHintProvider;
        QPointer<TextEditor::IAssistProvider> quickFixAssistProvider;
    };

    AssistProviders m_clientProviders;
    QMap<TextEditor::TextDocument *, AssistProviders> m_resetAssistProvider;
    QHash<LanguageServerProtocol::DocumentUri, LanguageServerProtocol::MessageId> m_highlightRequests;
    int m_restartsLeft = 5;
    QScopedPointer<BaseClientInterface> m_clientInterface;
    QMap<LanguageServerProtocol::DocumentUri, QList<LanguageServerProtocol::Diagnostic>> m_diagnostics;
    DocumentSymbolCache m_documentSymbolCache;
    HoverHandler m_hoverHandler;
    QHash<LanguageServerProtocol::DocumentUri, TextEditor::HighlightingResults> m_highlights;
    const ProjectExplorer::Project *m_project = nullptr;
    QSet<TextEditor::IAssistProcessor *> m_runningAssistProcessors;
    SymbolSupport m_symbolSupport;
};

} // namespace LanguageClient
