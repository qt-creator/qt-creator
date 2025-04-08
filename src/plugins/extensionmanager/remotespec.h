// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginspec.h>

#include <utils/hostosinfo.h>

#include <QJsonObject>

namespace ExtensionManager::Internal {

class Source
{
public:
    struct Platform
    {
        Utils::OsType os;
        Utils::OsArch architecture;
    };
    QString url;
    QString sha;
    std::optional<Platform> platform;
};

enum class Status {
    Published,
    Unpublished,
};

class RemoteSpec : public ExtensionSystem::PluginSpec
{
public:
    QString description() const override;
    QString longDescription() const override;

    bool loadLibrary() override;
    bool initializePlugin() override;
    bool initializeExtensions() override;
    bool delayedInitialize() override;
    ExtensionSystem::IPlugin::ShutdownFlag stop() override;
    void kill() override;
    ExtensionSystem::IPlugin *plugin() const override;
    Utils::FilePath installLocation(bool inUserFolder) const override;

    Utils::Result<> fromJson(const QJsonObject &remoteJsonData);
    Utils::Result<> fromJson(const QJsonObject &remoteJsonData, const QString &version);

    QString id() const override;
    QString displayName() const override;
    QString vendor() const override;
    QString vendorId() const override;

    std::optional<Source> compatibleSource() const;
    const QList<Source> sources() const;
    std::vector<std::unique_ptr<RemoteSpec>> versions() const;

    QList<QString> tags() const;

    QDateTime createdAt() const;
    QDateTime updatedAt() const;
    QDateTime releasedAt() const;

    int downloads() const;
    QString icon() const;
    QString smallIcon() const;

    bool isLatest() const;
    // ?? QString license() const {}
    Status status() const;
    QString statusString() const;
    QString uid() const;

    bool isPack() const;

    QJsonObject infoObject() const;
    QJsonObject pluginObject() const;
    QJsonObject packObject() const;

    QStringList packPluginIds() const;

private:
    QJsonObject m_remoteJsonData;
    QString m_version;
    bool m_isPack = false;
};

} // namespace ExtensionManager::Internal

Q_DECLARE_METATYPE(ExtensionManager::Internal::RemoteSpec)
