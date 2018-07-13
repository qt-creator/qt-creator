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

#include "clientcapabilities.h"

namespace LanguageServerProtocol {

SymbolCapabilities::SymbolKindCapabilities::SymbolKindCapabilities()
{
    setValueSet({SymbolKind::File, SymbolKind::Module, SymbolKind::Namespace,
                 SymbolKind::Package, SymbolKind::Class, SymbolKind::Method,
                 SymbolKind::Property, SymbolKind::Field, SymbolKind::Constructor,
                 SymbolKind::Enum, SymbolKind::Interface, SymbolKind::Function,
                 SymbolKind::Variable, SymbolKind::Constant, SymbolKind::String,
                 SymbolKind::Number, SymbolKind::Boolean, SymbolKind::Array,
                 SymbolKind::Object, SymbolKind::Key, SymbolKind::Null,
                 SymbolKind::EnumMember, SymbolKind::Struct, SymbolKind::Event,
                 SymbolKind::Operator, SymbolKind::TypeParameter});
}

Utils::optional<QList<SymbolKind> > SymbolCapabilities::SymbolKindCapabilities::valueSet() const
{
    Utils::optional<QList<int>> array = optionalArray<int>(valueSetKey);
    if (!array)
        return Utils::nullopt;
    return Utils::make_optional(Utils::transform(array.value(), [] (int value) {
        return static_cast<SymbolKind>(value);
    }));
}

void SymbolCapabilities::SymbolKindCapabilities::setValueSet(const QList<SymbolKind> &valueSet)
{
    insert(valueSetKey, enumArrayToJsonArray<SymbolKind>(valueSet));
}


ClientCapabilities::ClientCapabilities()
{
    setTextDocument(TextDocumentClientCapabilities());
    setWorkspace(WorkspaceClientCapabilities());
}

bool ClientCapabilities::isValid(QStringList *error) const
{
    return checkOptional<WorkspaceClientCapabilities>(error, workspaceKey)
            && checkOptional<TextDocumentClientCapabilities>(error, textDocumentKey);
}

WorkspaceClientCapabilities::WorkspaceClientCapabilities()
{
    setWorkspaceFolders(true);
}

bool WorkspaceClientCapabilities::isValid(QStringList *error) const
{
    return checkOptional<bool>(error,applyEditKey)
            && checkOptional<WorkspaceEditCapabilities>(error,workspaceEditKey)
            && checkOptional<DynamicRegistrationCapabilities>(error,didChangeConfigurationKey)
            && checkOptional<DynamicRegistrationCapabilities>(error,didChangeWatchedFilesKey)
            && checkOptional<SymbolCapabilities>(error,symbolKey)
            && checkOptional<DynamicRegistrationCapabilities>(error,executeCommandKey)
            && checkOptional<bool>(error,workspaceFoldersKey)
            && checkOptional<bool>(error,configurationKey);
}

TextDocumentClientCapabilities::SynchronizationCapabilities::SynchronizationCapabilities()
{
    setDynamicRegistration(true);
    setWillSave(true);
    setWillSaveWaitUntil(false);
    setDidSave(true);
}

bool TextDocumentClientCapabilities::SynchronizationCapabilities::isValid(QStringList *error) const
{
    return DynamicRegistrationCapabilities::isValid(error)
            && checkOptional<bool>(error, willSaveKey)
            && checkOptional<bool>(error, willSaveWaitUntilKey)
            && checkOptional<bool>(error, didSaveKey);
}

TextDocumentClientCapabilities::TextDocumentClientCapabilities()
{
    setSynchronization(SynchronizationCapabilities());
    setDocumentSymbol(SymbolCapabilities());
    setCompletion(CompletionCapabilities());
}

bool TextDocumentClientCapabilities::isValid(QStringList *error) const
{
    return checkOptional<SynchronizationCapabilities>(error, synchronizationKey)
       && checkOptional<CompletionCapabilities>(error, completionKey)
       && checkOptional<HoverCapabilities>(error, hoverKey)
       && checkOptional<SignatureHelpCapabilities>(error, signatureHelpKey)
       && checkOptional<DynamicRegistrationCapabilities>(error, referencesKey)
       && checkOptional<DynamicRegistrationCapabilities>(error, documentHighlightKey)
       && checkOptional<SymbolCapabilities>(error, documentSymbolKey)
       && checkOptional<DynamicRegistrationCapabilities>(error, formattingKey)
       && checkOptional<DynamicRegistrationCapabilities>(error, rangeFormattingKey)
       && checkOptional<DynamicRegistrationCapabilities>(error, onTypeFormattingKey)
       && checkOptional<DynamicRegistrationCapabilities>(error, definitionKey)
       && checkOptional<DynamicRegistrationCapabilities>(error, typeDefinitionKey)
       && checkOptional<DynamicRegistrationCapabilities>(error, implementationKey)
       && checkOptional<DynamicRegistrationCapabilities>(error, codeActionKey)
       && checkOptional<DynamicRegistrationCapabilities>(error, codeLensKey)
       && checkOptional<DynamicRegistrationCapabilities>(error, documentLinkKey)
       && checkOptional<DynamicRegistrationCapabilities>(error, colorProviderKey)
       && checkOptional<DynamicRegistrationCapabilities>(error, renameKey);
}

SymbolCapabilities::SymbolCapabilities()
{
    setSymbolKind(SymbolKindCapabilities());
}

bool SymbolCapabilities::isValid(QStringList *error) const
{
    return DynamicRegistrationCapabilities::isValid(error)
            && checkOptional<SymbolKindCapabilities>(error, symbolKindKey);
}

TextDocumentClientCapabilities::CompletionCapabilities::CompletionCapabilities()
{
    setDynamicRegistration(true);
    setCompletionItem(CompletionItemCapbilities());
    setCompletionItemKind(CompletionItemKindCapabilities());
}

bool TextDocumentClientCapabilities::CompletionCapabilities::isValid(QStringList *error) const
{
    return DynamicRegistrationCapabilities::isValid(error)
            && checkOptional<CompletionItemCapbilities>(error, completionItemKey)
            && checkOptional<CompletionItemKindCapabilities>(error, completionItemKindKey)
            && checkOptional<bool>(error, contextSupportKey);
}

bool TextDocumentClientCapabilities::HoverCapabilities::isValid(QStringList *error) const
{
    return DynamicRegistrationCapabilities::isValid(error)
            && checkOptionalArray<int>(error, contentFormatKey);
}

bool TextDocumentClientCapabilities::SignatureHelpCapabilities::isValid(QStringList *error) const
{
    return DynamicRegistrationCapabilities::isValid(error)
            && checkOptional<SignatureHelpCapabilities>(error, signatureInformationKey);
}

} // namespace LanguageServerProtocol
