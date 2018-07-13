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

#include "jsonrpcmessages.h"

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT WorkSpaceFolderRequest : public Request<
        Utils::variant<QList<WorkSpaceFolder>, Utils::nullopt_t>, LanguageClientNull, LanguageClientNull>
{
public:
    WorkSpaceFolderRequest();
    using Request::Request;
    constexpr static const char methodName[] = "workspace/workspaceFolders";

    bool parametersAreValid(QString * /*errorMessage*/) const override { return true; }
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkspaceFoldersChangeEvent : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QList<WorkSpaceFolder> added() const { return array<WorkSpaceFolder>(addedKey); }
    void setAdded(const QList<WorkSpaceFolder> &added) { insertArray(addedKey, added); }

    QList<WorkSpaceFolder> removed() const { return array<WorkSpaceFolder>(removedKey); }
    void setRemoved(const QList<WorkSpaceFolder> &removed) { insertArray(removedKey, removed); }

    bool isValid(QStringList *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeWorkspaceFoldersParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    WorkspaceFoldersChangeEvent event() const
    { return typedValue<WorkspaceFoldersChangeEvent>(eventKey); }
    void setEvent(const WorkspaceFoldersChangeEvent &event) { insert(eventKey, event); }

    bool isValid(QStringList *error) const override
    { return check<WorkspaceFoldersChangeEvent>(error, eventKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeWorkspaceFoldersNotification : public Notification<
        DidChangeWorkspaceFoldersParams>
{
public:
    DidChangeWorkspaceFoldersNotification(
            const DidChangeWorkspaceFoldersParams &params = DidChangeWorkspaceFoldersParams());
    constexpr static const char methodName[] = "workspace/didChangeWorkspaceFolders";
    using Notification::Notification;
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeConfigurationParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QJsonValue settings() const { return typedValue<QJsonValue>(settingsKey); }
    void setSettings(QJsonValue settings) { insert(settingsKey, settings); }

    bool isValid(QStringList *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeConfigurationNotification : public Notification<
        DidChangeConfigurationParams>
{
public:
    DidChangeConfigurationNotification(
            const DidChangeConfigurationParams &params = DidChangeConfigurationParams());
    using Notification::Notification;
    constexpr static const char methodName[] = "workspace/didChangeConfiguration";
};

class LANGUAGESERVERPROTOCOL_EXPORT ConfigurationParams : public JsonObject
{
public:
    using JsonObject::JsonObject;
    class ConfigureationItem : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        Utils::optional<QString> scopeUri() const { return optionalValue<QString>(scopeUriKey); }
        void setScopeUri(const QString &scopeUri) { insert(scopeUriKey, scopeUri); }
        void clearScopeUri() { remove(scopeUriKey); }

        Utils::optional<QString> section() const { return optionalValue<QString>(sectionKey); }
        void setSection(const QString &section) { insert(sectionKey, section); }
        void clearSection() { remove(sectionKey); }

        bool isValid(QStringList *error) const override;
    };

    QList<ConfigureationItem> items() const { return array<ConfigureationItem>(itemsKey); }
    void setItems(const QList<ConfigureationItem> &items) { insertArray(itemsKey, items); }

    bool isValid(QStringList *error) const override
    { return checkArray<ConfigureationItem>(error, itemsKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ConfigurationRequest : public Request<
        LanguageClientArray<QJsonValue>, LanguageClientNull, ConfigurationParams>
{
public:
    ConfigurationRequest(const ConfigurationParams &params = ConfigurationParams());
    using Request::Request;
    constexpr static const char methodName[] = "workspace/configuration";
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeWatchedFilesParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    class FileEvent : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        DocumentUri uri() const { return DocumentUri::fromProtocol(typedValue<QString>(uriKey)); }
        void setUri(const DocumentUri &uri) { insert(uriKey, uri); }

        int type() const { return typedValue<int>(typeKey); }
        void setType(int type) { insert(typeKey, type); }

        enum FileChangeType {
            Created = 1,
            Changed = 2,
            Deleted = 3
        };

        bool isValid(QStringList *error) const override
        { return check<QString>(error, uriKey) && check<int>(error, typeKey); }
    };

    QList<FileEvent> changes() const { return array<FileEvent>(changesKey); }
    void setChanges(const QList<FileEvent> &changes) { insertArray(changesKey, changes); }

    bool isValid(QStringList *error) const override
    { return checkArray<FileEvent>(error, changesKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeWatchedFilesNotification : public Notification<
        DidChangeWatchedFilesParams>
{
public:
    DidChangeWatchedFilesNotification(const DidChangeWatchedFilesParams &params = DidChangeWatchedFilesParams());
    using Notification::Notification;
    constexpr static const char methodName[] = "workspace/didChangeWatchedFiles";
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkspaceSymbolParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString query() const { return typedValue<QString>(queryKey); }
    void setQuery(const QString &query) { insert(queryKey, query); }

    bool isValid(QStringList *error) const override { return check<QString>(error, queryKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkspaceSymbolRequest : public Request<
        LanguageClientArray<SymbolInformation>, LanguageClientNull, WorkspaceSymbolParams>
{
public:
    WorkspaceSymbolRequest(const WorkspaceSymbolParams &params = WorkspaceSymbolParams());
    using Request::Request;
    constexpr static const char methodName[] = "workspace/symbol";
};

class LANGUAGESERVERPROTOCOL_EXPORT ExecuteCommandParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString command() const { return typedValue<QString>(commandKey); }
    void setCommand(const QString &command) { insert(commandKey, command); }

    Utils::optional<QList<QJsonValue>> arguments() const
    { return optionalArray<QJsonValue>(argumentsKey); }
    void setArguments(const QList<QJsonValue> &arguments)
    { insertArray(argumentsKey, arguments); }
    void clearArguments() { remove(argumentsKey); }

    bool isValid(QStringList *error) const override
    {
        return check<QString>(error, commandKey)
                && checkOptionalArray<QJsonValue>(error, argumentsKey);
    }
};

class LANGUAGESERVERPROTOCOL_EXPORT ExecuteCommandRequest : public Request<
        QJsonValue, LanguageClientNull, ExecuteCommandParams>
{
public:
    ExecuteCommandRequest(const ExecuteCommandParams &params = ExecuteCommandParams());
    using Request::Request;
    constexpr static const char methodName[] = "workspace/executeCommand";
};

class LANGUAGESERVERPROTOCOL_EXPORT ApplyWorkspaceEditParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    Utils::optional<QString> label() const { return optionalValue<QString>(labelKey); }
    void setLabel(const QString &label) { insert(labelKey, label); }
    void clearLabel() { remove(labelKey); }

    WorkspaceEdit edit() const { return typedValue<WorkspaceEdit>(editKey); }
    void setEdit(const WorkspaceEdit &edit) { insert(editKey, edit); }

    bool isValid(QStringList *error) const override
    { return check<WorkspaceEdit>(error, editKey) && checkOptional<QString>(error, labelKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ApplyWorkspaceEditResponse : public JsonObject
{
public:
    using JsonObject::JsonObject;

    bool applied() const { return typedValue<bool>(appliedKey); }
    void setApplied(bool applied) { insert(appliedKey, applied); }

    bool isValid(QStringList *error) const override { return check<bool>(error, appliedKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ApplyWorkspaceEditRequest : public Request<
        ApplyWorkspaceEditResponse, LanguageClientNull, ApplyWorkspaceEditParams>
{
public:
    ApplyWorkspaceEditRequest(const ApplyWorkspaceEditParams &params = ApplyWorkspaceEditParams());
    using Request::Request;
    constexpr static const char methodName[] = "workspace/applyEdit";
};

} // namespace LanguageClient
