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
    McpManager();
    ~McpManager();

    static McpManager &instance();

    bool registerMcpServer(ServerInfo info);
    void removeMcpServer(const QString &id);
    QList<ServerInfo> mcpServers() const;

signals:
    void mcpServersChanged();

private:
    std::unique_ptr<class McpManagerPrivate> d;
};

void setupMcpManager();

} // namespace Core
