// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "checkstatus.h"

#include <languageserverprotocol/jsonrpcmessages.h>
#include <languageserverprotocol/lsptypes.h>

namespace Copilot {

using SignOutParams = LanguageServerProtocol::JsonObject;

class SignOutRequest
    : public LanguageServerProtocol::Request<CheckStatusResponse, std::nullptr_t, SignOutParams>
{
public:
    explicit SignOutRequest()
        : Request(methodName, {})
    {}
    using Request::Request;
    constexpr static const char methodName[] = "signOut";
};

} // namespace Copilot
