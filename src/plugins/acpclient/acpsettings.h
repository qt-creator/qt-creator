// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/commandline.h>
#include <utils/environment.h>

#include <QFuture>
#include <QIcon>
#include <QObject>

namespace AcpClient::Internal {

class AcpSettings : public QObject
{
    Q_OBJECT
public:
    struct ServerInfo
    {
        QString id;
        QString name;
        QString iconUrl;
        Utils::CommandLine launchCommand;
        Utils::EnvironmentChanges envChanges;
    };

    static QFuture<QIcon> iconForUrl(const QString &url);

    ~AcpSettings() override;

    static AcpSettings &instance();

    static QList<ServerInfo> servers();

signals:
    void serversChanged();

private:
    AcpSettings();
};

void setupAcpSettings();
void prefetchAcpRegistry();

} // namespace AcpClient::Internal
