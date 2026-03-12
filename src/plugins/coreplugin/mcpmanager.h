// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/commandline.h>

#include <QString>
#include <QUrl>

namespace Core {

class CORE_EXPORT McpManager : public QObject
{
    Q_OBJECT
public:
    enum ConnectionType { Stdio, Sse, Streamable_Http };

    Q_ENUM(ConnectionType)

    struct ServerInfo
    {
        QString id;
        QString name;
        ConnectionType connectionType;
        std::variant<Utils::CommandLine, QUrl> launchInfo;
        QStringList httpHeaders;
    };

public:
    ~McpManager();

    static McpManager &instance();

    static bool registerMcpServer(ServerInfo info);
    static void removeMcpServer(const QString &id);
    static QList<ServerInfo> mcpServers();

signals:
    void mcpServersChanged();

private:
    McpManager();
};

void setupMcpManager();

} // namespace Core
