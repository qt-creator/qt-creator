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

Utils::optional<SemanticTokensOptions> ServerCapabilities::semanticTokensProvider() const
{
    return optionalValue<SemanticTokensOptions>(semanticTokensProviderKey);
}

void ServerCapabilities::setSemanticTokensProvider(
    const SemanticTokensOptions &semanticTokensProvider)
{
    insert(semanticTokensProviderKey, semanticTokensProvider);
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
        if (options.isValid())
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

Utils::optional<Utils::variant<bool, QJsonObject>> SemanticTokensOptions::range() const
{
    using RetType = Utils::variant<bool, QJsonObject>;
    const QJsonValue &rangeOptions = value(rangeKey);
    if (rangeOptions.isBool())
        return RetType(rangeOptions.toBool());
    if (rangeOptions.isObject())
        return RetType(rangeOptions.toObject());
    return Utils::nullopt;
}

void SemanticTokensOptions::setRange(const Utils::variant<bool, QJsonObject> &range)
{
    insertVariant<bool, QJsonObject>(rangeKey, range);
}

Utils::optional<Utils::variant<bool, SemanticTokensOptions::FullSemanticTokenOptions>>
SemanticTokensOptions::full() const
{
    using RetType = Utils::variant<bool, SemanticTokensOptions::FullSemanticTokenOptions>;
    const QJsonValue &fullOptions = value(fullKey);
    if (fullOptions.isBool())
        return RetType(fullOptions.toBool());
    if (fullOptions.isObject())
        return RetType(FullSemanticTokenOptions(fullOptions.toObject()));
    return Utils::nullopt;
}

void SemanticTokensOptions::setFull(
    const Utils::variant<bool, SemanticTokensOptions::FullSemanticTokenOptions> &full)
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
