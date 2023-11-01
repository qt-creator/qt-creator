// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "servercapabilities.h"

namespace LanguageServerProtocol {

std::optional<ServerCapabilities::TextDocumentSync> ServerCapabilities::textDocumentSync() const
{
    const QJsonValue &sync = value(textDocumentSyncKey);
    if (sync.isUndefined())
        return std::nullopt;
    return std::make_optional(sync.isDouble()
                                  ? TextDocumentSync(sync.toInt())
                                  : TextDocumentSync(TextDocumentSyncOptions(sync.toObject())));
}

void ServerCapabilities::setTextDocumentSync(const ServerCapabilities::TextDocumentSync &textDocumentSync)
{
    insertVariant<TextDocumentSyncOptions, int>(textDocumentSyncKey, textDocumentSync);
}

TextDocumentSyncKind ServerCapabilities::textDocumentSyncKindHelper()
{
    if (std::optional<TextDocumentSync> sync = textDocumentSync()) {
        if (auto kind = std::get_if<int>(&*sync))
            return static_cast<TextDocumentSyncKind>(*kind);
        if (auto options = std::get_if<TextDocumentSyncOptions>(&*sync)) {
            if (const std::optional<int> &change = options->change())
                return static_cast<TextDocumentSyncKind>(*change);
        }
    }
    return TextDocumentSyncKind::None;
}

std::optional<std::variant<bool, WorkDoneProgressOptions>> ServerCapabilities::hoverProvider()
    const
{
    using RetType = std::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(hoverProviderKey);
    if (provider.isBool())
        return std::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return std::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return std::nullopt;
}

void ServerCapabilities::setHoverProvider(
    const std::variant<bool, WorkDoneProgressOptions> &hoverProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(hoverProviderKey, hoverProvider);
}

std::optional<std::variant<bool, ServerCapabilities::RegistrationOptions>>
ServerCapabilities::definitionProvider() const
{
    using RetType = std::variant<bool, ServerCapabilities::RegistrationOptions>;
    const QJsonValue &provider = value(definitionProviderKey);
    if (provider.isUndefined() || !(provider.isBool() || provider.isObject()))
        return std::nullopt;
    return std::make_optional(provider.isBool()
                                  ? RetType(provider.toBool())
                                  : RetType(RegistrationOptions(provider.toObject())));
}

void ServerCapabilities::setDefinitionProvider(
    const std::variant<bool, RegistrationOptions> &definitionProvider)
{
    insertVariant<bool, RegistrationOptions>(definitionProviderKey, definitionProvider);
}

std::optional<std::variant<bool, ServerCapabilities::RegistrationOptions>>
ServerCapabilities::typeDefinitionProvider() const
{
    using RetType = std::variant<bool, ServerCapabilities::RegistrationOptions>;
    const QJsonValue &provider = value(typeDefinitionProviderKey);
    if (provider.isUndefined() || !(provider.isBool() || provider.isObject()))
        return std::nullopt;
    return std::make_optional(provider.isBool()
                                  ? RetType(provider.toBool())
                                  : RetType(RegistrationOptions(provider.toObject())));
}

void ServerCapabilities::setTypeDefinitionProvider(
        const std::variant<bool, ServerCapabilities::RegistrationOptions> &typeDefinitionProvider)
{
    insertVariant<bool, ServerCapabilities::RegistrationOptions>(typeDefinitionProviderKey,
                                                                 typeDefinitionProvider);
}

std::optional<std::variant<bool, ServerCapabilities::RegistrationOptions>>
ServerCapabilities::implementationProvider() const
{
    using RetType = std::variant<bool, ServerCapabilities::RegistrationOptions>;
    const QJsonValue &provider = value(implementationProviderKey);
    if (provider.isUndefined() || !(provider.isBool() || provider.isObject()))
        return std::nullopt;
    return std::make_optional(provider.isBool()
                                  ? RetType(provider.toBool())
                                  : RetType(RegistrationOptions(provider.toObject())));
}

void ServerCapabilities::setImplementationProvider(
        const std::variant<bool, ServerCapabilities::RegistrationOptions> &implementationProvider)
{
    insertVariant<bool, RegistrationOptions>(implementationProviderKey, implementationProvider);
}

std::optional<std::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::referencesProvider() const
{
    using RetType = std::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(referencesProviderKey);
    if (provider.isBool())
        return std::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return std::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return std::nullopt;
}

void ServerCapabilities::setReferencesProvider(
    const std::variant<bool, WorkDoneProgressOptions> &referencesProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(referencesProviderKey,
                                                 referencesProvider);
}

std::optional<std::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::documentHighlightProvider() const
{
    using RetType = std::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(documentHighlightProviderKey);
    if (provider.isBool())
        return std::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return std::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return std::nullopt;
}

void ServerCapabilities::setDocumentHighlightProvider(
    const std::variant<bool, WorkDoneProgressOptions> &documentHighlightProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(documentHighlightProviderKey,
                                                 documentHighlightProvider);
}

std::optional<std::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::documentSymbolProvider() const
{
    using RetType = std::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(documentSymbolProviderKey);
    if (provider.isBool())
        return std::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return std::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return std::nullopt;
}

void ServerCapabilities::setDocumentSymbolProvider(
    std::variant<bool, WorkDoneProgressOptions> documentSymbolProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(documentSymbolProviderKey,
                                                 documentSymbolProvider);
}

std::optional<SemanticTokensOptions> ServerCapabilities::semanticTokensProvider() const
{
    return optionalValue<SemanticTokensOptions>(semanticTokensProviderKey);
}

void ServerCapabilities::setSemanticTokensProvider(
    const SemanticTokensOptions &semanticTokensProvider)
{
    insert(semanticTokensProviderKey, semanticTokensProvider);
}

std::optional<std::variant<bool, WorkDoneProgressOptions> >
ServerCapabilities::callHierarchyProvider() const
{
    const QJsonValue &provider = value(callHierarchyProviderKey);
    if (provider.isBool())
        return provider.toBool();
    else if (provider.isObject())
        return WorkDoneProgressOptions(provider.toObject());
    return std::nullopt;
}

void ServerCapabilities::setCallHierarchyProvider(
    const std::variant<bool, WorkDoneProgressOptions> &callHierarchyProvider)
{
    QJsonValue val;
    if (std::holds_alternative<bool>(callHierarchyProvider))
        val = std::get<bool>(callHierarchyProvider);
    else if (std::holds_alternative<WorkDoneProgressOptions>(callHierarchyProvider))
        val = QJsonObject(std::get<WorkDoneProgressOptions>(callHierarchyProvider));
    insert(callHierarchyProviderKey, val);
}

std::optional<std::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::workspaceSymbolProvider() const
{
    using RetType = std::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(workspaceSymbolProviderKey);
    if (provider.isBool())
        return std::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return std::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return std::nullopt;
}

void ServerCapabilities::setWorkspaceSymbolProvider(
    std::variant<bool, WorkDoneProgressOptions> workspaceSymbolProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(workspaceSymbolProviderKey,
                                                 workspaceSymbolProvider);
}

std::optional<std::variant<bool, CodeActionOptions>> ServerCapabilities::codeActionProvider() const
{
    const QJsonValue &provider = value(codeActionProviderKey);
    if (provider.isBool())
        return std::make_optional(std::variant<bool, CodeActionOptions>(provider.toBool()));
    if (provider.isObject()) {
        CodeActionOptions options(provider);
        if (options.isValid())
            return std::make_optional(std::variant<bool, CodeActionOptions>(options));
    }
    return std::nullopt;
}

std::optional<std::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::documentFormattingProvider() const
{
    using RetType = std::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(documentFormattingProviderKey);
    if (provider.isBool())
        return std::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return std::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return std::nullopt;
}

void ServerCapabilities::setDocumentFormattingProvider(
    const std::variant<bool, WorkDoneProgressOptions> &documentFormattingProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(documentFormattingProviderKey,
                                                 documentFormattingProvider);
}

std::optional<std::variant<bool, WorkDoneProgressOptions>>
ServerCapabilities::documentRangeFormattingProvider() const
{
    using RetType = std::variant<bool, WorkDoneProgressOptions>;
    const QJsonValue &provider = value(documentRangeFormattingProviderKey);
    if (provider.isBool())
        return std::make_optional(RetType(provider.toBool()));
    if (provider.isObject())
        return std::make_optional(RetType(WorkDoneProgressOptions(provider.toObject())));
    return std::nullopt;
}

void ServerCapabilities::setDocumentRangeFormattingProvider(
    std::variant<bool, WorkDoneProgressOptions> documentRangeFormattingProvider)
{
    insertVariant<bool, WorkDoneProgressOptions>(documentRangeFormattingProviderKey,
                                                 documentRangeFormattingProvider);
}

std::optional<std::variant<ServerCapabilities::RenameOptions, bool>> ServerCapabilities::renameProvider() const
{
    using RetType = std::variant<ServerCapabilities::RenameOptions, bool>;
    const QJsonValue &localValue = value(renameProviderKey);
    if (localValue.isBool())
        return RetType(localValue.toBool());
    if (localValue.isObject())
        return RetType(RenameOptions(localValue.toObject()));
    return std::nullopt;
}

void ServerCapabilities::setRenameProvider(std::variant<ServerCapabilities::RenameOptions, bool> renameProvider)
{
    insertVariant<RenameOptions, bool>(renameProviderKey, renameProvider);
}

std::optional<std::variant<bool, JsonObject>> ServerCapabilities::colorProvider() const
{
    using RetType = std::variant<bool, JsonObject>;
    const QJsonValue &localValue = value(colorProviderKey);
    if (localValue.isBool())
        return RetType(localValue.toBool());
    if (localValue.isObject())
        return RetType(JsonObject(localValue.toObject()));
    return std::nullopt;
}

void ServerCapabilities::setColorProvider(std::variant<bool, JsonObject> colorProvider)
{
    insertVariant<bool, JsonObject>(renameProviderKey, colorProvider);
}

std::optional<std::variant<QString, bool> >
ServerCapabilities::WorkspaceServerCapabilities::WorkspaceFoldersCapabilities::changeNotifications() const
{
    using RetType = std::variant<QString, bool>;
    const QJsonValue &change = value(changeNotificationsKey);
    if (change.isUndefined())
        return std::nullopt;
    return std::make_optional(change.isBool() ? RetType(change.toBool())
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

std::optional<std::variant<bool, QJsonObject>> SemanticTokensOptions::range() const
{
    using RetType = std::variant<bool, QJsonObject>;
    const QJsonValue &rangeOptions = value(rangeKey);
    if (rangeOptions.isBool())
        return RetType(rangeOptions.toBool());
    if (rangeOptions.isObject())
        return RetType(rangeOptions.toObject());
    return std::nullopt;
}

void SemanticTokensOptions::setRange(const std::variant<bool, QJsonObject> &range)
{
    insertVariant<bool, QJsonObject>(rangeKey, range);
}

std::optional<std::variant<bool, SemanticTokensOptions::FullSemanticTokenOptions>>
SemanticTokensOptions::full() const
{
    using RetType = std::variant<bool, SemanticTokensOptions::FullSemanticTokenOptions>;
    const QJsonValue &fullOptions = value(fullKey);
    if (fullOptions.isBool())
        return RetType(fullOptions.toBool());
    if (fullOptions.isObject())
        return RetType(FullSemanticTokenOptions(fullOptions.toObject()));
    return std::nullopt;
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
