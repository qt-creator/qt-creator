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

#include "jsonkeys.h"
#include "lsptypes.h"

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT DynamicRegistrationCapabilities : public JsonObject
{
public:
    using JsonObject::JsonObject;

    Utils::optional<bool> dynamicRegistration() const { return optionalValue<bool>(dynamicRegistrationKey); }
    void setDynamicRegistration(bool dynamicRegistration) { insert(dynamicRegistrationKey, dynamicRegistration); }
    void clearDynamicRegistration() { remove(dynamicRegistrationKey); }

    bool isValid(ErrorHierarchy *error) const override
    { return checkOptional<bool>(error, dynamicRegistrationKey); }
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
        Utils::optional<QList<SymbolKind>> valueSet() const;
        void setValueSet(const QList<SymbolKind> &valueSet);
        void clearValueSet() { remove(valueSetKey); }

        bool isValid(ErrorHierarchy *error) const override
        { return checkOptionalArray<int>(error, valueSetKey); }
    };

    // Specific capabilities for the `SymbolKind` in the `workspace/symbol` request.
    Utils::optional<SymbolKindCapabilities> symbolKind() const
    { return optionalValue<SymbolKindCapabilities>(symbolKindKey); }
    void setSymbolKind(const SymbolKindCapabilities &symbolKind) { insert(symbolKindKey, symbolKind); }
    void clearSymbolKind() { remove(symbolKindKey); }

    bool isValid(ErrorHierarchy *error) const override;
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
        Utils::optional<bool> willSave() const { return optionalValue<bool>(willSaveKey); }
        void setWillSave(bool willSave) { insert(willSaveKey, willSave); }
        void clearWillSave() { remove(willSaveKey); }

        /*
         * The client supports sending a will save request and
         * waits for a response providing text edits which will
         * be applied to the document before it is saved.
         */
        Utils::optional<bool> willSaveWaitUntil() const
        { return optionalValue<bool>(willSaveWaitUntilKey); }
        void setWillSaveWaitUntil(bool willSaveWaitUntil)
        { insert(willSaveWaitUntilKey, willSaveWaitUntil); }
        void clearWillSaveWaitUntil() { remove(willSaveWaitUntilKey); }

        // The client supports did save notifications.
        Utils::optional<bool> didSave() const { return optionalValue<bool>(didSaveKey); }
        void setDidSave(bool didSave) { insert(didSaveKey, didSave); }
        void clearDidSave() { remove(didSaveKey); }

        bool isValid(ErrorHierarchy *error) const override;
    };

    Utils::optional<SynchronizationCapabilities> synchronization() const
    { return optionalValue<SynchronizationCapabilities>(synchronizationKey); }
    void setSynchronization(const SynchronizationCapabilities &synchronization)
    { insert(synchronizationKey, synchronization); }
    void clearSynchronization() { remove(synchronizationKey); }

    class LANGUAGESERVERPROTOCOL_EXPORT SemanticHighlightingCapabilities : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        bool semanticHighlighting() const { return typedValue<bool>(semanticHighlightingKey); }
        void setSemanticHighlighting(bool semanticHighlighting)
        { insert(semanticHighlightingKey, semanticHighlighting); }

        bool isValid(ErrorHierarchy *error) const override
        { return check<bool>(error, semanticHighlightingKey); }
    };

    Utils::optional<SemanticHighlightingCapabilities> semanticHighlightingCapabilities() const
    { return optionalValue<SemanticHighlightingCapabilities>(semanticHighlightingCapabilitiesKey); }
    void setSemanticHighlightingCapabilities(
        const SemanticHighlightingCapabilities &semanticHighlightingCapabilities)
    { insert(semanticHighlightingCapabilitiesKey, semanticHighlightingCapabilities); }
    void clearSemanticHighlightingCapabilities() { remove(semanticHighlightingCapabilitiesKey); }

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
            Utils::optional<bool> snippetSupport() const
            { return optionalValue<bool>(snippetSupportKey); }
            void setSnippetSupport(bool snippetSupport)
            { insert(snippetSupportKey, snippetSupport); }
            void clearSnippetSupport() { remove(snippetSupportKey); }

            // Client supports commit characters on a completion item.
            Utils::optional<bool> commitCharacterSupport() const
            { return optionalValue<bool>(commitCharacterSupportKey); }
            void setCommitCharacterSupport(bool commitCharacterSupport)
            { insert(commitCharacterSupportKey, commitCharacterSupport); }
            void clearCommitCharacterSupport() { remove(commitCharacterSupportKey); }

            /*
             * Client supports the follow content formats for the documentation
             * property. The order describes the preferred format of the client.
             */
            Utils::optional<QList<MarkupKind>> documentationFormat() const;
            void setDocumentationFormat(const QList<MarkupKind> &documentationFormat);
            void clearDocumentationFormat() { remove(documentationFormatKey); }

            bool isValid(ErrorHierarchy *error) const override
            {
                return checkOptional<bool>(error, snippetSupportKey)
                        && checkOptional<bool>(error, commitCharacterSupportKey)
                        && checkOptionalArray<int>(error, documentationFormatKey);
            }
        };

        // The client supports the following `CompletionItem` specific capabilities.
        Utils::optional<CompletionItemCapbilities> completionItem() const
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
            Utils::optional<QList<CompletionItemKind::Kind>> valueSet() const;
            void setValueSet(const QList<CompletionItemKind::Kind> &valueSet);
            void clearValueSet() { remove(valueSetKey); }

            bool isValid(ErrorHierarchy *error) const override
            { return checkOptionalArray<int>(error, valueSetKey); }
        };

        Utils::optional<CompletionItemKindCapabilities> completionItemKind() const
        { return optionalValue<CompletionItemKindCapabilities>(completionItemKindKey); }
        void setCompletionItemKind(const CompletionItemKindCapabilities &completionItemKind)
        { insert(completionItemKindKey, completionItemKind); }
        void clearCompletionItemKind() { remove(completionItemKindKey); }

        /*
         * The client supports to send additional context information for a
         * `textDocument/completion` request.
         */
        Utils::optional<bool> contextSupport() const { return optionalValue<bool>(contextSupportKey); }
        void setContextSupport(bool contextSupport) { insert(contextSupportKey, contextSupport); }
        void clearContextSupport() { remove(contextSupportKey); }

        bool isValid(ErrorHierarchy *error) const override;
    };

    // Capabilities specific to the `textDocument/completion`
    Utils::optional<CompletionCapabilities> completion() const
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
        Utils::optional<QList<MarkupKind>> contentFormat() const;
        void setContentFormat(const QList<MarkupKind> &contentFormat);
        void clearContentFormat() { remove(contentFormatKey); }

        bool isValid(ErrorHierarchy *error) const override;
    };

    Utils::optional<HoverCapabilities> hover() const { return optionalValue<HoverCapabilities>(hoverKey); }
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
            Utils::optional<QList<MarkupKind>> documentationFormat() const;
            void setDocumentationFormat(const QList<MarkupKind> &documentationFormat);
            void clearDocumentationFormat() { remove(documentationFormatKey); }

            bool isValid(ErrorHierarchy *error) const override
            { return checkOptionalArray<int>(error, documentationFormatKey); }
        };

        // The client supports the following `SignatureInformation` specific properties.
        Utils::optional<SignatureHelpCapabilities> signatureInformation() const
        { return optionalValue<SignatureHelpCapabilities>(signatureInformationKey); }
        void setSignatureInformation(const SignatureHelpCapabilities &signatureInformation)
        { insert(signatureInformationKey, signatureInformation); }
        void clearSignatureInformation() { remove(signatureInformationKey); }

        bool isValid(ErrorHierarchy *error) const override;
    };

    // Capabilities specific to the `textDocument/signatureHelp`
    Utils::optional<SignatureHelpCapabilities> signatureHelp() const
    { return optionalValue<SignatureHelpCapabilities>(signatureHelpKey); }
    void setSignatureHelp(const SignatureHelpCapabilities &signatureHelp)
    { insert(signatureHelpKey, signatureHelp); }
    void clearSignatureHelp() { remove(signatureHelpKey); }

    // Whether references supports dynamic registration.
    Utils::optional<DynamicRegistrationCapabilities> references() const
    { return optionalValue<DynamicRegistrationCapabilities>(referencesKey); }
    void setReferences(const DynamicRegistrationCapabilities &references)
    { insert(referencesKey, references); }
    void clearReferences() { remove(referencesKey); }

    // Whether document highlight supports dynamic registration.
    Utils::optional<DynamicRegistrationCapabilities> documentHighlight() const
    { return optionalValue<DynamicRegistrationCapabilities>(documentHighlightKey); }
    void setDocumentHighlight(const DynamicRegistrationCapabilities &documentHighlight)
    { insert(documentHighlightKey, documentHighlight); }
    void clearDocumentHighlight() { remove(documentHighlightKey); }

    // Capabilities specific to the `textDocument/documentSymbol`
    Utils::optional<SymbolCapabilities> documentSymbol() const
    { return optionalValue<SymbolCapabilities>(documentSymbolKey); }
    void setDocumentSymbol(const SymbolCapabilities &documentSymbol)
    { insert(documentSymbolKey, documentSymbol); }
    void clearDocumentSymbol() { remove(documentSymbolKey); }

    // Whether formatting supports dynamic registration.
    Utils::optional<DynamicRegistrationCapabilities> formatting() const
    { return optionalValue<DynamicRegistrationCapabilities>(formattingKey); }
    void setFormatting(const DynamicRegistrationCapabilities &formatting)
    { insert(formattingKey, formatting); }
    void clearFormatting() { remove(formattingKey); }

    // Whether range formatting supports dynamic registration.
    Utils::optional<DynamicRegistrationCapabilities> rangeFormatting() const
    { return optionalValue<DynamicRegistrationCapabilities>(rangeFormattingKey); }
    void setRangeFormatting(const DynamicRegistrationCapabilities &rangeFormatting)
    { insert(rangeFormattingKey, rangeFormatting); }
    void clearRangeFormatting() { remove(rangeFormattingKey); }

    // Whether on type formatting supports dynamic registration.
    Utils::optional<DynamicRegistrationCapabilities> onTypeFormatting() const
    { return optionalValue<DynamicRegistrationCapabilities>(onTypeFormattingKey); }
    void setOnTypeFormatting(const DynamicRegistrationCapabilities &onTypeFormatting)
    { insert(onTypeFormattingKey, onTypeFormatting); }
    void clearOnTypeFormatting() { remove(onTypeFormattingKey); }

    // Whether definition supports dynamic registration.
    Utils::optional<DynamicRegistrationCapabilities> definition() const
    { return optionalValue<DynamicRegistrationCapabilities>(definitionKey); }
    void setDefinition(const DynamicRegistrationCapabilities &definition)
    { insert(definitionKey, definition); }
    void clearDefinition() { remove(definitionKey); }

    /*
     * Whether typeDefinition supports dynamic registration. If this is set to `true`
     * the client supports the new `(TextDocumentRegistrationOptions & StaticRegistrationOptions)`
     * return value for the corresponding server capability as well.
     */
    Utils::optional<DynamicRegistrationCapabilities> typeDefinition() const
    { return optionalValue<DynamicRegistrationCapabilities>(typeDefinitionKey); }
    void setTypeDefinition(const DynamicRegistrationCapabilities &typeDefinition)
    { insert(typeDefinitionKey, typeDefinition); }
    void clearTypeDefinition() { remove(typeDefinitionKey); }

    /*
     * Whether implementation supports dynamic registration. If this is set to `true`
     * the client supports the new `(TextDocumentRegistrationOptions & StaticRegistrationOptions)`
     * return value for the corresponding server capability as well.
     */
    Utils::optional<DynamicRegistrationCapabilities> implementation() const
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

                bool isValid(ErrorHierarchy *errorHierarchy) const override
                { return checkArray<QString>(errorHierarchy, valueSetKey); }
            };

            CodeActionKind codeActionKind() const
            { return typedValue<CodeActionKind>(codeActionKindKey); }
            void setCodeActionKind(const CodeActionKind &codeActionKind)
            { insert(codeActionKindKey, codeActionKind); }

            bool isValid(ErrorHierarchy *errorHierarchy) const override
            { return check<CodeActionKind>(errorHierarchy, codeActionKindKey); }
        };

        Utils::optional<CodeActionLiteralSupport> codeActionLiteralSupport() const
        { return optionalValue<CodeActionLiteralSupport>(codeActionLiteralSupportKey); }
        void setCodeActionLiteralSupport(const CodeActionLiteralSupport &codeActionLiteralSupport)
        { insert(codeActionLiteralSupportKey, codeActionLiteralSupport); }
        void clearCodeActionLiteralSupport() { remove(codeActionLiteralSupportKey); }

        bool isValid(ErrorHierarchy *errorHierarchy) const override;
    };

    // Whether code action supports dynamic registration.
    Utils::optional<CodeActionCapabilities> codeAction() const
    { return optionalValue<CodeActionCapabilities>(codeActionKey); }
    void setCodeAction(const CodeActionCapabilities &codeAction)
    { insert(codeActionKey, codeAction); }
    void clearCodeAction() { remove(codeActionKey); }

    // Whether code lens supports dynamic registration.
    Utils::optional<DynamicRegistrationCapabilities> codeLens() const
    { return optionalValue<DynamicRegistrationCapabilities>(codeLensKey); }
    void setCodeLens(const DynamicRegistrationCapabilities &codeLens)
    { insert(codeLensKey, codeLens); }
    void clearCodeLens() { remove(codeLensKey); }

    // Whether document link supports dynamic registration.
    Utils::optional<DynamicRegistrationCapabilities> documentLink() const
    { return optionalValue<DynamicRegistrationCapabilities>(documentLinkKey); }
    void setDocumentLink(const DynamicRegistrationCapabilities &documentLink)
    { insert(documentLinkKey, documentLink); }
    void clearDocumentLink() { remove(documentLinkKey); }

    /*
     * Whether colorProvider supports dynamic registration. If this is set to `true`
     * the client supports the new `(ColorProviderOptions & TextDocumentRegistrationOptions & StaticRegistrationOptions)`
     * return value for the corresponding server capability as well.
     */
    Utils::optional<DynamicRegistrationCapabilities> colorProvider() const
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

        Utils::optional<bool> prepareSupport() const { return optionalValue<bool>(prepareSupportKey); }
        void setPrepareSupport(bool prepareSupport) { insert(prepareSupportKey, prepareSupport); }
        void clearPrepareSupport() { remove(prepareSupportKey); }

        bool isValid(ErrorHierarchy *error) const override;
    };

    // Whether rename supports dynamic registration.
    Utils::optional<RenameClientCapabilities> rename() const
    { return optionalValue<RenameClientCapabilities>(renameKey); }
    void setRename(const RenameClientCapabilities &rename)
    { insert(renameKey, rename); }
    void clearRename() { remove(renameKey); }

    bool isValid(ErrorHierarchy *error) const override;
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
    Utils::optional<bool> applyEdit() const { return optionalValue<bool>(applyEditKey); }
    void setApplyEdit(bool applyEdit) { insert(applyEditKey, applyEdit); }
    void clearApplyEdit() { remove(applyEditKey); }

    class LANGUAGESERVERPROTOCOL_EXPORT WorkspaceEditCapabilities : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        // The client supports versioned document changes in `WorkspaceEdit`s
        Utils::optional<bool> documentChanges() const
        { return optionalValue<bool>(documentChangesKey); }
        void setDocumentChanges(bool documentChanges)
        { insert(documentChangesKey, documentChanges); }
        void clearDocumentChanges() { remove(documentChangesKey); }

        bool isValid(ErrorHierarchy *error) const override
        { return checkOptional<bool>(error, documentChangesKey); }
    };

    // Capabilities specific to `WorkspaceEdit`s
    Utils::optional<WorkspaceEditCapabilities> workspaceEdit() const
    { return optionalValue<WorkspaceEditCapabilities>(workspaceEditKey); }
    void setWorkspaceEdit(const WorkspaceEditCapabilities &workspaceEdit)
    { insert(workspaceEditKey, workspaceEdit); }
    void clearWorkspaceEdit() { remove(workspaceEditKey); }

    // Capabilities specific to the `workspace/didChangeConfiguration` notification.
    Utils::optional<DynamicRegistrationCapabilities> didChangeConfiguration() const
    { return optionalValue<DynamicRegistrationCapabilities>(didChangeConfigurationKey); }
    void setDidChangeConfiguration(const DynamicRegistrationCapabilities &didChangeConfiguration)
    { insert(didChangeConfigurationKey, didChangeConfiguration); }
    void clearDidChangeConfiguration() { remove(didChangeConfigurationKey); }

    // Capabilities specific to the `workspace/didChangeWatchedFiles` notification.
    Utils::optional<DynamicRegistrationCapabilities> didChangeWatchedFiles() const
    { return optionalValue<DynamicRegistrationCapabilities>(didChangeWatchedFilesKey); }
    void setDidChangeWatchedFiles(const DynamicRegistrationCapabilities &didChangeWatchedFiles)
    { insert(didChangeWatchedFilesKey, didChangeWatchedFiles); }
    void clearDidChangeWatchedFiles() { remove(didChangeWatchedFilesKey); }

    // Specific capabilities for the `SymbolKind` in the `workspace/symbol` request.
    Utils::optional<SymbolCapabilities> symbol() const
    { return optionalValue<SymbolCapabilities>(symbolKey); }
    void setSymbol(const SymbolCapabilities &symbol) { insert(symbolKey, symbol); }
    void clearSymbol() { remove(symbolKey); }

    // Capabilities specific to the `workspace/executeCommand` request.
    Utils::optional<DynamicRegistrationCapabilities> executeCommand() const
    { return optionalValue<DynamicRegistrationCapabilities>(executeCommandKey); }
    void setExecuteCommand(const DynamicRegistrationCapabilities &executeCommand)
    { insert(executeCommandKey, executeCommand); }
    void clearExecuteCommand() { remove(executeCommandKey); }

    // The client has support for workspace folders. Since 3.6.0
    Utils::optional<bool> workspaceFolders() const
    { return optionalValue<bool>(workspaceFoldersKey); }
    void setWorkspaceFolders(bool workspaceFolders)
    { insert(workspaceFoldersKey, workspaceFolders); }
    void clearWorkspaceFolders() { remove(workspaceFoldersKey); }

    // The client supports `workspace/configuration` requests. Since 3.6.0
    Utils::optional<bool> configuration() const { return optionalValue<bool>(configurationKey); }
    void setConfiguration(bool configuration) { insert(configurationKey, configuration); }
    void clearConfiguration() { remove(configurationKey); }

    bool isValid(ErrorHierarchy *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT ClientCapabilities : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // Workspace specific client capabilities.
    Utils::optional<WorkspaceClientCapabilities> workspace() const
    { return optionalValue<WorkspaceClientCapabilities>(workspaceKey); }
    void setWorkspace(const WorkspaceClientCapabilities &workspace)
    { insert(workspaceKey, workspace); }
    void clearWorkspace() { remove(workspaceKey); }

    // Text document specific client capabilities.
    Utils::optional<TextDocumentClientCapabilities> textDocument() const
    { return optionalValue<TextDocumentClientCapabilities>(textDocumentKey); }
    void setTextDocument(const TextDocumentClientCapabilities &textDocument)
    { insert(textDocumentKey, textDocument); }
    void clearTextDocument() { remove(textDocumentKey); }

    // Experimental client capabilities.
    QJsonValue experimental() const { return value(experimentalKey); }
    void setExperimental(const QJsonValue &experimental) { insert(experimentalKey, experimental); }
    void clearExperimental() { remove(experimentalKey); }

    bool isValid(ErrorHierarchy *error) const override;
};

}
