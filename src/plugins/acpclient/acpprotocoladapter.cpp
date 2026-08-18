// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpprotocoladapter.h"

namespace AcpClient::Internal {

AcpProtocolAdapter::AcpProtocolAdapter(AcpClientObject *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
}

} // namespace AcpClient::Internal
