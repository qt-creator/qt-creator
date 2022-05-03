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

#include "clientcapabilities.h"
#include "jsonrpcmessages.h"
#include "lsptypes.h"
#include "servercapabilities.h"

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT Trace
{
public:
    enum Values
    {
        off,
        messages,
        verbose
    };

    Trace() = default;
    Trace(Values val) : m_value(val) {}
    Trace(QString val) : Trace(fromString(val)) {}

    static Trace fromString(const QString& val);
    QString toString() const;

private:
    Values m_value = off;
};

class LANGUAGESERVERPROTOCOL_EXPORT ClientInfo : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString name() const { return typedValue<QString>(nameKey); }
    void setName(const QString &name) { insert(nameKey, name); }

    Utils::optional<QString> version() const { return optionalValue<QString>(versionKey); }
    void setVersion(const QString &version) { insert(versionKey, version); }
    void clearVersion() { remove(versionKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT InitializeParams : public JsonObject
{
public:
    InitializeParams();
    using JsonObject::JsonObject;

    /*
     * The process Id of the parent process that started
     * the server. Is null if the process has not been started by another process.
     * If the parent process is not alive then the server should exit (see exit notification)
     * its process.
     */
    LanguageClientValue<int> processId() const { return clientValue<int>(processIdKey); }
    void setProcessId(const LanguageClientValue<int> &id) { insert(processIdKey, id); }

    /*
     * The rootPath of the workspace. Is null
     * if no folder is open.
     *
     * @deprecated in favor of rootUri.
     */
    Utils::optional<LanguageClientValue<QString>> rootPath() const
    { return optionalClientValue<QString>(rootPathKey); }
    void setRootPath(const LanguageClientValue<QString> &path)
    { insert(rootPathKey, path); }
    void clearRootPath() { remove(rootPathKey); }

    /*
     * The rootUri of the workspace. Is null if no
     * folder is open. If both `rootPath` and `rootUri` are set
     * `rootUri` wins.
     */
    LanguageClientValue<DocumentUri> rootUri() const
    { return clientValue<QString>(rootUriKey).transform<DocumentUri>(); }
    void setRootUri(const LanguageClientValue<DocumentUri> &uri)
    { insert(rootUriKey, uri); }

    // User provided initialization options.
    Utils::optional<QJsonObject> initializationOptions() const;
    void setInitializationOptions(const QJsonObject &options)
    { insert(initializationOptionsKey, options); }
    void clearInitializationOptions() { remove(initializationOptionsKey); }

    // The capabilities provided by the client (editor or tool)
    ClientCapabilities capabilities() const { return typedValue<ClientCapabilities>(capabilitiesKey); }
    void setCapabilities(const ClientCapabilities &capabilities)
    { insert(capabilitiesKey, capabilities); }

    // The initial trace setting. If omitted trace is disabled ('off').
    Utils::optional<Trace> trace() const;
    void setTrace(Trace trace) { insert(traceKey, trace.toString()); }
    void clearTrace() { remove(traceKey); }

    /*
     * The workspace folders configured in the client when the server starts.
     * This property is only available if the client supports workspace folders.
     * It can be `null` if the client supports workspace folders but none are
     * configured.
     *
     * Since 3.6.0
     */
    Utils::optional<LanguageClientArray<WorkSpaceFolder>> workspaceFolders() const
    { return optionalClientArray<WorkSpaceFolder>(workspaceFoldersKey); }
    void setWorkSpaceFolders(const LanguageClientArray<WorkSpaceFolder> &folders)
    { insert(workspaceFoldersKey, folders.toJson()); }
    void clearWorkSpaceFolders() { remove(workspaceFoldersKey); }

    Utils::optional<ClientInfo> clientInfo() const { return optionalValue<ClientInfo>(clientInfoKey); }
    void setClientInfo(const ClientInfo &clientInfo) { insert(clientInfoKey, clientInfo); }
    void clearClientInfo() { remove(clientInfoKey); }

    bool isValid() const override
    { return contains(processIdKey) && contains(rootUriKey) && contains(capabilitiesKey); }
};

using InitializedParams = JsonObject;

class LANGUAGESERVERPROTOCOL_EXPORT InitializeNotification : public Notification<InitializedParams>
{
public:
    explicit InitializeNotification(const InitializedParams &params);
    using Notification::Notification;
    constexpr static const char methodName[] = "initialized";

    bool parametersAreValid(QString * /*errorMessage*/) const final { return true; }
};

class LANGUAGESERVERPROTOCOL_EXPORT ServerInfo : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString name() const { return typedValue<QString>(nameKey); }
    Utils::optional<QString> version() const { return optionalValue<QString>(versionKey); }

    bool isValid() const override { return contains(nameKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT InitializeResult : public JsonObject
{
public:
    using JsonObject::JsonObject;

    ServerCapabilities capabilities() const
    { return typedValue<ServerCapabilities>(capabilitiesKey); }
    void setCapabilities(const ServerCapabilities &capabilities)
    { insert(capabilitiesKey, capabilities); }

    Utils::optional<ServerInfo> serverInfo() const
    { return optionalValue<ServerInfo>(serverInfoKey); }

    bool isValid() const override { return contains(capabilitiesKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT InitializeError : public JsonObject
{
public:
    using JsonObject::JsonObject;

    /*
     * Indicates whether the client execute the following retry logic:
     * (1) show the message provided by the ResponseError to the user
     * (2) user selects retry or cancel
     * (3) if user selected retry the initialize method is sent again.
     */
    bool retry() const { return typedValue<bool>(retryKey); }
    void setRetry(const bool &retry) { insert(retryKey, retry); }

    bool isValid() const override { return contains(retryKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT InitializeRequest : public Request<
        InitializeResult, InitializeError, InitializeParams>
{
public:
    explicit InitializeRequest(const InitializeParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "initialize";
};

} // namespace LanguageClient
