// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/jsonrpcinspector.h>

#include <QJsonObject>
#include <QMap>

namespace AcpClient::Internal {

using AcpLogMessage = Utils::JsonRpcLogMessage;

class AcpInspector : public Utils::JsonRpcInspector
{
public:
    AcpInspector();

    // Store the agent capabilities for an endpoint and refresh the "Capabilities" tab.
    void setCapabilities(const QString &endpoint, const QJsonObject &capabilities);
    QJsonObject capabilities(const QString &endpoint) const;

private:
    QMap<QString, QJsonObject> m_capabilities;
};

} // namespace AcpClient::Internal
