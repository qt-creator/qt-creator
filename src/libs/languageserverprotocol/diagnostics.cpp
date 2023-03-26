// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "diagnostics.h"

namespace LanguageServerProtocol {

constexpr const char PublishDiagnosticsNotification::methodName[];

PublishDiagnosticsNotification::PublishDiagnosticsNotification(const PublishDiagnosticsParams &params)
    : Notification(methodName, params) { }

} // namespace LanguageServerProtocol
