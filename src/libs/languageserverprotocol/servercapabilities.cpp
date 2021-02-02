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

#include "servercapabilities.h"

namespace LanguageServerProtocol {

Utils::optional<ServerCapabilities::TextDocumentSync> ServerCapabilities::textDocumentSync() const
{
    const QJsonValue &sync = value(textDocumentSyncKey);
    if (sync.isUndefined())
        return Utils::nullopt;
    return Utils::make_optional(sync.isDouble() ? TextDocumentSync(sync.toInt())
                                                : TextDocumentSync(TextDocumentSyncOptions(sync.toObject())));
}

void ServerCapabilities::setTextDocumentSync(const ServerCapabilities::TextDocumentSync &textDocumentSync)
{
    insertVariant<TextDocumentSyncOptions, int>(textDocumentSyncKey, textDocumentSync);
}

TextDocumentSyncKind ServerCapabilities::textDocumentSyncKindHelper()
{
    Utils::optional<TextDocumentSync> sync = textDocumentSync();
    if (sync.has_value()) {
        if (auto kind = Utils::get_if<int>(&sync.value()))
            return static_cast<TextDocumentSyncKind>(*kind);
        if (auto options = Utils::get_if<TextDocumentSyncOptions>(&sync.value())) {
            if (const Utils::optional<int> &change = options->change())
                return static_cast<TextDocumentSyncKind>(change.value());
        }
    }
    return TextDocumentSyncKind::None;
}

Utils::optional<Utils::variant<bool, WorkDoneProgressOptions>> ServerCapabilities::hoverProvider()
    const
{
    using RetType = Utils::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(hoverProviderKey);
    if (provider.isBool())
        return Utils::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return Utils::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return Utils::nullopt;
}

void ServerCapabilities::setHoverProvider(
    const Utils::variant<bool, WorkDoneProgressOptions> &hoverProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(hoverProviderKey, hoverProvider);
}

Utils::optional<Utils::variant<bool, ServerCapabilities::RegistrationOptions>>
ServerCapabilities::typeDefinitionProvider() const
{
    using RetType = Utils::variant<bool, ServerCapabilities::RegistrationOptions>;
    const QJsonValue &provider = value(typeDefinitionProviderKey);
    if (provider.isUndefined() || !(provider.isBool() || provider.isObject()))
        return Utils::nullopt;
    return Utils::make_optional(provider.isBool() ? RetType(provider.toBool())
                                                  : RetType(RegistrationOptions(provider.toObject())));
}

void ServerCapabilities::setTypeDefinitionProvider(
        const Utils::variant<bool, ServerCapabilities::RegistrationOptions> &typeDefinitionProvider)
{
    insertVariant<bool, ServerCapabilities::RegistrationOptions>(typeDefinitionProviderKey,
                                                                 typeDefinitionProvider);
}

Utils::optional<Utils::variant<bool, ServerCapabilities::RegistrationOptions>>
ServerCapabilities::implementationProvider() const
{
    using RetType = Utils::variant<bool, ServerCapabilities::RegistrationOptions>;
    const QJsonValue &provider = value(implementationProviderKey);
    if (provider.isUndefined() || !(provider.isBool() || provider.isObject()))
        return Utils::nullopt;
    return Utils::make_optional(provider.isBool() ? RetType(provider.toBool())
                                                  : RetType(RegistrationOptions(provider.toObject())));
}

void ServerCapabilities::setImplementationProvider(
        const Utils::variant<bool, ServerCapabilities::RegistrationOptions> &implementationProvider)
{
    insertVariant<bool, RegistrationOptions>(implementationProviderKey, implementationProvider);
}

Utils::optional<Utils::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::referencesProvider() const
{
    using RetType = Utils::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(referencesProviderKey);
    if (provider.isBool())
        return Utils::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return Utils::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return Utils::nullopt;
}

void ServerCapabilities::setReferencesProvider(
    const Utils::variant<bool, WorkDoneProgressOptions> &referencesProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(referencesProviderKey,
                                                 referencesProvider);
}

Utils::optional<Utils::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::documentHighlightProvider() const
{
    using RetType = Utils::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(documentHighlightProviderKey);
    if (provider.isBool())
        return Utils::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return Utils::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return Utils::nullopt;
}

void ServerCapabilities::setDocumentHighlightProvider(
    const Utils::variant<bool, WorkDoneProgressOptions> &documentHighlightProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(documentHighlightProviderKey,
                                                 documentHighlightProvider);
}

Utils::optional<Utils::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::documentSymbolProvider() const
{
    using RetType = Utils::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(documentSymbolProviderKey);
    if (provider.isBool())
        return Utils::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return Utils::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return Utils::nullopt;
}

void ServerCapabilities::setDocumentSymbolProvider(
    Utils::variant<bool, WorkDoneProgressOptions> documentSymbolProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(documentSymbolProviderKey,
                                                 documentSymbolProvider);
}

Utils::optional<Utils::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::workspaceSymbolProvider() const
{
    using RetType = Utils::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(workspaceSymbolProviderKey);
    if (provider.isBool())
        return Utils::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return Utils::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return Utils::nullopt;
}

void ServerCapabilities::setWorkspaceSymbolProvider(
    Utils::variant<bool, WorkDoneProgressOptions> workspaceSymbolProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(workspaceSymbolProviderKey,
                                                 workspaceSymbolProvider);
}

Utils::optional<Utils::variant<bool, CodeActionOptions>> ServerCapabilities::codeActionProvider() const
{
    const QJsonValue &provider = value(codeActionProviderKey);
    if (provider.isBool())
        return Utils::make_optional(Utils::variant<bool, CodeActionOptions>(provider.toBool()));
    if (provider.isObject()) {
        CodeActionOptions options(provider);
        if (options.isValid(nullptr))
            return Utils::make_optional(Utils::variant<bool, CodeActionOptions>(options));
    }
    return Utils::nullopt;
}

Utils::optional<Utils::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::documentFormattingProvider() const
{
    using RetType = Utils::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(documentFormattingProviderKey);
    if (provider.isBool())
        return Utils::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return Utils::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return Utils::nullopt;
}

void ServerCapabilities::setDocumentFormattingProvider(
    const Utils::variant<bool, WorkDoneProgressOptions> &documentFormattingProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(documentFormattingProviderKey,
                                                 documentFormattingProvider);
}

Utils::optional<Utils::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::documentRangeFormattingProvider() const
{
    using RetType = Utils::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(documentRangeFormattingProviderKey);
    if (provider.isBool())
        return Utils::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return Utils::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return Utils::nullopt;
}

void ServerCapabilities::setDocumentRangeFormattingProvider(
    Utils::variant<bool, WorkDoneProgressOptions> documentRangeFormattingProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(documentRangeFormattingProviderKey,
                                                 documentRangeFormattingProvider);
}

Utils::optional<Utils::variant<ServerCapabilities::RenameOptions, bool>> ServerCapabilities::renameProvider() const
{
    using RetType = Utils::variant<ServerCapabilities::RenameOptions, bool>;
    const QJsonValue &localValue = value(renameProviderKey);
    if (localValue.isBool())
        return RetType(localValue.toBool());
    if (localValue.isObject())
        return RetType(RenameOptions(localValue.toObject()));
    return Utils::nullopt;
}

void ServerCapabilities::setRenameProvider(Utils::variant<ServerCapabilities::RenameOptions, bool> renameProvider)
{
    insertVariant<RenameOptions, bool>(renameProviderKey, renameProvider);
}

Utils::optional<Utils::variant<bool, JsonObject>> ServerCapabilities::colorProvider() const
{
    using RetType = Utils::variant<bool, JsonObject>;
    const QJsonValue &localValue = value(colorProviderKey);
    if (localValue.isBool())
        return RetType(localValue.toBool());
    if (localValue.isObject())
        return RetType(JsonObject(localValue.toObject()));
    return Utils::nullopt;
}

void ServerCapabilities::setColorProvider(Utils::variant<bool, JsonObject> colorProvider)
{
    insertVariant<bool, JsonObject>(renameProviderKey, colorProvider);
}

bool ServerCapabilities::isValid(ErrorHierarchy *error) const
{
    return checkOptional<TextDocumentSyncOptions, int>(error, textDocumentSyncKey)
            && checkOptional<bool>(error, hoverProviderKey)
            && checkOptional<CompletionOptions>(error, completionProviderKey)
            && checkOptional<SignatureHelpOptions>(error, signatureHelpProviderKey)
            && checkOptional<bool>(error, definitionProviderKey)
            && checkOptional<bool, RegistrationOptions>(error, typeDefinitionProviderKey)
            && checkOptional<bool, RegistrationOptions>(error, implementationProviderKey)
            && checkOptional<bool>(error, referencesProviderKey)
            && checkOptional<bool>(error, documentHighlightProviderKey)
            && checkOptional<bool>(error, documentSymbolProviderKey)
            && checkOptional<bool>(error, workspaceSymbolProviderKey)
            && checkOptional<bool, CodeActionOptions>(error, codeActionProviderKey)
            && checkOptional<CodeLensOptions>(error, codeLensProviderKey)
            && checkOptional<bool>(error, documentFormattingProviderKey)
            && checkOptional<bool>(error, documentRangeFormattingProviderKey)
            && checkOptional<bool, RenameOptions>(error, renameProviderKey)
            && checkOptional<DocumentLinkOptions>(error, documentLinkProviderKey)
            && checkOptional<bool, JsonObject>(error, colorProviderKey)
            && checkOptional<ExecuteCommandOptions>(error, executeCommandProviderKey)
            && checkOptional<WorkspaceServerCapabilities>(error, workspaceKey)
            && checkOptional<SemanticHighlightingServerCapabilities>(error, semanticHighlightingKey);
}

Utils::optional<Utils::variant<QString, bool> >
ServerCapabilities::WorkspaceServerCapabilities::WorkspaceFoldersCapabilities::changeNotifications() const
{
    using RetType = Utils::variant<QString, bool>;
    const QJsonValue &provider = value(implementationProviderKey);
    if (provider.isUndefined())
        return Utils::nullopt;
    return Utils::make_optional(provider.isBool() ? RetType(provider.toBool())
                                                  : RetType(provider.toString()));
}

void ServerCapabilities::WorkspaceServerCapabilities::WorkspaceFoldersCapabilities::setChangeNotifications(
        Utils::variant<QString, bool> changeNotifications)
{
    insertVariant<QString, bool>(changeNotificationsKey, changeNotifications);
}

bool ServerCapabilities::WorkspaceServerCapabilities::WorkspaceFoldersCapabilities::isValid(ErrorHierarchy *error) const
{
    return checkOptional<bool>(error, supportedKey)
            && checkOptional<QString, bool>(error, changeNotificationsKey);
}

bool TextDocumentRegistrationOptions::filterApplies(const Utils::FilePath &fileName,
                                                    const Utils::MimeType &mimeType) const
{
    const LanguageClientArray<DocumentFilter> &selector = documentSelector();
    return selector.isNull()
            || selector.toList().isEmpty()
            || Utils::anyOf(selector.toList(), [&](auto filter){
        return filter.applies(fileName, mimeType);
    });
}

bool TextDocumentSyncOptions::isValid(ErrorHierarchy *error) const
{
    return checkOptional<bool>(error, openCloseKey)
            && checkOptional<int>(error, changeKey)
            && checkOptional<bool>(error, willSaveKey)
            && checkOptional<bool>(error, willSaveWaitUntilKey)
            && checkOptional<SaveOptions>(error, saveKey);
}

Utils::optional<QList<QList<QString>>> ServerCapabilities::SemanticHighlightingServerCapabilities::scopes() const
{
    QList<QList<QString>> scopes;
    if (!contains(scopesKey))
        return Utils::nullopt;
    for (const QJsonValue jsonScopeValue : value(scopesKey).toArray()) {
        if (!jsonScopeValue.isArray())
            return {};
        QList<QString> scope;
        for (const QJsonValue value : jsonScopeValue.toArray()) {
            if (!value.isString())
                return {};
            scope.append(value.toString());
        }
        scopes.append(scope);
    }
    return Utils::make_optional(scopes);
}

void ServerCapabilities::SemanticHighlightingServerCapabilities::setScopes(
    const QList<QList<QString>> &scopes)
{
    QJsonArray jsonScopes;
    for (const QList<QString> &scope : scopes) {
        QJsonArray jsonScope;
        for (const QString &value : scope)
            jsonScope.append(value);
        jsonScopes.append(jsonScope);
    }
    insert(scopesKey, jsonScopes);
}

bool ServerCapabilities::SemanticHighlightingServerCapabilities::isValid(ErrorHierarchy *) const
{
    return contains(scopesKey) && value(scopesKey).isArray()
           && Utils::allOf(value(scopesKey).toArray(), [](const QJsonValue &array) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                  return array.isArray() && Utils::allOf(array.toArray(), &QJsonValue::isString);
#else
                  return array.isArray() && Utils::allOf(array.toArray(), &QJsonValueRef::isString);
#endif
              });
}

bool ServerCapabilities::ExecuteCommandOptions::isValid(ErrorHierarchy *error) const
{
    return WorkDoneProgressOptions::isValid(error) && checkArray<QString>(error, commandsKey);
}

bool ServerCapabilities::CompletionOptions::isValid(ErrorHierarchy *error) const
{
    return WorkDoneProgressOptions::isValid(error)
           && checkOptionalArray<QString>(error, triggerCharactersKey)
           && checkOptional<bool>(error, resolveProviderKey);
}

bool ServerCapabilities::SignatureHelpOptions::isValid(ErrorHierarchy *error) const
{
    return WorkDoneProgressOptions::isValid(error)
           && checkOptionalArray<QString>(error, triggerCharactersKey);
}

bool CodeActionOptions::isValid(ErrorHierarchy *error) const
{
    return WorkDoneProgressOptions::isValid(error)
           && checkArray<QString>(error, codeActionKindsKey);
}

bool ServerCapabilities::RenameOptions::isValid(ErrorHierarchy *error) const
{
    return WorkDoneProgressOptions::isValid(error)
           && checkOptional<bool>(error, prepareProviderKey);
}

} // namespace LanguageServerProtocol
