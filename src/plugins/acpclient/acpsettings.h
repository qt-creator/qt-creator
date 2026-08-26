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

    struct RegistryAgent
    {
        QString id;
        QString name;
        QString description;
        QString iconUrl;
    };

    static QList<ServerInfo> servers();
    static bool hasServers();

    static bool isRegistryAvailable();
    // Fetches the registry unless it is available already. Answered by
    // registryFetched(), so a fetch that failed can be asked for again.
    static void fetchRegistry();
    // Registry agents that are not configured as a server yet.
    static QList<RegistryAgent> unconfiguredRegistryAgents();
    static void addServerFromRegistry(const QString &registryId);

signals:
    void serversChanged();
    void registryFetched(bool success);

private:
    AcpSettings();
};

// Terms and conditions covering the use of the ACP chat. They only apply to commercial
// users, which is what an installed and enabled licensechecker plugin marks. The accepted
// version is stored, so raising the shipped version asks again.
bool acpTermsPending();
bool acpTermsAccepted();
void setAcpTermsAccepted(bool accepted);

void setupAcpSettings();
void prefetchAcpRegistry();

} // namespace AcpClient::Internal
