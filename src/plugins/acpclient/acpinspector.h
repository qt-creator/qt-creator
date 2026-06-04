// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/jsonrpcinspector.h>

namespace AcpClient::Internal {

using AcpLogMessage = Utils::JsonRpcLogMessage;

class AcpInspector : public Utils::JsonRpcInspector
{
public:
    AcpInspector();
};

} // namespace AcpClient::Internal
