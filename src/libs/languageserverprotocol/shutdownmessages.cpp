// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "shutdownmessages.h"

namespace LanguageServerProtocol {

constexpr const char ShutdownRequest::methodName[];
constexpr const char ExitNotification::methodName[];
ShutdownRequest::ShutdownRequest() : Request(methodName, nullptr) { }
ExitNotification::ExitNotification() : Notification(methodName) { }

} // namespace LanguageServerProtocol
