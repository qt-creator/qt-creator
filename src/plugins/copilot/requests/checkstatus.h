// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <languageserverprotocol/jsonrpcmessages.h>
#include <languageserverprotocol/lsptypes.h>

namespace Copilot {

class CheckStatusParams : public LanguageServerProtocol::JsonObject
{
    static constexpr char optionsKey[] = "options";
    static constexpr char localChecksOnlyKey[] = "options";

public:
    using JsonObject::JsonObject;

    CheckStatusParams(bool localChecksOnly = false) { setLocalChecksOnly(localChecksOnly); }

    void setLocalChecksOnly(bool localChecksOnly)
    {
        QJsonObject options;
        options.insert(localChecksOnlyKey, localChecksOnly);
        setOptions(options);
    }

    void setOptions(QJsonObject options) { insert(optionsKey, options); }
};

class CheckStatusResponse : public LanguageServerProtocol::JsonObject
{
    static constexpr char userKey[] = "user";
    static constexpr char statusKey[] = "status";

public:
    using JsonObject::JsonObject;

    QString status() const { return typedValue<QString>(statusKey); }
    QString user() const { return typedValue<QString>(userKey); }
};

class CheckStatusRequest
    : public LanguageServerProtocol::Request<CheckStatusResponse, std::nullptr_t, CheckStatusParams>
{
public:
    explicit CheckStatusRequest(const CheckStatusParams &params)
        : Request(methodName, params)
    {}
    using Request::Request;
    constexpr static const char methodName[] = "checkStatus";
};

} // namespace Copilot
