// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <languageserverprotocol/lspjsonrpc.h>
#include <languageserverprotocol/lsptypes.h>

#include <QJsonArray>
#include <QJsonObject>

// The methods the copilot agent adds to the protocol, and the types they carry.

namespace Copilot {

struct CheckStatusParams
{
    bool localChecksOnly = false;
};

inline QJsonObject toJson(const CheckStatusParams &params)
{
    return {{"options", QJsonObject{{"localChecksOnly", params.localChecksOnly}}}};
}

struct Status
{
    QString status;
    QString user;
};

struct SignInConfirmParams
{
    QString userCode;
};

inline QJsonObject toJson(const SignInConfirmParams &params)
{
    return {{"userCode", params.userCode}};
}

struct SignInInitiateResult
{
    QString verificationUri;
    QString userCode;
};

struct Completion
{
    QString displayText;
    QString text;
    QString uuid;
    LanguageServerProtocol::Position position;
    LanguageServerProtocol::Range range;
};

struct Completions
{
    QList<Completion> completions;
};

struct GetCompletionParams
{
    QString uri;
    int version = 0;
    LanguageServerProtocol::Position position;
};

inline QJsonObject toJson(const GetCompletionParams &params)
{
    return {
        {"doc",
         QJsonObject{
             {"uri", params.uri},
             {"version", params.version},
             {"position", LanguageServerProtocol::toJson(params.position)}}}};
}

struct CheckStatusRequest
{
    static constexpr char method[] = "checkStatus";
    static constexpr bool isRequest = true;
    using Params = CheckStatusParams;
    using Result = Status;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

struct SignOutRequest
{
    static constexpr char method[] = "signOut";
    static constexpr bool isRequest = true;
    using Params = std::monostate;
    using Result = Status;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

struct SignInInitiateRequest
{
    static constexpr char method[] = "signInInitiate";
    static constexpr bool isRequest = true;
    using Params = std::monostate;
    using Result = SignInInitiateResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

struct SignInConfirmRequest
{
    static constexpr char method[] = "signInConfirm";
    static constexpr bool isRequest = true;
    using Params = SignInConfirmParams;
    using Result = Status;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

struct GetCompletionRequest
{
    static constexpr char method[] = "getCompletionsCycling";
    static constexpr bool isRequest = true;
    using Params = GetCompletionParams;
    using Result = Completions;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

} // namespace Copilot

namespace LanguageServerProtocol {

template<>
inline Utils::Result<Copilot::Status> fromJson<Copilot::Status>(const QJsonValue &val)
{
    const QJsonObject object = val.toObject();
    return Copilot::Status{object.value("status").toString(), object.value("user").toString()};
}

template<>
inline Utils::Result<Copilot::SignInInitiateResult> fromJson<Copilot::SignInInitiateResult>(
    const QJsonValue &val)
{
    const QJsonObject object = val.toObject();
    return Copilot::SignInInitiateResult{object.value("verificationUri").toString(),
                                         object.value("userCode").toString()};
}

template<>
inline Utils::Result<Copilot::Completions> fromJson<Copilot::Completions>(const QJsonValue &val)
{
    Copilot::Completions result;
    for (const QJsonValue &value : val.toObject().value("completions").toArray()) {
        const QJsonObject object = value.toObject();
        if (!object.contains("text") || !object.contains("range") || !object.contains("position"))
            continue;
        Copilot::Completion completion;
        completion.displayText = object.value("displayText").toString();
        completion.text = object.value("text").toString();
        completion.uuid = object.value("uuid").toString();
        const Utils::Result<Position> position = fromJson<Position>(object.value("position"));
        const Utils::Result<Range> range = fromJson<Range>(object.value("range"));
        if (!position || !range)
            continue;
        completion.position = *position;
        completion.range = *range;
        result.completions << completion;
    }
    return result;
}

} // namespace LanguageServerProtocol
