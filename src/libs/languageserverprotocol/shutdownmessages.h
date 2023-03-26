// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "jsonrpcmessages.h"

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT ShutdownRequest : public Request<
        std::nullptr_t, std::nullptr_t, std::nullptr_t>
{
public:
    ShutdownRequest();
    using Request::Request;
    constexpr static const char methodName[] = "shutdown";
};

class LANGUAGESERVERPROTOCOL_EXPORT ExitNotification : public Notification<std::nullptr_t>
{
public:
    ExitNotification();
    using Notification::Notification;
    constexpr static const char methodName[] = "exit";

    bool parametersAreValid(QString * /*errorMessage*/) const final { return true; }
};

} // namespace LanguageClient
