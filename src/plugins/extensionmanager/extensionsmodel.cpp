// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionsmodel.h"

#include "extensionmanagertr.h"
#include "remotespec.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginview.h>
#include <extensionsystem/pluginmanager.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardItemModel>
#include <QVersionNumber>

using namespace ExtensionSystem;
using namespace Core;
using namespace Utils;

namespace ExtensionManager::Internal {

Q_LOGGING_CATEGORY(modelLog, "qtc.extensionmanager.model", QtWarningMsg)

class ExtensionsModelPrivate
{
public:
    void addUnlistedLocalPlugins();

    static QVariant dataFromRemotePack(const RemoteSpec *spec, int role);
    static QVariant dataFromRemotePlugin(const RemoteSpec *spec, int role);
    QVariant dataFromRemoteExtension(int index, int role) const;
    QVariant dataFromLocalPlugin(int index, int role) const;

    //QJsonArray responseItems;
    PluginSpecs localPlugins;
    std::vector<std::unique_ptr<RemoteSpec>> remotePlugins;
};

void ExtensionsModelPrivate::addUnlistedLocalPlugins()
{
    QSet<QString> remoteIds = Utils::transform<QSet>(remotePlugins, &RemoteSpec::id);
    localPlugins = Utils::filtered(PluginManager::plugins(), [&remoteIds](const PluginSpec *plugin) {
        return !remoteIds.contains(plugin->id());
    });

    qCDebug(modelLog) << "Number of extensions from JSON:" << remotePlugins.size();
    qCDebug(modelLog) << "Number of added local plugins:" << localPlugins.count();
}

QString joinedStringList(const QJsonValue &json)
{
    if (json.isArray()) {
        const QStringList lines = json.toVariant().toStringList();
        return lines.join("\n");
    }
    return json.toString();
}

QString descriptionWithLinks(const QString &description, const QString &url,
                             const QString &documentationUrl)
{
    QStringList fragments;
    const QString mdLink("[%1](%2)");
    if (!url.isEmpty())
        fragments.append(mdLink.arg(url).arg(url));
    if (!documentationUrl.isEmpty())
        fragments.append(mdLink.arg(Tr::tr("Documentation")).arg(documentationUrl));
    if (!fragments.isEmpty())
        fragments.prepend("### " + Tr::tr("More Information"));
    fragments.prepend(description);
    return fragments.join("\n\n");
}

QVariant ExtensionsModelPrivate::dataFromRemoteExtension(int index, int role) const
{
    QTC_ASSERT(index >= 0 && size_t(index) < remotePlugins.size(), return {});
    const RemoteSpec *remoteSpec = remotePlugins.at(index).get();

    switch (role) {
    case RoleSpec:
        return QVariant::fromValue(remoteSpec);
    case Qt::DisplayRole:
    case RoleName:
        return remoteSpec->displayName();
    case RoleDownloadCount:
        return remoteSpec->downloads();
    case RoleFullId:
        return QString("%1.%2").arg(remoteSpec->vendorId()).arg(remoteSpec->id());
    case RoleId:
        return remoteSpec->id();
    case RoleDateUpdated:
        return remoteSpec->updatedAt();
    case RoleStatus:
        return remoteSpec->statusString();
    case RoleTags:
        return remoteSpec->tags();
    case RoleVendor:
        return remoteSpec->vendor();
    case RoleVendorId:
        return remoteSpec->vendorId();
    case RoleCopyright:
        return remoteSpec->copyright();
    case RoleDownloadUrl: {
        for (const auto &source : remoteSpec->sources()) {
            if (!source.platform)
                return source.url;

            if (source.platform->os == HostOsInfo::hostOs()
                && source.platform->architecture == HostOsInfo::hostArchitecture())
                return source.url;
        }
        return {};
    }
    case RoleDependencies: {
        QStringList dependencies
            = Utils::transform(remoteSpec->dependencies(), &PluginDependency::id);
        return dependencies;
    }
    case RolePlatforms: {
        QStringList platforms
            = Utils::transform<QStringList>(remoteSpec->sources(), [](const Source &s) -> QString {
                  if (!s.platform)
                      return Tr::tr("Platform agnostic");

                  const QString name = customOsTypeToString(s.platform->os);
                  const QString architecture = customOsArchToString(s.platform->architecture);
                  return name + " " + architecture;
              });
        platforms.sort(Qt::CaseInsensitive);
        return Utils::filteredUnique(platforms);
    }
    case RoleVersion:
        return remoteSpec->version();
    case RoleItemType:
        if (remoteSpec->isPack())
            return ItemTypePack;

        return ItemTypeExtension;
    case RoleDescriptionLong: {
        const QString description = remoteSpec->longDescription();
        const QString url = remoteSpec->url();
        const QString documentationUrl = remoteSpec->documentationUrl();
        return descriptionWithLinks(description, url, documentationUrl);
    }
    case RoleDescriptionShort:
        return remoteSpec->description();
    case RolePlugins:
        return remoteSpec->packPluginIds();
    }

    return {};
}

QVariant ExtensionsModelPrivate::dataFromLocalPlugin(int index, int role) const
{
    const PluginSpec *pluginSpec = localPlugins.at(index);

    switch (role) {
    case RoleSpec:
        return QVariant::fromValue(pluginSpec);
    case Qt::DisplayRole:
    case RoleName:
        return pluginSpec->displayName();
    case RoleCopyright:
        return pluginSpec->copyright();
    case RoleDependencies: {
        const QStringList dependencies
            = transform(pluginSpec->dependencies(), &PluginDependency::id);
        return dependencies;
    }
    case RoleDescriptionLong:
        return descriptionWithLinks(pluginSpec->longDescription(), pluginSpec->url(),
                                    pluginSpec->documentationUrl());
    case RoleDescriptionShort:
        return pluginSpec->description();
    case RoleFullId:
        return QString("%1.%2").arg(pluginSpec->vendorId()).arg(pluginSpec->id());
    case RoleId:
        return pluginSpec->id();
    case RoleItemType:
        return ItemTypeExtension;
    case RolePlatforms: {
        const QString platformsPattern = pluginSpec->platformSpecification().pattern();
        const QStringList platforms = platformsPattern.isEmpty()
                                          ? QStringList({customOsTypeToString(OsTypeMac),
                                                         customOsTypeToString(OsTypeWindows),
                                                         customOsTypeToString(OsTypeLinux)})
                                          : QStringList(platformsPattern);
        return platforms;
    }
#ifdef QTC_SHOW_BUILD_DATE
    case RoleDateUpdated:
        return QDate::fromString(QLatin1String(__DATE__), "MMM dd yyyy");
#endif
    case RoleVendor:
        return pluginSpec->vendor();
    case RoleVendorId:
        return pluginSpec->vendorId();
    default:
        break;
    }
    return {};
}

ExtensionsModel::ExtensionsModel(QObject *parent)
    : QAbstractListModel(parent)
    , d(new ExtensionsModelPrivate)
{
}

ExtensionsModel::~ExtensionsModel()
{
    delete d;
}

int ExtensionsModel::rowCount([[maybe_unused]] const QModelIndex &parent) const
{
    return int(d->remotePlugins.size() + d->localPlugins.count());
}

static QString badgeText(const QModelIndex &index)
{
    if (index.data(RoleDownloadUrl).isNull())
        return {};

    const PluginSpec *ps = PluginManager::specById(index.data(RoleId).toString());
    if (!ps)
        return Tr::tr("New");

    const QVersionNumber remoteVersion = QVersionNumber::fromString(
        index.data(RoleVersion).toString());
    const QVersionNumber localVersion = QVersionNumber::fromString(ps->version());
    return remoteVersion > localVersion ? Tr::tr("Updated") : QString();
}

ExtensionState extensionState(const QModelIndex &index)
{
    if (index.data(RoleItemType) != ItemTypeExtension)
        return None;

    const PluginSpec *ps = PluginManager::specById(index.data(RoleId).toString());
    if (!ps)
        return NotInstalled;

    return ps->isEffectivelyEnabled() ? InstalledEnabled : InstalledDisabled;
}

static QString searchText(const QModelIndex &index)
{
    QStringList searchTexts;
    searchTexts.append(index.data(RoleName).toString());
    searchTexts.append(index.data(RoleTags).toStringList());
    searchTexts.append(index.data(RoleDescriptionShort).toString());
    searchTexts.append(index.data(RoleDescriptionLong).toString());
    searchTexts.append(index.data(RoleVendor).toString());
    return searchTexts.join(" ");
}

QVariant ExtensionsModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case RoleBadge:
        return badgeText(index);
    case RoleExtensionState:
        return extensionState(index);
    case RoleSearchText:
        return searchText(index);
    default:
        break;
    }

    const bool isRemoteExtension = size_t(index.row()) < d->remotePlugins.size();
    const int itemIndex = int(index.row() - (isRemoteExtension ? 0 : d->remotePlugins.size()));

    return isRemoteExtension ? d->dataFromRemoteExtension(itemIndex, role)
                             : d->dataFromLocalPlugin(itemIndex, role);
}

QModelIndex ExtensionsModel::indexOfId(const QString &extensionId) const
{
    const int localIndex = indexOf(d->localPlugins, equal(&PluginSpec::id, extensionId));
    if (localIndex >= 0)
        return index(int(d->remotePlugins.size() + localIndex));

    for (int remoteIndex = 0;
         const std::unique_ptr<RemoteSpec> &spec : std::as_const(d->remotePlugins)) {
        if (spec->id() == extensionId)
            return index(remoteIndex);
        ++remoteIndex;
    }

    return {};
}

void ExtensionsModel::setRepositoryPaths(const FilePaths &paths)
{
    beginResetModel();
    d->remotePlugins.clear();

    const auto scanPath = [this](const FilePath &path) {
        if (path.isEmpty())
            return;

        FilePath registryPath = path / "registry";
        if (!registryPath.isReadableDir()) {
            // Github has one top-level directory in its zip, so lets check if thats the case ...
            const FilePaths firstLevelEntries = path.dirEntries(QDir::Dirs | QDir::NoDotAndDotDot);

            if (firstLevelEntries.size() == 1)
                registryPath = firstLevelEntries.first() / "registry";

            if (!registryPath.isReadableDir()) {
                qCWarning(modelLog) << "Registry path not readable:" << registryPath;
                return;
            }
        }

        registryPath.iterateDirectory(
            [this](const FilePath &item) -> IterationPolicy {
                const auto contents = item.fileContents();
                if (!contents) {
                    qCWarning(modelLog) << "Failed to read file:" << item << contents.error();
                    return IterationPolicy::Continue;
                }

                QJsonParseError error;
                const QJsonObject obj = QJsonDocument::fromJson(*contents, &error).object();
                if (error.error != QJsonParseError::NoError) {
                    qCWarning(modelLog) << "Failed to parse JSON" << item.toUserOutput() << ":"
                                        << error.errorString();
                    return IterationPolicy::Continue;
                }
                std::unique_ptr<RemoteSpec> remoteSpec(new RemoteSpec());
                auto result = remoteSpec->fromJson(obj);
                if (!result) {
                    qCWarning(modelLog) << "Failed to read remote extension" << item.toUserOutput()
                                        << ":" << result.error();
                    return IterationPolicy::Continue;
                }

                for (auto it = d->remotePlugins.begin(); it != d->remotePlugins.end(); ++it) {
                    if ((*it)->id() == remoteSpec->id()) {
                        std::swap(*it, remoteSpec);
                        return IterationPolicy::Continue;
                    }
                }

                d->remotePlugins.push_back(std::move(remoteSpec));
                return IterationPolicy::Continue;
            },
            FileFilter({"extension.json"}, QDir::Files, QDirIterator::Subdirectories));
    };

    for (const FilePath &path : paths)
        scanPath(path);

    d->addUnlistedLocalPlugins();
    endResetModel();
}

QString customOsTypeToString(OsType osType)
{
    switch (osType) {
    case OsTypeWindows:
        return "Windows";
    case OsTypeLinux:
        return "Linux";
    case OsTypeMac:
        return "macOS";
    case OsTypeOtherUnix:
        return "Other Unix";
    case OsTypeOther:
    default:
        return "Other";
    }
}

QString customOsArchToString(OsArch osArch)
{
    switch (osArch) {
    case OsArchX86:
        return "x86";
    case OsArchAMD64:
        return "x86_64";
    case OsArchItanium:
        return "ia64";
    case OsArchArm:
        return "arm";
    case OsArchArm64:
        return "arm64";
    case OsArchUnknown:
        break;
    }

    return "Unknown";
}

QString statusDisplayString(const QModelIndex &index)
{
    const QString statusString = index.data(RoleStatus).toString();
    return statusString != "published" ? statusString : QString();
}

} // ExtensionManager::Internal
