// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

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
    if (Utils::optional<TextDocumentSync> sync = textDocumentSync()) {
        if (auto kind = std::get_if<int>(&*sync))
            return static_cast<TextDocumentSyncKind>(*kind);
        if (auto options = std::get_if<TextDocumentSyncOptions>(&*sync)) {
            if (const Utils::optional<int> &change = options->change())
                return static_cast<TextDocumentSyncKind>(*change);
        }
    }
    return TextDocumentSyncKind::None;
}

Utils::optional<std::variant<bool, WorkDoneProgressOptions>> ServerCapabilities::hoverProvider()
    const
{
    using RetType = std::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(hoverProviderKey);
    if (provider.isBool())
        return Utils::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return Utils::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return Utils::nullopt;
}

void ServerCapabilities::setHoverProvider(
    const std::variant<bool, WorkDoneProgressOptions> &hoverProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(hoverProviderKey, hoverProvider);
}

Utils::optional<std::variant<bool, ServerCapabilities::RegistrationOptions>>
ServerCapabilities::typeDefinitionProvider() const
{
    using RetType = std::variant<bool, ServerCapabilities::RegistrationOptions>;
    const QJsonValue &provider = value(typeDefinitionProviderKey);
    if (provider.isUndefined() || !(provider.isBool() || provider.isObject()))
        return Utils::nullopt;
    return Utils::make_optional(provider.isBool() ? RetType(provider.toBool())
                                                  : RetType(RegistrationOptions(provider.toObject())));
}

void ServerCapabilities::setTypeDefinitionProvider(
        const std::variant<bool, ServerCapabilities::RegistrationOptions> &typeDefinitionProvider)
{
    insertVariant<bool, ServerCapabilities::RegistrationOptions>(typeDefinitionProviderKey,
                                                                 typeDefinitionProvider);
}

Utils::optional<std::variant<bool, ServerCapabilities::RegistrationOptions>>
ServerCapabilities::implementationProvider() const
{
    using RetType = std::variant<bool, ServerCapabilities::RegistrationOptions>;
    const QJsonValue &provider = value(implementationProviderKey);
    if (provider.isUndefined() || !(provider.isBool() || provider.isObject()))
        return Utils::nullopt;
    return Utils::make_optional(provider.isBool() ? RetType(provider.toBool())
                                                  : RetType(RegistrationOptions(provider.toObject())));
}

void ServerCapabilities::setImplementationProvider(
        const std::variant<bool, ServerCapabilities::RegistrationOptions> &implementationProvider)
{
    insertVariant<bool, RegistrationOptions>(implementationProviderKey, implementationProvider);
}

Utils::optional<std::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::referencesProvider() const
{
    using RetType = std::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(referencesProviderKey);
    if (provider.isBool())
        return Utils::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return Utils::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return Utils::nullopt;
}

void ServerCapabilities::setReferencesProvider(
    const std::variant<bool, WorkDoneProgressOptions> &referencesProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(referencesProviderKey,
                                                 referencesProvider);
}

Utils::optional<std::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::documentHighlightProvider() const
{
    using RetType = std::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(documentHighlightProviderKey);
    if (provider.isBool())
        return Utils::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return Utils::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return Utils::nullopt;
}

void ServerCapabilities::setDocumentHighlightProvider(
    const std::variant<bool, WorkDoneProgressOptions> &documentHighlightProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(documentHighlightProviderKey,
                                                 documentHighlightProvider);
}

Utils::optional<std::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::documentSymbolProvider() const
{
    using RetType = std::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(documentSymbolProviderKey);
    if (provider.isBool())
        return Utils::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return Utils::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return Utils::nullopt;
}

void ServerCapabilities::setDocumentSymbolProvider(
    std::variant<bool, WorkDoneProgressOptions> documentSymbolProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(documentSymbolProviderKey,
                                                 documentSymbolProvider);
}

Utils::optional<SemanticTokensOptions> ServerCapabilities::semanticTokensProvider() const
{
    return optionalValue<SemanticTokensOptions>(semanticTokensProviderKey);
}

void ServerCapabilities::setSemanticTokensProvider(
    const SemanticTokensOptions &semanticTokensProvider)
{
    insert(semanticTokensProviderKey, semanticTokensProvider);
}

Utils::optional<std::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::workspaceSymbolProvider() const
{
    using RetType = std::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(workspaceSymbolProviderKey);
    if (provider.isBool())
        return Utils::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return Utils::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return Utils::nullopt;
}

void ServerCapabilities::setWorkspaceSymbolProvider(
    std::variant<bool, WorkDoneProgressOptions> workspaceSymbolProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(workspaceSymbolProviderKey,
                                                 workspaceSymbolProvider);
}

Utils::optional<std::variant<bool, CodeActionOptions>> ServerCapabilities::codeActionProvider() const
{
    const QJsonValue &provider = value(codeActionProviderKey);
    if (provider.isBool())
        return Utils::make_optional(std::variant<bool, CodeActionOptions>(provider.toBool()));
    if (provider.isObject()) {
        CodeActionOptions options(provider);
        if (options.isValid())
            return Utils::make_optional(std::variant<bool, CodeActionOptions>(options));
    }
    return Utils::nullopt;
}

Utils::optional<std::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::documentFormattingProvider() const
{
    using RetType = std::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(documentFormattingProviderKey);
    if (provider.isBool())
        return Utils::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return Utils::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return Utils::nullopt;
}

void ServerCapabilities::setDocumentFormattingProvider(
    const std::variant<bool, WorkDoneProgressOptions> &documentFormattingProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(documentFormattingProviderKey,
                                                 documentFormattingProvider);
}

Utils::optional<std::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::documentRangeFormattingProvider() const
{
    using RetType = std::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(documentRangeFormattingProviderKey);
    if (provider.isBool())
        return Utils::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return Utils::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return Utils::nullopt;
}

void ServerCapabilities::setDocumentRangeFormattingProvider(
    std::variant<bool, WorkDoneProgressOptions> documentRangeFormattingProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(documentRangeFormattingProviderKey,
                                                 documentRangeFormattingProvider);
}

Utils::optional<std::variant<ServerCapabilities::RenameOptions, bool>> ServerCapabilities::renameProvider() const
{
    using RetType = std::variant<ServerCapabilities::RenameOptions, bool>;
    const QJsonValue &localValue = value(renameProviderKey);
    if (localValue.isBool())
        return RetType(localValue.toBool());
    if (localValue.isObject())
        return RetType(RenameOptions(localValue.toObject()));
    return Utils::nullopt;
}

void ServerCapabilities::setRenameProvider(std::variant<ServerCapabilities::RenameOptions, bool> renameProvider)
{
    insertVariant<RenameOptions, bool>(renameProviderKey, renameProvider);
}

Utils::optional<std::variant<bool, JsonObject>> ServerCapabilities::colorProvider() const
{
    using RetType = std::variant<bool, JsonObject>;
    const QJsonValue &localValue = value(colorProviderKey);
    if (localValue.isBool())
        return RetType(localValue.toBool());
    if (localValue.isObject())
        return RetType(JsonObject(localValue.toObject()));
    return Utils::nullopt;
}

void ServerCapabilities::setColorProvider(std::variant<bool, JsonObject> colorProvider)
{
    insertVariant<bool, JsonObject>(renameProviderKey, colorProvider);
}

Utils::optional<std::variant<QString, bool> >
ServerCapabilities::WorkspaceServerCapabilities::WorkspaceFoldersCapabilities::changeNotifications() const
{
    using RetType = std::variant<QString, bool>;
    const QJsonValue &change = value(changeNotificationsKey);
    if (change.isUndefined())
        return Utils::nullopt;
    return Utils::make_optional(change.isBool() ? RetType(change.toBool())
                                                : RetType(change.toString()));
}

void ServerCapabilities::WorkspaceServerCapabilities::WorkspaceFoldersCapabilities::setChangeNotifications(
        std::variant<QString, bool> changeNotifications)
{
    insertVariant<QString, bool>(changeNotificationsKey, changeNotifications);
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

bool ServerCapabilities::ExecuteCommandOptions::isValid() const
{
    return WorkDoneProgressOptions::isValid() && contains(commandsKey);
}

bool CodeActionOptions::isValid() const
{
    return WorkDoneProgressOptions::isValid() && contains(codeActionKindsKey);
}

Utils::optional<std::variant<bool, QJsonObject>> SemanticTokensOptions::range() const
{
    using RetType = std::variant<bool, QJsonObject>;
    const QJsonValue &rangeOptions = value(rangeKey);
    if (rangeOptions.isBool())
        return RetType(rangeOptions.toBool());
    if (rangeOptions.isObject())
        return RetType(rangeOptions.toObject());
    return Utils::nullopt;
}

void SemanticTokensOptions::setRange(const std::variant<bool, QJsonObject> &range)
{
    insertVariant<bool, QJsonObject>(rangeKey, range);
}

Utils::optional<std::variant<bool, SemanticTokensOptions::FullSemanticTokenOptions>>
SemanticTokensOptions::full() const
{
    using RetType = std::variant<bool, SemanticTokensOptions::FullSemanticTokenOptions>;
    const QJsonValue &fullOptions = value(fullKey);
    if (fullOptions.isBool())
        return RetType(fullOptions.toBool());
    if (fullOptions.isObject())
        return RetType(FullSemanticTokenOptions(fullOptions.toObject()));
    return Utils::nullopt;
}

void SemanticTokensOptions::setFull(
    const std::variant<bool, SemanticTokensOptions::FullSemanticTokenOptions> &full)
{
    insertVariant<bool, FullSemanticTokenOptions>(fullKey, full);
}

SemanticRequestTypes SemanticTokensOptions::supportedRequests() const
{
    SemanticRequestTypes result;
    QJsonValue rangeValue = value(rangeKey);
    if (rangeValue.isObject() || rangeValue.toBool())
        result |= SemanticRequestType::Range;
    QJsonValue fullValue = value(fullKey);
    if (fullValue.isObject()) {
        SemanticTokensOptions::FullSemanticTokenOptions options(fullValue.toObject());
        if (options.delta().value_or(false))
            result |= SemanticRequestType::FullDelta;
        result |= SemanticRequestType::Full;
    } else if (fullValue.toBool()) {
        result |= SemanticRequestType::Full;
    }
    return result;
}

} // namespace LanguageServerProtocol
