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

#include "lsptypes.h"

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT ResolveProviderOption : public JsonObject
{
public:
    using JsonObject::JsonObject;

    Utils::optional<bool> resolveProvider() const { return optionalValue<bool>(resolveProviderKey); }
    void setResolveProvider(bool resolveProvider) { insert(resolveProviderKey, resolveProvider); }
    void clearResolveProvider() { remove(resolveProviderKey); }

    bool isValid(QStringList *error) const override
    { return checkOptional<bool>(error, resolveProviderKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT TextDocumentRegistrationOptions : public JsonObject
{
public:
    using JsonObject::JsonObject;

    LanguageClientArray<DocumentFilter> documentSelector() const
    { return clientArray<DocumentFilter>(documentSelectorKey); }
    void setDocumentSelector(const LanguageClientArray<DocumentFilter> &documentSelector)
    { insert(documentSelectorKey, documentSelector.toJson()); }

    bool filterApplies(const Utils::FileName &fileName,
                       const Utils::MimeType &mimeType = Utils::MimeType()) const;

    bool isValid(QStringList *error) const override
    { return checkArray<DocumentFilter>(error, documentSelectorKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT SaveOptions : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // The client is supposed to include the content on save.
    Utils::optional<bool> includeText() const { return optionalValue<bool>(includeTextKey); }
    void setIncludeText(bool includeText) { insert(includeTextKey, includeText); }
    void clearIncludeText() { remove(includeTextKey); }

    bool isValid(QStringList *error) const override
    { return checkOptional<bool>(error, includeTextKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT TextDocumentSyncOptions : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // Open and close notifications are sent to the server.
    Utils::optional<bool> openClose() const { return optionalValue<bool>(openCloseKey); }
    void setOpenClose(bool openClose) { insert(openCloseKey, openClose); }
    void clearOpenClose() { remove(openCloseKey); }

    // Change notifications are sent to the server. See TextDocumentSyncKind.None,
    // TextDocumentSyncKind.Full and TextDocumentSyncKind.Incremental.
    Utils::optional<int> change() const { return optionalValue<int>(changeKey); }
    void setChange(int change) { insert(changeKey, change); }
    void clearChange() { remove(changeKey); }

    // Will save notifications are sent to the server.
    Utils::optional<bool> willSave() const { return optionalValue<bool>(willSaveKey); }
    void setWillSave(bool willSave) { insert(willSaveKey, willSave); }
    void clearWillSave() { remove(willSaveKey); }

    // Will save wait until requests are sent to the server.
    Utils::optional<bool> willSaveWaitUntil() const
    { return optionalValue<bool>(willSaveWaitUntilKey); }
    void setWillSaveWaitUntil(bool willSaveWaitUntil)
    { insert(willSaveWaitUntilKey, willSaveWaitUntil); }
    void clearWillSaveWaitUntil() { remove(willSaveWaitUntilKey); }

    // Save notifications are sent to the server.
    Utils::optional<SaveOptions> save() const { return optionalValue<SaveOptions>(saveKey); }
    void setSave(const SaveOptions &save) { insert(saveKey, save); }
    void clearSave() { remove(saveKey); }

    bool isValid(QStringList *error) const override;
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

class LANGUAGESERVERPROTOCOL_EXPORT CodeActionOptions : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QList<QString> codeActionKinds() const { return array<QString>(codeActionKindsKey); }
    void setCodeActionKinds(const QList<QString> &codeActionKinds)
    { insertArray(codeActionKindsKey, codeActionKinds); }

    bool isValid(QStringList *error) const override
    { return checkArray<QString>(error, codeActionKindsKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ServerCapabilities : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // Defines how the host (editor) should sync document changes to the language server.

    class LANGUAGESERVERPROTOCOL_EXPORT CompletionOptions : public ResolveProviderOption
    {
    public:
        using ResolveProviderOption::ResolveProviderOption;

        // The characters that trigger completion automatically.
        Utils::optional<QList<QString>> triggerCharacters() const
        { return optionalArray<QString>(triggerCharactersKey); }
        void setTriggerCharacters(const QList<QString> &triggerCharacters)
        { insertArray(triggerCharactersKey, triggerCharacters); }
        void clearTriggerCharacters() { remove(triggerCharactersKey); }

        bool isValid(QStringList *error) const override
        { return checkOptionalArray<QString>(error, triggerCharactersKey); }
    };

    class LANGUAGESERVERPROTOCOL_EXPORT SignatureHelpOptions : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        // The characters that trigger signature help automatically.
        Utils::optional<QList<QString>> triggerCharacters() const
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
        Utils::optional<QList<QString>> moreTriggerCharacter() const
        { return optionalArray<QString>(moreTriggerCharacterKey); }
        void setMoreTriggerCharacter(const QList<QString> &moreTriggerCharacter)
        { insertArray(moreTriggerCharacterKey, moreTriggerCharacter); }
        void clearMoreTriggerCharacter() { remove(moreTriggerCharacterKey); }

        bool isValid(QStringList *error) const override
        {
            return check<QString>(error, firstTriggerCharacterKey)
                    && checkOptionalArray<QString>(error, moreTriggerCharacterKey);
        }
    };

    using DocumentLinkOptions = ResolveProviderOption;

    class LANGUAGESERVERPROTOCOL_EXPORT ExecuteCommandOptions : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        QList<QString> commands() const { return array<QString>(commandsKey); }
        void setCommands(const QList<QString> &commands) { insertArray(commandsKey, commands); }

        bool isValid(QStringList *error) const override
        { return checkArray<QString>(error, commandsKey); }
    };

    using ColorProviderOptions = JsonObject;

    class LANGUAGESERVERPROTOCOL_EXPORT StaticRegistrationOptions : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        // The id used to register the request. The id can be used to deregister
        // the request again. See also Registration#id.
        Utils::optional<QString> id() const { return optionalValue<QString>(idKey); }
        void setId(const QString &id) { insert(idKey, id); }
        void clearId() { remove(idKey); }
    };

    // Defines how text documents are synced. Is either a detailed structure defining each
    // notification or for backwards compatibility the TextDocumentSyncKind number.
    using TextDocumentSync = Utils::variant<TextDocumentSyncOptions, int>;
    Utils::optional<TextDocumentSync> textDocumentSync() const;
    void setTextDocumentSync(const TextDocumentSync &textDocumentSync);
    void clearTextDocumentSync() { remove(textDocumentSyncKey); }

    TextDocumentSyncKind textDocumentSyncKindHelper();

    // The server provides hover support.
    Utils::optional<bool> hoverProvider() const { return optionalValue<bool>(hoverProviderKey); }
    void setHoverProvider(bool hoverProvider) { insert(hoverProviderKey, hoverProvider); }
    void clearHoverProvider() { remove(hoverProviderKey); }

    // The server provides completion support.
    Utils::optional<CompletionOptions> completionProvider() const
    { return optionalValue<CompletionOptions>(completionProviderKey); }
    void setCompletionProvider(const CompletionOptions &completionProvider)
    { insert(completionProviderKey, completionProvider); }
    void clearCompletionProvider() { remove(completionProviderKey); }

    // The server provides signature help support.
    Utils::optional<SignatureHelpOptions> signatureHelpProvider() const
    { return optionalValue<SignatureHelpOptions>(signatureHelpProviderKey); }
    void setSignatureHelpProvider(const SignatureHelpOptions &signatureHelpProvider)
    { insert(signatureHelpProviderKey, signatureHelpProvider); }
    void clearSignatureHelpProvider() { remove(signatureHelpProviderKey); }

    // The server provides goto definition support.
    Utils::optional<bool> definitionProvider() const
    { return optionalValue<bool>(definitionProviderKey); }
    void setDefinitionProvider(bool definitionProvider)
    { insert(definitionProviderKey, definitionProvider); }
    void clearDefinitionProvider() { remove(definitionProviderKey); }

    class LANGUAGESERVERPROTOCOL_EXPORT RegistrationOptions : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        LanguageClientArray<DocumentFilter> documentSelector() const
        { return clientArray<DocumentFilter>(documentSelectorKey); }
        void setDocumentSelector(const LanguageClientArray<DocumentFilter> &documentSelector)
        { insert(documentSelectorKey, documentSelector.toJson()); }

        bool filterApplies(const Utils::FileName &fileName,
                           const Utils::MimeType &mimeType = Utils::MimeType()) const;

        // The id used to register the request. The id can be used to deregister
        // the request again. See also Registration#id.
        Utils::optional<QString> id() const { return optionalValue<QString>(idKey); }
        void setId(const QString &id) { insert(idKey, id); }
        void clearId() { remove(idKey); }

        bool isValid(QStringList *error) const override
        { return checkArray<DocumentFilter>(error, documentSelectorKey) && checkOptional<bool>(error, idKey); }
    };

    // The server provides Goto Type Definition support.
    Utils::optional<Utils::variant<bool, RegistrationOptions>> typeDefinitionProvider() const;
    void setTypeDefinitionProvider(const Utils::variant<bool, RegistrationOptions> &typeDefinitionProvider);
    void clearTypeDefinitionProvider() { remove(typeDefinitionProviderKey); }

    // The server provides Goto Implementation support.
    Utils::optional<Utils::variant<bool, RegistrationOptions>> implementationProvider() const;
    void setImplementationProvider(const Utils::variant<bool, RegistrationOptions> &implementationProvider);
    void clearImplementationProvider() { remove(implementationProviderKey); }

    // The server provides find references support.
    Utils::optional<bool> referencesProvider() const { return optionalValue<bool>(referencesProviderKey); }
    void setReferencesProvider(bool referenceProvider) { insert(referencesProviderKey, referenceProvider); }
    void clearReferencesProvider() { remove(referencesProviderKey); }

    // The server provides document highlight support.
    Utils::optional<bool> documentHighlightProvider() const
    { return optionalValue<bool>(documentHighlightProviderKey); }
    void setDocumentHighlightProvider(bool documentHighlightProvider)
    { insert(documentHighlightProviderKey, documentHighlightProvider); }
    void clearDocumentHighlightProvider() { remove(documentHighlightProviderKey); }

    // The server provides document symbol support.
    Utils::optional<bool> documentSymbolProvider() const
    { return optionalValue<bool>(documentSymbolProviderKey); }
    void setDocumentSymbolProvider(bool documentSymbolProvider)
    { insert(documentSymbolProviderKey, documentSymbolProvider); }
    void clearDocumentSymbolProvider() { remove(documentSymbolProviderKey); }

    // The server provides workspace symbol support.
    Utils::optional<bool> workspaceSymbolProvider() const
    { return optionalValue<bool>(workspaceSymbolProviderKey); }
    void setWorkspaceSymbolProvider(bool workspaceSymbolProvider)
    { insert(workspaceSymbolProviderKey, workspaceSymbolProvider); }
    void clearWorkspaceSymbolProvider() { remove(workspaceSymbolProviderKey); }

    // The server provides code actions.
    Utils::optional<Utils::variant<bool, CodeActionOptions>> codeActionProvider() const;
    void setCodeActionProvider(bool codeActionProvider)
    { insert(codeActionProviderKey, codeActionProvider); }
    void setCodeActionProvider(CodeActionOptions options)
    { insert(codeActionProviderKey, options); }
    void clearCodeActionProvider() { remove(codeActionProviderKey); }

    // The server provides code lens.
    Utils::optional<CodeLensOptions> codeLensProvider() const
    { return optionalValue<CodeLensOptions>(codeLensProviderKey); }
    void setCodeLensProvider(CodeLensOptions codeLensProvider)
    { insert(codeLensProviderKey, codeLensProvider); }
    void clearCodeLensProvider() { remove(codeLensProviderKey); }

    // The server provides document formatting.
    Utils::optional<bool> documentFormattingProvider() const
    { return optionalValue<bool>(documentFormattingProviderKey); }
    void setDocumentFormattingProvider(bool documentFormattingProvider)
    { insert(documentFormattingProviderKey, documentFormattingProvider); }
    void clearDocumentFormattingProvider() { remove(documentFormattingProviderKey); }

    // The server provides document formatting on typing.
    Utils::optional<bool> documentRangeFormattingProvider() const
    { return optionalValue<bool>(documentRangeFormattingProviderKey); }
    void setDocumentRangeFormattingProvider(bool documentRangeFormattingProvider)
    { insert(documentRangeFormattingProviderKey, documentRangeFormattingProvider); }
    void clearDocumentRangeFormattingProvider() { remove(documentRangeFormattingProviderKey); }

    // The server provides rename support.
    Utils::optional<bool> renameProvider() const { return optionalValue<bool>(renameProviderKey); }
    void setRenameProvider(bool renameProvider) { insert(renameProviderKey, renameProvider); }
    void clearRenameProvider() { remove(renameProviderKey); }

    // The server provides document link support.
    Utils::optional<DocumentLinkOptions> documentLinkProvider() const
    { return optionalValue<DocumentLinkOptions>(documentLinkProviderKey); }
    void setDocumentLinkProvider(const DocumentLinkOptions &documentLinkProvider)
    { insert(documentLinkProviderKey, documentLinkProvider); }
    void clearDocumentLinkProvider() { remove(documentLinkProviderKey); }

    // The server provides color provider support.
    Utils::optional<TextDocumentRegistrationOptions> colorProvider() const
    { return optionalValue<TextDocumentRegistrationOptions>(colorProviderKey); }
    void setColorProvider(TextDocumentRegistrationOptions colorProvider)
    { insert(colorProviderKey, colorProvider); }
    void clearColorProvider() { remove(colorProviderKey); }

    // The server provides execute command support.
    Utils::optional<ExecuteCommandOptions> executeCommandProvider() const
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
            Utils::optional<bool> supported() const { return optionalValue<bool>(supportedKey); }
            void setSupported(bool supported) { insert(supportedKey, supported); }
            void clearSupported() { remove(supportedKey); }

            Utils::optional<Utils::variant<QString, bool>> changeNotifications() const;
            void setChangeNotifications(Utils::variant<QString, bool> changeNotifications);
            void clearChangeNotifications() { remove(changeNotificationsKey); }

            bool isValid(QStringList *error) const override;
        };

        Utils::optional<WorkspaceFoldersCapabilities> workspaceFolders() const
        { return optionalValue<WorkspaceFoldersCapabilities>(workspaceFoldersKey); }
        void setWorkspaceFolders(const WorkspaceFoldersCapabilities &workspaceFolders)
        { insert(workspaceFoldersKey, workspaceFolders); }
        void clearWorkspaceFolders() { remove(workspaceFoldersKey); }
    };

    Utils::optional<WorkspaceServerCapabilities> workspace() const
    { return optionalValue<WorkspaceServerCapabilities>(workspaceKey); }
    void setWorkspace(const WorkspaceServerCapabilities &workspace)
    { insert(workspaceKey, workspace); }
    void clearWorkspace() { remove(workspaceKey); }

    Utils::optional<JsonObject> experimental() const { return optionalValue<JsonObject>(experimentalKey); }
    void setExperimental(const JsonObject &experimental) { insert(experimentalKey, experimental); }
    void clearExperimental() { remove(experimentalKey); }

    bool isValid(QStringList *error) const override;
};

} // namespace LanguageClient
