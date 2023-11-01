// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <languageserverprotocol/jsonrpcmessages.h>
#include <languageserverprotocol/lsptypes.h>

namespace Copilot {

using SignInInitiateParams = LanguageServerProtocol::JsonObject;

class SignInInitiateResponse : public LanguageServerProtocol::JsonObject
{
    static constexpr char verificationUriKey[] = "verificationUri";
    static constexpr char userCodeKey[] = "userCode";

public:
    using JsonObject::JsonObject;

public:
    QString verificationUri() const { return typedValue<QString>(verificationUriKey); }
    QString userCode() const { return typedValue<QString>(userCodeKey); }
};

class SignInInitiateRequest : public LanguageServerProtocol::Request<SignInInitiateResponse,
                                                                     std::nullptr_t,
                                                                     SignInInitiateParams>
{
public:
    explicit SignInInitiateRequest()
        : Request(methodName, {})
    {}
    using Request::Request;
    constexpr static const char methodName[] = "signInInitiate";
};

} // namespace Copilot
