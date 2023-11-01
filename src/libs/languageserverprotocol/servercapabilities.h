// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "lsptypes.h"
#include "semantictokens.h"

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT WorkDoneProgressOptions : public JsonObject
{
public:
    using JsonObject::JsonObject;

    std::optional<bool> workDoneProgress() const { return optionalValue<bool>(workDoneProgressKey); }
    void setWorkDoneProgress(bool workDoneProgress) { insert(workDoneProgressKey, workDoneProgress); }
    void clearWorkDoneProgress() { remove(workDoneProgressKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ResolveProviderOption : public JsonObject
{
public:
    using JsonObject::JsonObject;

    std::optional<bool> resolveProvider() const { return optionalValue<bool>(resolveProviderKey); }
    void setResolveProvider(bool resolveProvider) { insert(resolveProviderKey, resolveProvider); }
    void clearResolveProvider() { remove(resolveProviderKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT TextDocumentRegistrationOptions : public JsonObject
{
public:
    using JsonObject::JsonObject;

    LanguageClientArray<DocumentFilter> documentSelector() const
    { return clientArray<DocumentFilter>(documentSelectorKey); }
    void setDocumentSelector(const LanguageClientArray<DocumentFilter> &documentSelector)
    { insert(documentSelectorKey, documentSelector.toJson()); }

    bool filterApplies(const Utils::FilePath &fileName,
                       const Utils::MimeType &mimeType = Utils::MimeType()) const;

    bool isValid() const override { return contains(documentSelectorKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT SaveOptions : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // The client is supposed to include the content on save.
    std::optional<bool> includeText() const { return optionalValue<bool>(includeTextKey); }
    void setIncludeText(bool includeText) { insert(includeTextKey, includeText); }
    void clearIncludeText() { remove(includeTextKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT TextDocumentSyncOptions : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // Open and close notifications are sent to the server.
    std::optional<bool> openClose() const { return optionalValue<bool>(openCloseKey); }
    void setOpenClose(bool openClose) { insert(openCloseKey, openClose); }
    void clearOpenClose() { remove(openCloseKey); }

    // Change notifications are sent to the server. See TextDocumentSyncKind.None,
    // TextDocumentSyncKind.Full and TextDocumentSyncKind.Incremental.
    std::optional<int> change() const { return optionalValue<int>(changeKey); }
    void setChange(int change) { insert(changeKey, change); }
    void clearChange() { remove(changeKey); }

    // Will save notifications are sent to the server.
    std::optional<bool> willSave() const { return optionalValue<bool>(willSaveKey); }
    void setWillSave(bool willSave) { insert(willSaveKey, willSave); }
    void clearWillSave() { remove(willSaveKey); }

    // Will save wait until requests are sent to the server.
    std::optional<bool> willSaveWaitUntil() const
    { return optionalValue<bool>(willSaveWaitUntilKey); }
    void setWillSaveWaitUntil(bool willSaveWaitUntil)
    { insert(willSaveWaitUntilKey, willSaveWaitUntil); }
    void clearWillSaveWaitUntil() { remove(willSaveWaitUntilKey); }

    // Save notifications are sent to the server.
    std::optional<SaveOptions> save() const { return optionalValue<SaveOptions>(saveKey); }
    void setSave(const SaveOptions &save) { insert(saveKey, save); }
    void clearSave() { remove(saveKey); }
};

enum class TextDocumentSyncKind
{
    // Documents should not be synced at all.
    None = 0,
    // Documents are synced by always sending the full content of the document.
    Full = 1,
    // Documents are synced by sending the full content on open.
    // After that only incremental updates to the document are send.
    Incremental = 2
};

class LANGUAGESERVERPROTOCOL_EXPORT CodeActionOptions : public WorkDoneProgressOptions
{
public:
    using WorkDoneProgressOptions::WorkDoneProgressOptions;

    QList<QString> codeActionKinds() const { return array<QString>(codeActionKindsKey); }
    void setCodeActionKinds(const QList<QString> &codeActionKinds)
    { insertArray(codeActionKindsKey, codeActionKinds); }

    bool isValid() const override;
};

enum class SemanticRequestType {
    None = 0x0,
    Full = 0x1,
    FullDelta = 0x2,
    Range = 0x4
};
Q_DECLARE_FLAGS(SemanticRequestTypes, SemanticRequestType)

class LANGUAGESERVERPROTOCOL_EXPORT SemanticTokensOptions : public WorkDoneProgressOptions
{
public:
    using WorkDoneProgressOptions::WorkDoneProgressOptions;

    /// The legend used by the server
    SemanticTokensLegend legend() const { return typedValue<SemanticTokensLegend>(legendKey); }
    void setLegend(const SemanticTokensLegend &legend) { insert(legendKey, legend); }

    /// Server supports providing semantic tokens for a specific range of a document.
    std::optional<std::variant<bool, QJsonObject>> range() const;
    void setRange(const std::variant<bool, QJsonObject> &range);
    void clearRange() { remove(rangeKey); }

    class FullSemanticTokenOptions : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        /// The server supports deltas for full documents.
        std::optional<bool> delta() const { return optionalValue<bool>(deltaKey); }
        void setDelta(bool delta) { insert(deltaKey, delta); }
        void clearDelta() { remove(deltaKey); }
    };

    /// Server supports providing semantic tokens for a full document.
    std::optional<std::variant<bool, FullSemanticTokenOptions>> full() const;
    void setFull(const std::variant<bool, FullSemanticTokenOptions> &full);
    void clearFull() { remove(fullKey); }

    bool isValid() const override { return contains(legendKey); }

    SemanticRequestTypes supportedRequests() const;
};

class LANGUAGESERVERPROTOCOL_EXPORT ServerCapabilities : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // Defines how the host (editor) should sync document changes to the language server.

    class LANGUAGESERVERPROTOCOL_EXPORT CompletionOptions : public WorkDoneProgressOptions
    {
    public:
        using WorkDoneProgressOptions::WorkDoneProgressOptions;

        // The characters that trigger completion automatically.
        std::optional<QList<QString>> triggerCharacters() const
        { return optionalArray<QString>(triggerCharactersKey); }
        void setTriggerCharacters(const QList<QString> &triggerCharacters)
        { insertArray(triggerCharactersKey, triggerCharacters); }
        void clearTriggerCharacters() { remove(triggerCharactersKey); }

        std::optional<bool> resolveProvider() const { return optionalValue<bool>(resolveProviderKey); }
        void setResolveProvider(bool resolveProvider) { insert(resolveProviderKey, resolveProvider); }
        void clearResolveProvider() { remove(resolveProviderKey); }
    };

    class LANGUAGESERVERPROTOCOL_EXPORT SignatureHelpOptions : public WorkDoneProgressOptions
    {
    public:
        using WorkDoneProgressOptions::WorkDoneProgressOptions;

        // The characters that trigger signature help automatically.
        std::optional<QList<QString>> triggerCharacters() const
        { return optionalArray<QString>(triggerCharactersKey); }
        void setTriggerCharacters(const QList<QString> &triggerCharacters)
        { insertArray(triggerCharactersKey, triggerCharacters); }
        void clearTriggerCharacters() { remove(triggerCharactersKey); }
    };

    using CodeLensOptions = ResolveProviderOption;

    class LANGUAGESERVERPROTOCOL_EXPORT DocumentOnTypeFormattingOptions : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        // A character on which formatting should be triggered, like `}`.
        QString firstTriggerCharacter() const { return typedValue<QString>(firstTriggerCharacterKey); }
        void setFirstTriggerCharacter(QString firstTriggerCharacter)
        { insert(firstTriggerCharacterKey, firstTriggerCharacter); }

        // More trigger characters.
        std::optional<QList<QString>> moreTriggerCharacter() const
        { return optionalArray<QString>(moreTriggerCharacterKey); }
        void setMoreTriggerCharacter(const QList<QString> &moreTriggerCharacter)
        { insertArray(moreTriggerCharacterKey, moreTriggerCharacter); }
        void clearMoreTriggerCharacter() { remove(moreTriggerCharacterKey); }

        bool isValid() const override { return contains(firstTriggerCharacterKey); }
    };

    using DocumentLinkOptions = ResolveProviderOption;

    class LANGUAGESERVERPROTOCOL_EXPORT ExecuteCommandOptions : public WorkDoneProgressOptions
    {
    public:
        using WorkDoneProgressOptions::WorkDoneProgressOptions;

        QList<QString> commands() const { return array<QString>(commandsKey); }
        void setCommands(const QList<QString> &commands) { insertArray(commandsKey, commands); }

        bool isValid() const override;
    };

    using ColorProviderOptions = JsonObject;

    class LANGUAGESERVERPROTOCOL_EXPORT StaticRegistrationOptions : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        // The id used to register the request. The id can be used to deregister
        // the request again. See also Registration#id.
        std::optional<QString> id() const { return optionalValue<QString>(idKey); }
        void setId(const QString &id) { insert(idKey, id); }
        void clearId() { remove(idKey); }
    };

    // Defines how text documents are synced. Is either a detailed structure defining each
    // notification or for backwards compatibility the TextDocumentSyncKind number.
    using TextDocumentSync = std::variant<TextDocumentSyncOptions, int>;
    std::optional<TextDocumentSync> textDocumentSync() const;
    void setTextDocumentSync(const TextDocumentSync &textDocumentSync);
    void clearTextDocumentSync() { remove(textDocumentSyncKey); }

    TextDocumentSyncKind textDocumentSyncKindHelper();

    // The server provides hover support.
    std::optional<std::variant<bool, WorkDoneProgressOptions>> hoverProvider() const;
    void setHoverProvider(const std::variant<bool, WorkDoneProgressOptions> &hoverProvider);
    void clearHoverProvider() { remove(hoverProviderKey); }

    // The server provides completion support.
    std::optional<CompletionOptions> completionProvider() const
    { return optionalValue<CompletionOptions>(completionProviderKey); }
    void setCompletionProvider(const CompletionOptions &completionProvider)
    { insert(completionProviderKey, completionProvider); }
    void clearCompletionProvider() { remove(completionProviderKey); }

    // The server provides signature help support.
    std::optional<SignatureHelpOptions> signatureHelpProvider() const
    { return optionalValue<SignatureHelpOptions>(signatureHelpProviderKey); }
    void setSignatureHelpProvider(const SignatureHelpOptions &signatureHelpProvider)
    { insert(signatureHelpProviderKey, signatureHelpProvider); }
    void clearSignatureHelpProvider() { remove(signatureHelpProviderKey); }

    class LANGUAGESERVERPROTOCOL_EXPORT RegistrationOptions : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        LanguageClientArray<DocumentFilter> documentSelector() const
        { return clientArray<DocumentFilter>(documentSelectorKey); }
        void setDocumentSelector(const LanguageClientArray<DocumentFilter> &documentSelector)
        { insert(documentSelectorKey, documentSelector.toJson()); }

        bool filterApplies(const Utils::FilePath &fileName,
                           const Utils::MimeType &mimeType = Utils::MimeType()) const;

        // The id used to register the request. The id can be used to deregister
        // the request again. See also Registration#id.
        std::optional<QString> id() const { return optionalValue<QString>(idKey); }
        void setId(const QString &id) { insert(idKey, id); }
        void clearId() { remove(idKey); }

        bool isValid() const override { return contains(documentSelectorKey); }
    };

    // The server provides goto definition support.
    std::optional<std::variant<bool, RegistrationOptions>> definitionProvider() const;
    void setDefinitionProvider(const std::variant<bool, RegistrationOptions> &typeDefinitionProvider);
    void clearDefinitionProvider() { remove(typeDefinitionProviderKey); }

    // The server provides Goto Type Definition support.
    std::optional<std::variant<bool, RegistrationOptions>> typeDefinitionProvider() const;
    void setTypeDefinitionProvider(const std::variant<bool, RegistrationOptions> &typeDefinitionProvider);
    void clearTypeDefinitionProvider() { remove(typeDefinitionProviderKey); }

    // The server provides Goto Implementation support.
    std::optional<std::variant<bool, RegistrationOptions>> implementationProvider() const;
    void setImplementationProvider(const std::variant<bool, RegistrationOptions> &implementationProvider);
    void clearImplementationProvider() { remove(implementationProviderKey); }

    // The server provides find references support.
    std::optional<std::variant<bool, WorkDoneProgressOptions>> referencesProvider() const;
    void setReferencesProvider(const std::variant<bool, WorkDoneProgressOptions> &referencesProvider);
    void clearReferencesProvider() { remove(referencesProviderKey); }

    // The server provides document highlight support.
    std::optional<std::variant<bool, WorkDoneProgressOptions>> documentHighlightProvider() const;
    void setDocumentHighlightProvider(
        const std::variant<bool, WorkDoneProgressOptions> &documentHighlightProvider);
    void clearDocumentHighlightProvider() { remove(documentHighlightProviderKey); }

    // The server provides document symbol support.
    std::optional<std::variant<bool, WorkDoneProgressOptions>> documentSymbolProvider() const;
    void setDocumentSymbolProvider(std::variant<bool, WorkDoneProgressOptions> documentSymbolProvider);
    void clearDocumentSymbolProvider() { remove(documentSymbolProviderKey); }

    std::optional<SemanticTokensOptions> semanticTokensProvider() const;
    void setSemanticTokensProvider(const SemanticTokensOptions &semanticTokensProvider);
    void clearSemanticTokensProvider() { remove(semanticTokensProviderKey); }

    std::optional<std::variant<bool, WorkDoneProgressOptions>> callHierarchyProvider() const;
    void setCallHierarchyProvider(const std::variant<bool, WorkDoneProgressOptions> &callHierarchyProvider);
    void clearCallHierarchyProvider() { remove(callHierarchyProviderKey); }

    // The server provides workspace symbol support.
    std::optional<std::variant<bool, WorkDoneProgressOptions>> workspaceSymbolProvider() const;
    void setWorkspaceSymbolProvider(std::variant<bool, WorkDoneProgressOptions> workspaceSymbolProvider);
    void clearWorkspaceSymbolProvider() { remove(workspaceSymbolProviderKey); }

    // The server provides code actions.
    std::optional<std::variant<bool, CodeActionOptions>> codeActionProvider() const;
    void setCodeActionProvider(bool codeActionProvider)
    { insert(codeActionProviderKey, codeActionProvider); }
    void setCodeActionProvider(CodeActionOptions options)
    { insert(codeActionProviderKey, options); }
    void clearCodeActionProvider() { remove(codeActionProviderKey); }

    // The server provides code lens.
    std::optional<CodeLensOptions> codeLensProvider() const
    { return optionalValue<CodeLensOptions>(codeLensProviderKey); }
    void setCodeLensProvider(CodeLensOptions codeLensProvider)
    { insert(codeLensProviderKey, codeLensProvider); }
    void clearCodeLensProvider() { remove(codeLensProviderKey); }

    // The server provides document formatting.
    std::optional<std::variant<bool, WorkDoneProgressOptions>> documentFormattingProvider() const;
    void setDocumentFormattingProvider(
        const std::variant<bool, WorkDoneProgressOptions> &documentFormattingProvider);
    void clearDocumentFormattingProvider() { remove(documentFormattingProviderKey); }

    // The server provides document formatting on typing.
    std::optional<std::variant<bool, WorkDoneProgressOptions>> documentRangeFormattingProvider() const;
    void setDocumentRangeFormattingProvider(std::variant<bool, WorkDoneProgressOptions> documentRangeFormattingProvider);
    void clearDocumentRangeFormattingProvider() { remove(documentRangeFormattingProviderKey); }

    class LANGUAGESERVERPROTOCOL_EXPORT RenameOptions : public WorkDoneProgressOptions
    {
    public:
        using WorkDoneProgressOptions::WorkDoneProgressOptions;

        // Renames should be checked and tested before being executed.
        std::optional<bool> prepareProvider() const { return optionalValue<bool>(prepareProviderKey); }
        void setPrepareProvider(bool prepareProvider) { insert(prepareProviderKey, prepareProvider); }
        void clearPrepareProvider() { remove(prepareProviderKey); }
    };

    // The server provides rename support.
    std::optional<std::variant<RenameOptions, bool>> renameProvider() const;
    void setRenameProvider(std::variant<RenameOptions,bool> renameProvider);
    void clearRenameProvider() { remove(renameProviderKey); }

    // The server provides document link support.
    std::optional<DocumentLinkOptions> documentLinkProvider() const
    { return optionalValue<DocumentLinkOptions>(documentLinkProviderKey); }
    void setDocumentLinkProvider(const DocumentLinkOptions &documentLinkProvider)
    { insert(documentLinkProviderKey, documentLinkProvider); }
    void clearDocumentLinkProvider() { remove(documentLinkProviderKey); }

    // The server provides color provider support.
    std::optional<std::variant<bool, JsonObject>> colorProvider() const;
    void setColorProvider(std::variant<bool, JsonObject> colorProvider);
    void clearColorProvider() { remove(colorProviderKey); }

    // The server provides execute command support.
    std::optional<ExecuteCommandOptions> executeCommandProvider() const
    { return optionalValue<ExecuteCommandOptions>(executeCommandProviderKey); }
    void setExecuteCommandProvider(ExecuteCommandOptions executeCommandProvider)
    { insert(executeCommandProviderKey, executeCommandProvider); }
    void clearExecuteCommandProvider() { remove(executeCommandProviderKey); }

    class LANGUAGESERVERPROTOCOL_EXPORT WorkspaceServerCapabilities : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        class LANGUAGESERVERPROTOCOL_EXPORT WorkspaceFoldersCapabilities : public JsonObject
        {
        public:
            using JsonObject::JsonObject;

            // The server has support for workspace folders
            std::optional<bool> supported() const { return optionalValue<bool>(supportedKey); }
            void setSupported(bool supported) { insert(supportedKey, supported); }
            void clearSupported() { remove(supportedKey); }

            std::optional<std::variant<QString, bool>> changeNotifications() const;
            void setChangeNotifications(std::variant<QString, bool> changeNotifications);
            void clearChangeNotifications() { remove(changeNotificationsKey); }
        };

        std::optional<WorkspaceFoldersCapabilities> workspaceFolders() const
        { return optionalValue<WorkspaceFoldersCapabilities>(workspaceFoldersKey); }
        void setWorkspaceFolders(const WorkspaceFoldersCapabilities &workspaceFolders)
        { insert(workspaceFoldersKey, workspaceFolders); }
        void clearWorkspaceFolders() { remove(workspaceFoldersKey); }
    };

    std::optional<WorkspaceServerCapabilities> workspace() const
    { return optionalValue<WorkspaceServerCapabilities>(workspaceKey); }
    void setWorkspace(const WorkspaceServerCapabilities &workspace)
    { insert(workspaceKey, workspace); }
    void clearWorkspace() { remove(workspaceKey); }

    std::optional<JsonObject> experimental() const { return optionalValue<JsonObject>(experimentalKey); }
    void setExperimental(const JsonObject &experimental) { insert(experimentalKey, experimental); }
    void clearExperimental() { remove(experimentalKey); }
};

} // namespace LanguageClient
