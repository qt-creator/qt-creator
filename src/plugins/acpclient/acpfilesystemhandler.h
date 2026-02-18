// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <acp/acp.h>

#include <QJsonValue>
#include <QObject>

namespace AcpClient::Internal {

class AcpClientObject;

class AcpFilesystemHandler : public QObject
{
    Q_OBJECT

public:
    explicit AcpFilesystemHandler(AcpClientObject *client, QObject *parent = nullptr);

private:
    void handleReadTextFile(const QJsonValue &id, const Acp::ReadTextFileRequest &request);
    void handleWriteTextFile(const QJsonValue &id, const Acp::WriteTextFileRequest &request);

    AcpClientObject *m_client;
};

} // namespace AcpClient::Internal
