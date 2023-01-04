// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "client.h"

namespace LanguageServerProtocol {

constexpr const char RegisterCapabilityRequest::methodName[];
constexpr const char UnregisterCapabilityRequest::methodName[];

RegisterCapabilityRequest::RegisterCapabilityRequest(const RegistrationParams &params)
    : Request(methodName, params) { }

UnregisterCapabilityRequest::UnregisterCapabilityRequest(const UnregistrationParams &params)
    : UnregisterCapabilityRequest(methodName, params) { }

} // namespace LanguageServerProtocol
