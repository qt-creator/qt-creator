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
    if (auto val = Utils::get_if<int>(&textDocumentSync))
        insert(textDocumentSyncKey, *val);
    else if (auto val = Utils::get_if<TextDocumentSyncOptions>(&textDocumentSync))
        insert(textDocumentSyncKey, *val);
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

Utils::optional<Utils::variant<bool, ServerCapabilities::RegistrationOptions>>
ServerCapabilities::typeDefinitionProvider() const
{
    using RetType = Utils::variant<bool, ServerCapabilities::RegistrationOptions>;
    QJsonValue provider = value(typeDefinitionProviderKey);
    if (provider.isUndefined() || !(provider.isBool() || provider.isObject()))
        return Utils::nullopt;
    return Utils::make_optional(provider.isBool() ? RetType(provider.toBool())
                                                  : RetType(RegistrationOptions(provider.toObject())));
}

void ServerCapabilities::setTypeDefinitionProvider(
        const Utils::variant<bool, ServerCapabilities::RegistrationOptions> &typeDefinitionProvider)
{
    if (auto activated = Utils::get_if<bool>(&typeDefinitionProvider))
        insert(typeDefinitionProviderKey, *activated);
    else if (auto options = Utils::get_if<RegistrationOptions>(&typeDefinitionProvider))
        insert(typeDefinitionProviderKey, *options);
}

Utils::optional<Utils::variant<bool, ServerCapabilities::RegistrationOptions>>
ServerCapabilities::implementationProvider() const
{
    using RetType = Utils::variant<bool, ServerCapabilities::RegistrationOptions>;
    QJsonValue provider = value(implementationProviderKey);
    if (provider.isUndefined() || !(provider.isBool() || provider.isObject()))
        return Utils::nullopt;
    return Utils::make_optional(provider.isBool() ? RetType(provider.toBool())
                                                  : RetType(RegistrationOptions(provider.toObject())));
}

void ServerCapabilities::setImplementationProvider(
        const Utils::variant<bool, ServerCapabilities::RegistrationOptions> &implementationProvider)
{
    if (Utils::holds_alternative<bool>(implementationProvider))
        insert(implementationProviderKey, Utils::get<bool>(implementationProvider));
    else
        insert(implementationProviderKey, Utils::get<RegistrationOptions>(implementationProvider));
}

bool ServerCapabilities::isValid(QStringList *error) const
{
    return checkOptional<TextDocumentSyncOptions, int>(error, textDocumentSyncKey)
            && checkOptional<bool>(error, hoverProviderKey)
            && checkOptional<CompletionOptions>(error, completionProviderKey)
            && checkOptional<SignatureHelpOptions>(error, signatureHelpProviderKey)
            && checkOptional<bool>(error, definitionProviderKey)
            && checkOptional<bool, RegistrationOptions>(error, typeDefinitionProviderKey)
            && checkOptional<bool, RegistrationOptions>(error, implementationProviderKey)
            && checkOptional<bool>(error, referenceProviderKey)
            && checkOptional<bool>(error, documentHighlightProviderKey)
            && checkOptional<bool>(error, documentSymbolProviderKey)
            && checkOptional<bool>(error, workspaceSymbolProviderKey)
            && checkOptional<bool>(error, codeActionProviderKey)
            && checkOptional<CodeLensOptions>(error, codeLensProviderKey)
            && checkOptional<bool>(error, documentFormattingProviderKey)
            && checkOptional<bool>(error, documentRangeFormattingProviderKey)
            && checkOptional<bool>(error, renameProviderKey)
            && checkOptional<DocumentLinkOptions>(error, documentLinkProviderKey)
            && checkOptional<TextDocumentRegistrationOptions>(error, colorProviderKey)
            && checkOptional<ExecuteCommandOptions>(error, executeCommandProviderKey)
            && checkOptional<WorkspaceServerCapabilities>(error, workspaceKey);
}

Utils::optional<Utils::variant<QString, bool> >
ServerCapabilities::WorkspaceServerCapabilities::WorkspaceFoldersCapabilities::changeNotifications() const
{
    using RetType = Utils::variant<QString, bool>;
    QJsonValue provider = value(implementationProviderKey);
    if (provider.isUndefined())
        return Utils::nullopt;
    return Utils::make_optional(provider.isBool() ? RetType(provider.toBool())
                                                  : RetType(provider.toString()));
}

void ServerCapabilities::WorkspaceServerCapabilities::WorkspaceFoldersCapabilities::setChangeNotifications(
        Utils::variant<QString, bool> changeNotifications)
{
    if (auto val = Utils::get_if<bool>(&changeNotifications))
        insert(changeNotificationsKey, *val);
    else if (auto val = Utils::get_if<QString>(&changeNotifications))
        insert(changeNotificationsKey, *val);
}

bool ServerCapabilities::WorkspaceServerCapabilities::WorkspaceFoldersCapabilities::isValid(QStringList *error) const
{
    return checkOptional<bool>(error, supportedKey)
            && checkOptional<QString, bool>(error, changeNotificationsKey);
}

bool TextDocumentRegistrationOptions::filterApplies(const Utils::FileName &fileName,
                                                    const Utils::MimeType &mimeType) const
{
    const LanguageClientArray<DocumentFilter> &selector = documentSelector();
    return selector.isNull()
            || selector.toList().isEmpty()
            || Utils::anyOf(selector.toList(), [&](auto filter){
        return filter.applies(fileName, mimeType);
    });
}

bool TextDocumentSyncOptions::isValid(QStringList *error) const
{
    return checkOptional<bool>(error, openCloseKey)
            && checkOptional<int>(error, changeKey)
            && checkOptional<bool>(error, willSaveKey)
            && checkOptional<bool>(error, willSaveWaitUntilKey)
            && checkOptional<SaveOptions>(error, saveKey);
}

} // namespace LanguageServerProtocol
