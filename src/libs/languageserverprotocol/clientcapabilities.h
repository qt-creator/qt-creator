// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "jsonkeys.h"
#include "lsptypes.h"
#include "semantictokens.h"

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT DynamicRegistrationCapabilities : public JsonObject
{
public:
    using JsonObject::JsonObject;

    std::optional<bool> dynamicRegistration() const
    {
        return optionalValue<bool>(dynamicRegistrationKey);
    }
    void setDynamicRegistration(bool dynamicRegistration) { insert(dynamicRegistrationKey, dynamicRegistration); }
    void clearDynamicRegistration() { remove(dynamicRegistrationKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT FullSemanticTokenOptions : public JsonObject
{
public:
    using JsonObject::JsonObject;

    /**
      * The client will send the `textDocument/semanticTokens/full/delta`
      * request if the server provides a corresponding handler.
      */
    std::optional<bool> delta() const { return optionalValue<bool>(deltaKey); }
    void setDelta(bool delta) { insert(deltaKey, delta); }
    void clearDelta() { remove(deltaKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT SemanticTokensClientCapabilities : public DynamicRegistrationCapabilities
{
public:
    using DynamicRegistrationCapabilities::DynamicRegistrationCapabilities;
    class LANGUAGESERVERPROTOCOL_EXPORT Requests : public JsonObject
    {
        /**
         * Which requests the client supports and might send to the server
         * depending on the server's capability. Please note that clients might not
         * show semantic tokens or degrade some of the user experience if a range
         * or full request is advertised by the client but not provided by the
         * server. If for example the client capability `requests.full` and
         * `request.range` are both set to true but the server only provides a
         * range provider the client might not render a minimap correctly or might
         * even decide to not show any semantic tokens at all.
         */
    public:
        using JsonObject::JsonObject;

        /**
         * The client will send the `textDocument/semanticTokens/range` request
         * if the server provides a corresponding handler.
         */
        std::optional<std::variant<bool, QJsonObject>> range() const;
        void setRange(const std::variant<bool, QJsonObject> &range);
        void clearRange() { remove(rangeKey); }

        /**
         * The client will send the `textDocument/semanticTokens/full` request
         * if the server provides a corresponding handler.
         */
        std::optional<std::variant<bool, FullSemanticTokenOptions>> full() const;
        void setFull(const std::variant<bool, FullSemanticTokenOptions> &full);
        void clearFull() { remove(fullKey); }
    };

    Requests requests() const { return typedValue<Requests>(requestsKey); }
    void setRequests(const Requests &requests) { insert(requestsKey, requests); }

    /// The token types that the client supports.
    QList<QString> tokenTypes() const { return array<QString>(tokenTypesKey); }
    void setTokenTypes(const QList<QString> &value) { insertArray(tokenTypesKey, value); }

    /// The token modifiers that the client supports.
    QList<QString> tokenModifiers() const { return array<QString>(tokenModifiersKey); }
    void setTokenModifiers(const QList<QString> &value) { insertArray(tokenModifiersKey, value); }

    /// The formats the clients supports.
    QList<QString> formats() const { return array<QString>(formatsKey); }
    void setFormats(const QList<QString> &value) { insertArray(formatsKey, value); }

    /// Whether the client supports tokens that can overlap each other.
    std::optional<bool> overlappingTokenSupport() const
    {
        return optionalValue<bool>(overlappingTokenSupportKey);
    }
    void setOverlappingTokenSupport(bool overlappingTokenSupport) { insert(overlappingTokenSupportKey, overlappingTokenSupport); }
    void clearOverlappingTokenSupport() { remove(overlappingTokenSupportKey); }

    /// Whether the client supports tokens that can span multiple lines.
    std::optional<bool> multiLineTokenSupport() const
    {
        return optionalValue<bool>(multiLineTokenSupportKey);
    }
    void setMultiLineTokenSupport(bool multiLineTokenSupport) { insert(multiLineTokenSupportKey, multiLineTokenSupport); }
    void clearMultiLineTokenSupport() { remove(multiLineTokenSupportKey); }

    bool isValid() const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT SymbolCapabilities : public DynamicRegistrationCapabilities
{
public:
    using DynamicRegistrationCapabilities::DynamicRegistrationCapabilities;

    class LANGUAGESERVERPROTOCOL_EXPORT SymbolKindCapabilities : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        /*
         * The symbol kind values the client supports. When this
         * property exists the client also guarantees that it will
         * handle values outside its set gracefully and falls back
         * to a default value when unknown.
         *
         * If this property is not present the client only supports
         * the symbol kinds from `File` to `Array` as defined in
         * the initial version of the protocol.
         */
        std::optional<QList<SymbolKind>> valueSet() const;
        void setValueSet(const QList<SymbolKind> &valueSet);
        void clearValueSet() { remove(valueSetKey); }
    };

    // Specific capabilities for the `SymbolKind` in the `workspace/symbol` request.
    std::optional<SymbolKindCapabilities> symbolKind() const
    { return optionalValue<SymbolKindCapabilities>(symbolKindKey); }
    void setSymbolKind(const SymbolKindCapabilities &symbolKind) { insert(symbolKindKey, symbolKind); }
    void clearSymbolKind() { remove(symbolKindKey); }

    std::optional<bool> hierarchicalDocumentSymbolSupport() const
    { return optionalValue<bool>(hierarchicalDocumentSymbolSupportKey); }
    void setHierarchicalDocumentSymbolSupport(bool hierarchicalDocumentSymbolSupport)
    { insert(hierarchicalDocumentSymbolSupportKey, hierarchicalDocumentSymbolSupport); }
    void clearHierachicalDocumentSymbolSupport() { remove(hierarchicalDocumentSymbolSupportKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT TextDocumentClientCapabilities : public JsonObject
{
public:
    using JsonObject::JsonObject;

    class LANGUAGESERVERPROTOCOL_EXPORT SynchronizationCapabilities : public DynamicRegistrationCapabilities
    {
    public:
        using DynamicRegistrationCapabilities::DynamicRegistrationCapabilities;

        // The client supports sending will save notifications.
        std::optional<bool> willSave() const { return optionalValue<bool>(willSaveKey); }
        void setWillSave(bool willSave) { insert(willSaveKey, willSave); }
        void clearWillSave() { remove(willSaveKey); }

        /*
         * The client supports sending a will save request and
         * waits for a response providing text edits which will
         * be applied to the document before it is saved.
         */
        std::optional<bool> willSaveWaitUntil() const
        { return optionalValue<bool>(willSaveWaitUntilKey); }
        void setWillSaveWaitUntil(bool willSaveWaitUntil)
        { insert(willSaveWaitUntilKey, willSaveWaitUntil); }
        void clearWillSaveWaitUntil() { remove(willSaveWaitUntilKey); }

        // The client supports did save notifications.
        std::optional<bool> didSave() const { return optionalValue<bool>(didSaveKey); }
        void setDidSave(bool didSave) { insert(didSaveKey, didSave); }
        void clearDidSave() { remove(didSaveKey); }
    };

    std::optional<SynchronizationCapabilities> synchronization() const
    { return optionalValue<SynchronizationCapabilities>(synchronizationKey); }
    void setSynchronization(const SynchronizationCapabilities &synchronization)
    { insert(synchronizationKey, synchronization); }
    void clearSynchronization() { remove(synchronizationKey); }

    class LANGUAGESERVERPROTOCOL_EXPORT CompletionCapabilities : public DynamicRegistrationCapabilities
    {
    public:
        using DynamicRegistrationCapabilities::DynamicRegistrationCapabilities;

        class LANGUAGESERVERPROTOCOL_EXPORT CompletionItemCapbilities : public JsonObject
        {
        public:
            using JsonObject::JsonObject;

            /*
             * Client supports snippets as insert text.
             *
             * A snippet can define tab stops and placeholders with `$1`, `$2`
             * and `${3:foo}`. `$0` defines the final tab stop, it defaults to
             * the end of the snippet. Placeholders with equal identifiers are linked,
             * that is typing in one will update others too.
             */
            std::optional<bool> snippetSupport() const
            { return optionalValue<bool>(snippetSupportKey); }
            void setSnippetSupport(bool snippetSupport)
            { insert(snippetSupportKey, snippetSupport); }
            void clearSnippetSupport() { remove(snippetSupportKey); }

            // Client supports commit characters on a completion item.
            std::optional<bool> commitCharacterSupport() const
            { return optionalValue<bool>(commitCharacterSupportKey); }
            void setCommitCharacterSupport(bool commitCharacterSupport)
            { insert(commitCharacterSupportKey, commitCharacterSupport); }
            void clearCommitCharacterSupport() { remove(commitCharacterSupportKey); }

            /*
             * Client supports the follow content formats for the documentation
             * property. The order describes the preferred format of the client.
             */
            std::optional<QList<MarkupKind>> documentationFormat() const;
            void setDocumentationFormat(const QList<MarkupKind> &documentationFormat);
            void clearDocumentationFormat() { remove(documentationFormatKey); }
        };

        // The client supports the following `CompletionItem` specific capabilities.
        std::optional<CompletionItemCapbilities> completionItem() const
        { return optionalValue<CompletionItemCapbilities>(completionItemKey); }
        void setCompletionItem(const CompletionItemCapbilities &completionItem)
        { insert(completionItemKey, completionItem); }
        void clearCompletionItem() { remove(completionItemKey); }

        class LANGUAGESERVERPROTOCOL_EXPORT CompletionItemKindCapabilities : public JsonObject
        {
        public:
            CompletionItemKindCapabilities();
            using JsonObject::JsonObject;
            /*
             * The completion item kind values the client supports. When this
             * property exists the client also guarantees that it will
             * handle values outside its set gracefully and falls back
             * to a default value when unknown.
             *
             * If this property is not present the client only supports
             * the completion items kinds from `Text` to `Reference` as defined in
             * the initial version of the protocol.
             */
            std::optional<QList<CompletionItemKind::Kind>> valueSet() const;
            void setValueSet(const QList<CompletionItemKind::Kind> &valueSet);
            void clearValueSet() { remove(valueSetKey); }
        };

        std::optional<CompletionItemKindCapabilities> completionItemKind() const
        { return optionalValue<CompletionItemKindCapabilities>(completionItemKindKey); }
        void setCompletionItemKind(const CompletionItemKindCapabilities &completionItemKind)
        { insert(completionItemKindKey, completionItemKind); }
        void clearCompletionItemKind() { remove(completionItemKindKey); }

        /*
         * The client supports to send additional context information for a
         * `textDocument/completion` request.
         */
        std::optional<bool> contextSupport() const
        {
            return optionalValue<bool>(contextSupportKey);
        }
        void setContextSupport(bool contextSupport) { insert(contextSupportKey, contextSupport); }
        void clearContextSupport() { remove(contextSupportKey); }
    };

    // Capabilities specific to the `textDocument/completion`
    std::optional<CompletionCapabilities> completion() const
    { return optionalValue<CompletionCapabilities>(completionKey); }
    void setCompletion(const CompletionCapabilities &completion)
    { insert(completionKey, completion); }
    void clearCompletion() { remove(completionKey); }

    class LANGUAGESERVERPROTOCOL_EXPORT HoverCapabilities : public DynamicRegistrationCapabilities
    {
    public:
        using DynamicRegistrationCapabilities::DynamicRegistrationCapabilities;
        /*
         * Client supports the follow content formats for the content
         * property. The order describes the preferred format of the client.
         */
        std::optional<QList<MarkupKind>> contentFormat() const;
        void setContentFormat(const QList<MarkupKind> &contentFormat);
        void clearContentFormat() { remove(contentFormatKey); }
    };

    std::optional<HoverCapabilities> hover() const
    {
        return optionalValue<HoverCapabilities>(hoverKey);
    }
    void setHover(const HoverCapabilities &hover) { insert(hoverKey, hover); }
    void clearHover() { remove(hoverKey); }

    class LANGUAGESERVERPROTOCOL_EXPORT SignatureHelpCapabilities : public DynamicRegistrationCapabilities
    {
    public:
        using DynamicRegistrationCapabilities::DynamicRegistrationCapabilities;

        class LANGUAGESERVERPROTOCOL_EXPORT SignatureInformationCapabilities : public JsonObject
        {
        public:
            using JsonObject::JsonObject;
            /*
             * Client supports the follow content formats for the documentation
             * property. The order describes the preferred format of the client.
             */
            std::optional<QList<MarkupKind>> documentationFormat() const;
            void setDocumentationFormat(const QList<MarkupKind> &documentationFormat);
            void clearDocumentationFormat() { remove(documentationFormatKey); }

            std::optional<bool> activeParameterSupport() const
            { return optionalValue<bool>(activeParameterSupportKey); }
            void setActiveParameterSupport(bool activeParameterSupport)
            { insert(activeParameterSupportKey, activeParameterSupport); }
            void clearActiveParameterSupport() { remove(activeParameterSupportKey); }
        };

        // The client supports the following `SignatureInformation` specific properties.
        std::optional<SignatureInformationCapabilities> signatureInformation() const
        { return optionalValue<SignatureInformationCapabilities>(signatureInformationKey); }
        void setSignatureInformation(const SignatureInformationCapabilities &signatureInformation)
        { insert(signatureInformationKey, signatureInformation); }
        void clearSignatureInformation() { remove(signatureInformationKey); }
    };

    // Capabilities specific to the `textDocument/signatureHelp`
    std::optional<SignatureHelpCapabilities> signatureHelp() const
    { return optionalValue<SignatureHelpCapabilities>(signatureHelpKey); }
    void setSignatureHelp(const SignatureHelpCapabilities &signatureHelp)
    { insert(signatureHelpKey, signatureHelp); }
    void clearSignatureHelp() { remove(signatureHelpKey); }

    // Whether references supports dynamic registration.
    std::optional<DynamicRegistrationCapabilities> references() const
    { return optionalValue<DynamicRegistrationCapabilities>(referencesKey); }
    void setReferences(const DynamicRegistrationCapabilities &references)
    { insert(referencesKey, references); }
    void clearReferences() { remove(referencesKey); }

    // Whether document highlight supports dynamic registration.
    std::optional<DynamicRegistrationCapabilities> documentHighlight() const
    { return optionalValue<DynamicRegistrationCapabilities>(documentHighlightKey); }
    void setDocumentHighlight(const DynamicRegistrationCapabilities &documentHighlight)
    { insert(documentHighlightKey, documentHighlight); }
    void clearDocumentHighlight() { remove(documentHighlightKey); }

    // Capabilities specific to the `textDocument/documentSymbol`
    std::optional<SymbolCapabilities> documentSymbol() const
    { return optionalValue<SymbolCapabilities>(documentSymbolKey); }
    void setDocumentSymbol(const SymbolCapabilities &documentSymbol)
    { insert(documentSymbolKey, documentSymbol); }
    void clearDocumentSymbol() { remove(documentSymbolKey); }

    // Whether formatting supports dynamic registration.
    std::optional<DynamicRegistrationCapabilities> formatting() const
    { return optionalValue<DynamicRegistrationCapabilities>(formattingKey); }
    void setFormatting(const DynamicRegistrationCapabilities &formatting)
    { insert(formattingKey, formatting); }
    void clearFormatting() { remove(formattingKey); }

    // Whether range formatting supports dynamic registration.
    std::optional<DynamicRegistrationCapabilities> rangeFormatting() const
    { return optionalValue<DynamicRegistrationCapabilities>(rangeFormattingKey); }
    void setRangeFormatting(const DynamicRegistrationCapabilities &rangeFormatting)
    { insert(rangeFormattingKey, rangeFormatting); }
    void clearRangeFormatting() { remove(rangeFormattingKey); }

    // Whether on type formatting supports dynamic registration.
    std::optional<DynamicRegistrationCapabilities> onTypeFormatting() const
    { return optionalValue<DynamicRegistrationCapabilities>(onTypeFormattingKey); }
    void setOnTypeFormatting(const DynamicRegistrationCapabilities &onTypeFormatting)
    { insert(onTypeFormattingKey, onTypeFormatting); }
    void clearOnTypeFormatting() { remove(onTypeFormattingKey); }

    // Whether definition supports dynamic registration.
    std::optional<DynamicRegistrationCapabilities> definition() const
    { return optionalValue<DynamicRegistrationCapabilities>(definitionKey); }
    void setDefinition(const DynamicRegistrationCapabilities &definition)
    { insert(definitionKey, definition); }
    void clearDefinition() { remove(definitionKey); }

    /*
     * Whether typeDefinition supports dynamic registration. If this is set to `true`
     * the client supports the new `(TextDocumentRegistrationOptions & StaticRegistrationOptions)`
     * return value for the corresponding server capability as well.
     */
    std::optional<DynamicRegistrationCapabilities> typeDefinition() const
    { return optionalValue<DynamicRegistrationCapabilities>(typeDefinitionKey); }
    void setTypeDefinition(const DynamicRegistrationCapabilities &typeDefinition)
    { insert(typeDefinitionKey, typeDefinition); }
    void clearTypeDefinition() { remove(typeDefinitionKey); }

    /*
     * Whether implementation supports dynamic registration. If this is set to `true`
     * the client supports the new `(TextDocumentRegistrationOptions & StaticRegistrationOptions)`
     * return value for the corresponding server capability as well.
     */
    std::optional<DynamicRegistrationCapabilities> implementation() const
    { return optionalValue<DynamicRegistrationCapabilities>(implementationKey); }
    void setImplementation(const DynamicRegistrationCapabilities &implementation)
    { insert(implementationKey, implementation); }
    void clearImplementation() { remove(implementationKey); }

    class LANGUAGESERVERPROTOCOL_EXPORT CodeActionCapabilities : public DynamicRegistrationCapabilities
    {
    public:
        using DynamicRegistrationCapabilities::DynamicRegistrationCapabilities;

        class LANGUAGESERVERPROTOCOL_EXPORT CodeActionLiteralSupport : public JsonObject
        {
        public:
            using JsonObject::JsonObject;

            class LANGUAGESERVERPROTOCOL_EXPORT CodeActionKind : public JsonObject
            {
            public:
                using JsonObject::JsonObject;
                CodeActionKind() : CodeActionKind(QList<QString>()) {}
                explicit CodeActionKind(const QList<QString> &kinds) { setValueSet(kinds); }

                QList<QString> valueSet() const { return array<QString>(valueSetKey); }
                void setValueSet(const QList<QString> &valueSet)
                { insertArray(valueSetKey, valueSet); }

                bool isValid() const override { return contains(valueSetKey); }
            };

            CodeActionKind codeActionKind() const
            { return typedValue<CodeActionKind>(codeActionKindKey); }
            void setCodeActionKind(const CodeActionKind &codeActionKind)
            { insert(codeActionKindKey, codeActionKind); }

            bool isValid() const override { return contains(codeActionKindKey); }
        };

        std::optional<CodeActionLiteralSupport> codeActionLiteralSupport() const
        { return optionalValue<CodeActionLiteralSupport>(codeActionLiteralSupportKey); }
        void setCodeActionLiteralSupport(const CodeActionLiteralSupport &codeActionLiteralSupport)
        { insert(codeActionLiteralSupportKey, codeActionLiteralSupport); }
        void clearCodeActionLiteralSupport() { remove(codeActionLiteralSupportKey); }
    };

    // Whether code action supports dynamic registration.
    std::optional<CodeActionCapabilities> codeAction() const
    { return optionalValue<CodeActionCapabilities>(codeActionKey); }
    void setCodeAction(const CodeActionCapabilities &codeAction)
    { insert(codeActionKey, codeAction); }
    void clearCodeAction() { remove(codeActionKey); }

    // Whether code lens supports dynamic registration.
    std::optional<DynamicRegistrationCapabilities> codeLens() const
    { return optionalValue<DynamicRegistrationCapabilities>(codeLensKey); }
    void setCodeLens(const DynamicRegistrationCapabilities &codeLens)
    { insert(codeLensKey, codeLens); }
    void clearCodeLens() { remove(codeLensKey); }

    // Whether document link supports dynamic registration.
    std::optional<DynamicRegistrationCapabilities> documentLink() const
    { return optionalValue<DynamicRegistrationCapabilities>(documentLinkKey); }
    void setDocumentLink(const DynamicRegistrationCapabilities &documentLink)
    { insert(documentLinkKey, documentLink); }
    void clearDocumentLink() { remove(documentLinkKey); }

    /*
     * Whether colorProvider supports dynamic registration. If this is set to `true`
     * the client supports the new `(ColorProviderOptions & TextDocumentRegistrationOptions & StaticRegistrationOptions)`
     * return value for the corresponding server capability as well.
     */
    std::optional<DynamicRegistrationCapabilities> colorProvider() const
    { return optionalValue<DynamicRegistrationCapabilities>(colorProviderKey); }
    void setColorProvider(const DynamicRegistrationCapabilities &colorProvider)
    { insert(colorProviderKey, colorProvider); }
    void clearColorProvider() { remove(colorProviderKey); }

    class LANGUAGESERVERPROTOCOL_EXPORT RenameClientCapabilities : public DynamicRegistrationCapabilities
    {
    public:
        using DynamicRegistrationCapabilities::DynamicRegistrationCapabilities;
        /**
         * Client supports testing for validity of rename operations
         * before execution.
         *
         * @since version 3.12.0
         */

        std::optional<bool> prepareSupport() const
        {
            return optionalValue<bool>(prepareSupportKey);
        }
        void setPrepareSupport(bool prepareSupport) { insert(prepareSupportKey, prepareSupport); }
        void clearPrepareSupport() { remove(prepareSupportKey); }
    };

    // Whether rename supports dynamic registration.
    std::optional<RenameClientCapabilities> rename() const
    { return optionalValue<RenameClientCapabilities>(renameKey); }
    void setRename(const RenameClientCapabilities &rename)
    { insert(renameKey, rename); }
    void clearRename() { remove(renameKey); }

    std::optional<SemanticTokensClientCapabilities> semanticTokens() const;
    void setSemanticTokens(const SemanticTokensClientCapabilities &semanticTokens);
    void clearSemanticTokens() { remove(semanticTokensKey); }

    std::optional<DynamicRegistrationCapabilities> callHierarchy() const
    { return optionalValue<DynamicRegistrationCapabilities>(callHierarchyKey); }
    void setCallHierarchy(const DynamicRegistrationCapabilities &callHierarchy)
    { insert(callHierarchyKey, callHierarchy); }
    void clearCallHierarchy() { remove(callHierarchyKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT SemanticTokensWorkspaceClientCapabilities : public JsonObject
{
public:
    using JsonObject::JsonObject;
    /**
     * Whether the client implementation supports a refresh request sent from
     * the server to the client.
     *
     * Note that this event is global and will force the client to refresh all
     * semantic tokens currently shown. It should be used with absolute care
     * and is useful for situation where a server for example detect a project
     * wide change that requires such a calculation.
     */
    std::optional<bool> refreshSupport() const { return optionalValue<bool>(refreshSupportKey); }
    void setRefreshSupport(bool refreshSupport) { insert(refreshSupportKey, refreshSupport); }
    void clearRefreshSupport() { remove(refreshSupportKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkspaceClientCapabilities : public JsonObject
{
public:
    WorkspaceClientCapabilities();
    using JsonObject::JsonObject;

    /*
     * The client supports applying batch edits to the workspace by supporting the request
     * 'workspace/applyEdit'
     */
    std::optional<bool> applyEdit() const { return optionalValue<bool>(applyEditKey); }
    void setApplyEdit(bool applyEdit) { insert(applyEditKey, applyEdit); }
    void clearApplyEdit() { remove(applyEditKey); }

    class LANGUAGESERVERPROTOCOL_EXPORT WorkspaceEditCapabilities : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        // The client supports versioned document changes in `WorkspaceEdit`s
        std::optional<bool> documentChanges() const
        { return optionalValue<bool>(documentChangesKey); }
        void setDocumentChanges(bool documentChanges)
        { insert(documentChangesKey, documentChanges); }
        void clearDocumentChanges() { remove(documentChangesKey); }

        enum class ResourceOperationKind { Create, Rename, Delete };

        // The resource operations the client supports. Clients should at least support 'create',
        // 'rename' and 'delete' files and folders.
        std::optional<QList<ResourceOperationKind>> resourceOperations() const;
        void setResourceOperations(const QList<ResourceOperationKind> &resourceOperations);
        void clearResourceOperations() { remove(resourceOperationsKey); }
    };

    // Capabilities specific to `WorkspaceEdit`s
    std::optional<WorkspaceEditCapabilities> workspaceEdit() const
    { return optionalValue<WorkspaceEditCapabilities>(workspaceEditKey); }
    void setWorkspaceEdit(const WorkspaceEditCapabilities &workspaceEdit)
    { insert(workspaceEditKey, workspaceEdit); }
    void clearWorkspaceEdit() { remove(workspaceEditKey); }

    // Capabilities specific to the `workspace/didChangeConfiguration` notification.
    std::optional<DynamicRegistrationCapabilities> didChangeConfiguration() const
    { return optionalValue<DynamicRegistrationCapabilities>(didChangeConfigurationKey); }
    void setDidChangeConfiguration(const DynamicRegistrationCapabilities &didChangeConfiguration)
    { insert(didChangeConfigurationKey, didChangeConfiguration); }
    void clearDidChangeConfiguration() { remove(didChangeConfigurationKey); }

    // Capabilities specific to the `workspace/didChangeWatchedFiles` notification.
    std::optional<DynamicRegistrationCapabilities> didChangeWatchedFiles() const
    { return optionalValue<DynamicRegistrationCapabilities>(didChangeWatchedFilesKey); }
    void setDidChangeWatchedFiles(const DynamicRegistrationCapabilities &didChangeWatchedFiles)
    { insert(didChangeWatchedFilesKey, didChangeWatchedFiles); }
    void clearDidChangeWatchedFiles() { remove(didChangeWatchedFilesKey); }

    // Specific capabilities for the `SymbolKind` in the `workspace/symbol` request.
    std::optional<SymbolCapabilities> symbol() const
    { return optionalValue<SymbolCapabilities>(symbolKey); }
    void setSymbol(const SymbolCapabilities &symbol) { insert(symbolKey, symbol); }
    void clearSymbol() { remove(symbolKey); }

    // Capabilities specific to the `workspace/executeCommand` request.
    std::optional<DynamicRegistrationCapabilities> executeCommand() const
    { return optionalValue<DynamicRegistrationCapabilities>(executeCommandKey); }
    void setExecuteCommand(const DynamicRegistrationCapabilities &executeCommand)
    { insert(executeCommandKey, executeCommand); }
    void clearExecuteCommand() { remove(executeCommandKey); }

    // The client has support for workspace folders. Since 3.6.0
    std::optional<bool> workspaceFolders() const
    { return optionalValue<bool>(workspaceFoldersKey); }
    void setWorkspaceFolders(bool workspaceFolders)
    { insert(workspaceFoldersKey, workspaceFolders); }
    void clearWorkspaceFolders() { remove(workspaceFoldersKey); }

    // The client supports `workspace/configuration` requests. Since 3.6.0
    std::optional<bool> configuration() const { return optionalValue<bool>(configurationKey); }
    void setConfiguration(bool configuration) { insert(configurationKey, configuration); }
    void clearConfiguration() { remove(configurationKey); }

    std::optional<SemanticTokensWorkspaceClientCapabilities> semanticTokens() const
    { return optionalValue<SemanticTokensWorkspaceClientCapabilities>(semanticTokensKey); }
    void setSemanticTokens(const SemanticTokensWorkspaceClientCapabilities &semanticTokens)
    { insert(semanticTokensKey, semanticTokens); }
    void clearSemanticTokens() { remove(semanticTokensKey); }
};

class WindowClientClientCapabilities : public JsonObject
{
public:
    using JsonObject::JsonObject;

    /**
      * Whether client supports handling progress notifications.
      * If set, servers are allowed to report in `workDoneProgress` property
      * in the request specific server capabilities.
      *
      */
    std::optional<bool> workDoneProgress() const
    { return optionalValue<bool>(workDoneProgressKey); }
    void setWorkDoneProgress(bool workDoneProgress)
    { insert(workDoneProgressKey, workDoneProgress); }
    void clearWorkDoneProgress() { remove(workDoneProgressKey); }

private:
    constexpr static const char workDoneProgressKey[] = "workDoneProgress";
};

class LANGUAGESERVERPROTOCOL_EXPORT ClientCapabilities : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // Workspace specific client capabilities.
    std::optional<WorkspaceClientCapabilities> workspace() const
    { return optionalValue<WorkspaceClientCapabilities>(workspaceKey); }
    void setWorkspace(const WorkspaceClientCapabilities &workspace)
    { insert(workspaceKey, workspace); }
    void clearWorkspace() { remove(workspaceKey); }

    // Text document specific client capabilities.
    std::optional<TextDocumentClientCapabilities> textDocument() const
    { return optionalValue<TextDocumentClientCapabilities>(textDocumentKey); }
    void setTextDocument(const TextDocumentClientCapabilities &textDocument)
    { insert(textDocumentKey, textDocument); }
    void clearTextDocument() { remove(textDocumentKey); }

    // Window specific client capabilities.
    std::optional<WindowClientClientCapabilities> window() const
    { return optionalValue<WindowClientClientCapabilities>(windowKey); }
    void setWindow(const WindowClientClientCapabilities &window)
    { insert(windowKey, window); }
    void clearWindow() { remove(windowKey); }

    // Experimental client capabilities.
    QJsonValue experimental() const { return value(experimentalKey); }
    void setExperimental(const QJsonValue &experimental) { insert(experimentalKey, experimental); }
    void clearExperimental() { remove(experimentalKey); }
};

} // namespace LanguageServerProtocol
