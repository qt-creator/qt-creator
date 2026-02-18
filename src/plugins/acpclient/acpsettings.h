// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/commandline.h>
#include <utils/environment.h>

#include <QObject>
#include <QString>

namespace AcpClient::Internal {

class AcpSettings : public QObject
{
    Q_OBJECT
public:
    enum ConnectionType { Stdio, Tcp };
    Q_ENUM(ConnectionType)

    struct ServerInfo
    {
        QString id;
        QString name;
        ConnectionType connectionType = Stdio;
        // Stdio: CommandLine, Tcp: QPair<host, port>
        std::variant<Utils::CommandLine, QPair<QString, quint16>> launchInfo;
        Utils::EnvironmentChanges envChanges; // Stdio only
    };

    ~AcpSettings();

    static AcpSettings &instance();

    static QList<ServerInfo> servers();

signals:
    void serversChanged();

private:
    AcpSettings();
};

void setupAcpSettings();

} // namespace AcpClient::Internal
