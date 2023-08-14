// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "checkstatus.h"

#include <languageserverprotocol/jsonrpcmessages.h>
#include <languageserverprotocol/lsptypes.h>

namespace Copilot {

class EditorPluginInfo : public LanguageServerProtocol::JsonObject
{
    static constexpr char version[] = "version";
    static constexpr char name[] = "name";

public:
    using JsonObject::JsonObject;

    EditorPluginInfo(const QString &version, const QString &name)
    {
        setEditorVersion(version);
        setEditorName(name);
    }

    void setEditorVersion(const QString &v) { insert(version, v); }
    void setEditorName(const QString &n) { insert(name, n); }
};

class EditorInfo : public LanguageServerProtocol::JsonObject
{
    static constexpr char version[] = "version";
    static constexpr char name[] = "name";

public:
    using JsonObject::JsonObject;

    EditorInfo(const QString &version, const QString &name)
    {
        setEditorVersion(version);
        setEditorName(name);
    }

    void setEditorVersion(const QString &v) { insert(version, v); }
    void setEditorName(const QString &n) { insert(name, n); }
};

class NetworkProxy : public LanguageServerProtocol::JsonObject
{
    static constexpr char host[] = "host";
    static constexpr char port[] = "port";
    static constexpr char user[] = "username";
    static constexpr char password[] = "password";
    static constexpr char rejectUnauthorized[] = "rejectUnauthorized";

public:
    using JsonObject::JsonObject;

    NetworkProxy(const QString &host,
                 int port,
                 const QString &user,
                 const QString &password,
                 bool rejectUnauthorized)
    {
        setHost(host);
        setPort(port);
        setUser(user);
        setPassword(password);
        setRejectUnauthorized(rejectUnauthorized);
    }

    void insertIfNotEmpty(const std::string_view key, const QString &value)
    {
        if (!value.isEmpty())
            insert(key, value);
    }

    void setHost(const QString &h) { insert(host, h); }
    void setPort(int p) { insert(port, p); }
    void setUser(const QString &u) { insertIfNotEmpty(user, u); }
    void setPassword(const QString &p) { insertIfNotEmpty(password, p); }
    void setRejectUnauthorized(bool r) { insert(rejectUnauthorized, r); }
};

class SetEditorInfoParams : public LanguageServerProtocol::JsonObject
{
    static constexpr char editorInfo[] = "editorInfo";
    static constexpr char editorPluginInfo[] = "editorPluginInfo";
    static constexpr char networkProxy[] = "networkProxy";

public:
    using JsonObject::JsonObject;

    SetEditorInfoParams(const EditorInfo &editorInfo, const EditorPluginInfo &editorPluginInfo)
    {
        setEditorInfo(editorInfo);
        setEditorPluginInfo(editorPluginInfo);
    }

    SetEditorInfoParams(const EditorInfo &editorInfo,
                        const EditorPluginInfo &editorPluginInfo,
                        const NetworkProxy &networkProxy)
    {
        setEditorInfo(editorInfo);
        setEditorPluginInfo(editorPluginInfo);
        setNetworkProxy(networkProxy);
    }

    void setEditorInfo(const EditorInfo &info) { insert(editorInfo, info); }
    void setEditorPluginInfo(const EditorPluginInfo &info) { insert(editorPluginInfo, info); }
    void setNetworkProxy(const NetworkProxy &proxy) { insert(networkProxy, proxy); }
};

class SetEditorInfoRequest
    : public LanguageServerProtocol::Request<CheckStatusResponse, std::nullptr_t, SetEditorInfoParams>
{
public:
    explicit SetEditorInfoRequest(const SetEditorInfoParams &params)
        : Request(methodName, params)
    {}
    using Request::Request;
    constexpr static const char methodName[] = "setEditorInfo";
};

} // namespace Copilot
