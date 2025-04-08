// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotespec.h"

#include <utils/algorithm.h>
#include <utils/stringutils.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(remoteSpec, "qtc.extensionmanager.remotespec", QtWarningMsg)

using namespace Utils;

namespace ExtensionManager::Internal {
bool RemoteSpec::loadLibrary()
{
    return false;
};
bool RemoteSpec::initializePlugin()
{
    return false;
};
bool RemoteSpec::initializeExtensions()
{
    return false;
};
bool RemoteSpec::delayedInitialize()
{
    return false;
};
ExtensionSystem::IPlugin::ShutdownFlag RemoteSpec::stop()
{
    return ExtensionSystem::IPlugin::SynchronousShutdown;
};

void RemoteSpec::kill() {};

ExtensionSystem::IPlugin *RemoteSpec::plugin() const
{
    return nullptr;
};

FilePath RemoteSpec::installLocation(bool inUserFolder) const
{
    Q_UNUSED(inUserFolder);
    return {};
};

Result<> RemoteSpec::fromJson(const QJsonObject &remoteJsonData)
{
    qCDebug(remoteSpec).noquote() << "Remote JSON data:"
                                  << QJsonDocument(remoteJsonData).toJson(QJsonDocument::Indented);
    return fromJson(remoteJsonData, remoteJsonData.value("latest").toString());
}

Utils::Result<> RemoteSpec::fromJson(const QJsonObject &remoteJsonData, const QString &version)
{
    m_remoteJsonData = remoteJsonData;
    m_version = version;

    const QJsonObject plugin = pluginObject();

    if (!plugin.isEmpty()) {
        auto res = ExtensionSystem::PluginSpec::readMetaData(plugin.value("metadata").toObject());
        if (!res)
            return ResultError(res.error());
        if (hasError())
            return ResultError(errorString());
        return ResultOk;
    }

    m_isPack = true;

    return ResultOk;
}

std::vector<std::unique_ptr<RemoteSpec>> RemoteSpec::versions() const
{
    std::vector<std::unique_ptr<RemoteSpec>> versionList;

    QJsonObject versions = m_remoteJsonData.value("versions").toObject();
    for (const auto &version : versions.keys()) {
        std::unique_ptr<RemoteSpec> remoteSpec(new RemoteSpec());
        auto result = remoteSpec->fromJson(m_remoteJsonData, version);
        if (!result) {
            qWarning() << "Remote version error:" << result.error();
            continue;
        }
        versionList.push_back(std::move(remoteSpec));
    }

    return versionList;
}

std::optional<Source> RemoteSpec::compatibleSource() const
{
    return Utils::findOr(sources(), std::nullopt, [](const auto &source) {
        if (!source.platform)
            return true;

        if (source.platform->os == HostOsInfo::hostOs()
            && source.platform->architecture == HostOsInfo::hostArchitecture())
            return true;

        return false;
    });
}

const QList<Source> RemoteSpec::sources() const
{
    const auto obj = pluginObject();
    if (obj.isEmpty())
        return {};

    QList<Source> sources;
    for (const QJsonValue &source : obj.value("sources").toArray()) {
        Source s;
        const QJsonObject platform = source.toObject().value("platform").toObject();
        if (!platform.isEmpty()) {
            s.platform = Source::Platform{
                osTypeFromString(platform.value("name").toString()).value_or(OsTypeOther),
                osArchFromString(platform.value("architecture").toString()).value_or(OsArchUnknown)};
        }
        s.url = source.toObject().value("url").toString();
        s.sha = source.toObject().value("sha256").toString();
        sources.append(s);
    }
    return sources;
}

QList<QString> RemoteSpec::tags() const
{
    return infoObject().value("tags").toVariant().toStringList();
}
QDateTime RemoteSpec::createdAt() const
{
    return QDateTime::fromString(m_remoteJsonData.value("created_at").toString(), Qt::ISODate);
}
QDateTime RemoteSpec::updatedAt() const
{
    return QDateTime::fromString(m_remoteJsonData.value("updated_at").toString(), Qt::ISODate);
}
QDateTime RemoteSpec::releasedAt() const
{
    return QDateTime::fromString(m_remoteJsonData.value("released_at").toString(), Qt::ISODate);
}
int RemoteSpec::downloads() const
{
    return m_remoteJsonData.value("downloads").toInt();
}
QString RemoteSpec::icon() const
{
    return infoObject().value("icon").toString();
}
QString RemoteSpec::smallIcon() const
{
    return infoObject().value("small_icon").toString();
}
bool RemoteSpec::isLatest() const
{
    return m_remoteJsonData.value("is_latest").toBool();
}
Status RemoteSpec::status() const
{
    const QString status = m_remoteJsonData.value("status").toString();
    if (status == "published") {
        return Status::Published;
    } else if (status == "unpublished") {
        return Status::Unpublished;
    }
    return Status::Published;
}

QString RemoteSpec::uid() const
{
    return m_remoteJsonData.value("uid").toString();
}

QString RemoteSpec::statusString() const
{
    return m_remoteJsonData.value("status").toString();
}
QJsonObject RemoteSpec::packObject() const
{
    return m_remoteJsonData.value("pack").toObject();
}
QJsonObject RemoteSpec::pluginObject() const
{
    if (m_version.isEmpty())
        return {};

    return m_remoteJsonData.value("versions").toObject().value(m_version).toObject();
}
bool RemoteSpec::isPack() const
{
    return m_isPack;
}
QString RemoteSpec::description() const
{
    if (isPack()) {
        QString result;
        if (readMultiLineString(packObject().value("description"), &result))
            return result;
        return {};
    }

    return PluginSpec::description();
}

QString RemoteSpec::longDescription() const
{
    if (isPack()) {
        QString result;
        if (readMultiLineString(packObject().value("long_description"), &result))
            return result;
        return {};
    }

    return PluginSpec::longDescription();
}

QStringList RemoteSpec::packPluginIds() const
{
    return packObject().value("plugins").toVariant().toStringList();
}

QString RemoteSpec::id() const
{
    if (isPack())
        return infoObject().value("id").toString();

    return PluginSpec::id();
}

QString RemoteSpec::displayName() const
{
    if (isPack())
        return infoObject().value("display_name").toString();

    return PluginSpec::displayName();
}

QString RemoteSpec::vendor() const
{
    if (isPack())
        return infoObject().value("vendor").toString();

    return PluginSpec::vendor();
}

QString RemoteSpec::vendorId() const
{
    if (isPack())
        return infoObject().value("vendor_id").toString();

    return PluginSpec::vendorId();
}

QJsonObject RemoteSpec::infoObject() const
{
    return m_remoteJsonData.value("info").toObject();
}
} // namespace ExtensionManager::Internal
