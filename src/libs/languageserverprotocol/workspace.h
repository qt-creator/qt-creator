// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "jsonrpcmessages.h"

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT WorkSpaceFolderResult
    : public std::variant<QList<WorkSpaceFolder>, std::nullptr_t>
{
public:
    using variant::variant;
    using variant::operator=;
    operator const QJsonValue() const;
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkSpaceFolderRequest : public Request<
    WorkSpaceFolderResult, std::nullptr_t, std::nullptr_t>
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
    WorkspaceFoldersChangeEvent();

    QList<WorkSpaceFolder> added() const { return array<WorkSpaceFolder>(addedKey); }
    void setAdded(const QList<WorkSpaceFolder> &added) { insertArray(addedKey, added); }

    QList<WorkSpaceFolder> removed() const { return array<WorkSpaceFolder>(removedKey); }
    void setRemoved(const QList<WorkSpaceFolder> &removed) { insertArray(removedKey, removed); }

    bool isValid() const override { return contains(addedKey) && contains(removedKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeWorkspaceFoldersParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    WorkspaceFoldersChangeEvent event() const
    { return typedValue<WorkspaceFoldersChangeEvent>(eventKey); }
    void setEvent(const WorkspaceFoldersChangeEvent &event) { insert(eventKey, event); }

    bool isValid() const override { return contains(eventKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeWorkspaceFoldersNotification : public Notification<
        DidChangeWorkspaceFoldersParams>
{
public:
    explicit DidChangeWorkspaceFoldersNotification(const DidChangeWorkspaceFoldersParams &params);
    constexpr static const char methodName[] = "workspace/didChangeWorkspaceFolders";
    using Notification::Notification;
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeConfigurationParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QJsonValue settings() const { return value(settingsKey); }
    void setSettings(QJsonValue settings) { insert(settingsKey, settings); }

    bool isValid() const override { return contains(settingsKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeConfigurationNotification : public Notification<
        DidChangeConfigurationParams>
{
public:
    explicit DidChangeConfigurationNotification(const DidChangeConfigurationParams &params);
    using Notification::Notification;
    constexpr static const char methodName[] = "workspace/didChangeConfiguration";
};

class LANGUAGESERVERPROTOCOL_EXPORT ConfigurationParams : public JsonObject
{
public:
    using JsonObject::JsonObject;
    class LANGUAGESERVERPROTOCOL_EXPORT ConfigurationItem : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        std::optional<DocumentUri> scopeUri() const;
        void setScopeUri(const DocumentUri &scopeUri) { insert(scopeUriKey, scopeUri); }
        void clearScopeUri() { remove(scopeUriKey); }

        std::optional<QString> section() const { return optionalValue<QString>(sectionKey); }
        void setSection(const QString &section) { insert(sectionKey, section); }
        void clearSection() { remove(sectionKey); }

        bool isValid() const override { return contains(scopeUriKey); }
    };

    QList<ConfigurationItem> items() const { return array<ConfigurationItem>(itemsKey); }
    void setItems(const QList<ConfigurationItem> &items) { insertArray(itemsKey, items); }

    bool isValid() const override { return contains(itemsKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ConfigurationRequest
    : public Request<QJsonArray, std::nullptr_t, ConfigurationParams>
{
public:
    explicit ConfigurationRequest(const ConfigurationParams &params);
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

        bool isValid() const override { return contains(uriKey) && contains(typeKey); }
    };

    QList<FileEvent> changes() const { return array<FileEvent>(changesKey); }
    void setChanges(const QList<FileEvent> &changes) { insertArray(changesKey, changes); }

    bool isValid() const override { return contains(changesKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeWatchedFilesNotification : public Notification<
        DidChangeWatchedFilesParams>
{
public:
    explicit DidChangeWatchedFilesNotification(const DidChangeWatchedFilesParams &params);
    using Notification::Notification;
    constexpr static const char methodName[] = "workspace/didChangeWatchedFiles";
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkspaceSymbolParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString query() const { return typedValue<QString>(queryKey); }
    void setQuery(const QString &query) { insert(queryKey, query); }

    void setLimit(int limit) { insert("limit", limit); } // clangd extension

    bool isValid() const override { return contains(queryKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkspaceSymbolRequest : public Request<
        LanguageClientArray<SymbolInformation>, std::nullptr_t, WorkspaceSymbolParams>
{
public:
    explicit WorkspaceSymbolRequest(const WorkspaceSymbolParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "workspace/symbol";
};

class LANGUAGESERVERPROTOCOL_EXPORT ExecuteCommandParams : public JsonObject
{
public:
    explicit ExecuteCommandParams(const Command &command);
    explicit ExecuteCommandParams(const QJsonValue &value) : JsonObject(value) {}
    ExecuteCommandParams() : JsonObject() {}

    QString command() const { return typedValue<QString>(commandKey); }
    void setCommand(const QString &command) { insert(commandKey, command); }
    void clearCommand() { remove(commandKey); }

    std::optional<QJsonArray> arguments() const { return typedValue<QJsonArray>(argumentsKey); }
    void setArguments(const QJsonArray &arguments) { insert(argumentsKey, arguments); }
    void clearArguments() { remove(argumentsKey); }

    bool isValid() const override { return contains(commandKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ExecuteCommandRequest : public Request<
        QJsonValue, std::nullptr_t, ExecuteCommandParams>
{
public:
    explicit ExecuteCommandRequest(const ExecuteCommandParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "workspace/executeCommand";
};

class LANGUAGESERVERPROTOCOL_EXPORT ApplyWorkspaceEditParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    std::optional<QString> label() const { return optionalValue<QString>(labelKey); }
    void setLabel(const QString &label) { insert(labelKey, label); }
    void clearLabel() { remove(labelKey); }

    WorkspaceEdit edit() const { return typedValue<WorkspaceEdit>(editKey); }
    void setEdit(const WorkspaceEdit &edit) { insert(editKey, edit); }

    bool isValid() const override { return contains(editKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ApplyWorkspaceEditResult : public JsonObject
{
public:
    using JsonObject::JsonObject;

    bool applied() const { return typedValue<bool>(appliedKey); }
    void setApplied(bool applied) { insert(appliedKey, applied); }

    bool isValid() const override { return contains(appliedKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ApplyWorkspaceEditRequest : public Request<
        ApplyWorkspaceEditResult, std::nullptr_t, ApplyWorkspaceEditParams>
{
public:
    explicit ApplyWorkspaceEditRequest(const ApplyWorkspaceEditParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "workspace/applyEdit";
};

} // namespace LanguageClient
