// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "checkstatus.h"

#include <languageserverprotocol/jsonrpcmessages.h>
#include <languageserverprotocol/lsptypes.h>

namespace Copilot {

class SignInConfirmParams : public LanguageServerProtocol::JsonObject
{
    static constexpr char userCodeKey[] = "userCode";

public:
    using JsonObject::JsonObject;

    SignInConfirmParams(const QString &userCode) { setUserCode(userCode); }

    void setUserCode(const QString &userCode) { insert(userCodeKey, userCode); }
};

class SignInConfirmRequest
    : public LanguageServerProtocol::Request<CheckStatusResponse, std::nullptr_t, SignInConfirmParams>
{
public:
    explicit SignInConfirmRequest(const QString &userCode)
        : Request(methodName, {userCode})
    {}
    using Request::Request;
    constexpr static const char methodName[] = "signInConfirm";
};

} // namespace Copilot
