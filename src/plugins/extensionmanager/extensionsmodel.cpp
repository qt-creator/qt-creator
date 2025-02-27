// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionsmodel.h"

#include "extensionmanagertr.h"

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

const char EXTENSION_KEY_ID[] = "id";

Q_LOGGING_CATEGORY(modelLog, "qtc.extensionmanager.model", QtWarningMsg)

class ExtensionsModelPrivate
{
public:
    void addUnlistedLocalPlugins();

    static QVariant dataFromRemotePack(const QJsonObject &json, int role);
    static QVariant dataFromRemotePlugin(const QJsonObject &json, int role);
    QVariant dataFromRemoteExtension(int index, int role) const;
    QVariant dataFromLocalPlugin(int index, int role) const;

    QJsonArray responseItems;
    PluginSpecs localPlugins;
};

void ExtensionsModelPrivate::addUnlistedLocalPlugins()
{
    QSet<QString> responseExtensions;
    for (const QJsonValueConstRef &responseItem : qAsConst(responseItems))
        responseExtensions << responseItem.toObject().value("id").toString();

    localPlugins.clear();
    for (PluginSpec *plugin : PluginManager::plugins())
        if (!responseExtensions.contains(plugin->id()))
            localPlugins.append(plugin);

    qCDebug(modelLog) << "Number of extensions from JSON:" << responseExtensions.count();
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

QVariant ExtensionsModelPrivate::dataFromRemotePack(const QJsonObject &json, int role)
{
    switch (role) {
    case RoleDescriptionLong:
        return joinedStringList(json.value("long_description"));
    case RoleDescriptionShort:
        return joinedStringList(json.value("description"));
    case RoleItemType:
        return ItemTypePack;
    case RolePlugins:
        return json.value("plugins").toVariant().toStringList();
    default:
        break;
    }

    return {};
}

QVariant ExtensionsModelPrivate::dataFromRemotePlugin(const QJsonObject &json, int role)
{
    const QJsonObject metaData = json.value("metadata").toObject();

    switch (role) {
    case RoleCopyright:
        return metaData.value("Copyright");
    case RoleDownloadUrl: {
        const QJsonArray sources = json.value("sources").toArray();
        const QString thisPlatform = customOsTypeToString(HostOsInfo::hostOs());
        const QString thisArch = QSysInfo::currentCpuArchitecture();
        for (const QJsonValue &source : sources) {
            const QJsonObject sourceObject = source.toObject();
            const QJsonObject platform = sourceObject.value("platform").toObject();
            if (platform.isEmpty() // Might be a Lua plugin
                    || (platform.value("name").toString() == thisPlatform
                        && platform.value("architecture") == thisArch))
                return sourceObject.value("url").toString();
        }
        break;
    }
    case RoleDependencies: {
        QStringList dependencies;

        const QJsonArray dependenciesArray = metaData.value("Dependencies").toArray();
        for (const auto &dependency : dependenciesArray) {
            const QJsonObject dependencyObject = dependency.toObject();
            dependencies.append(dependencyObject.value("Id").toString());
        }

        return dependencies;
    }
    case RoleVersion:
        return metaData.value("Version");
    case RoleItemType:
        return ItemTypeExtension;
    case RoleDescriptionLong: {
        const QString description = joinedStringList(metaData.value("LongDescription"));
        const QString url = metaData.value("Url").toString();
        const QString documentationUrl = metaData.value("DocumentationUrl").toString();
        return descriptionWithLinks(description, url, documentationUrl);
    }
    case RoleDescriptionShort:
        return joinedStringList(metaData.value("Description"));
    default:
        break;
    }

    return {};
}

QVariant ExtensionsModelPrivate::dataFromRemoteExtension(int index, int role) const
{
    const QJsonObject json = responseItems.at(index).toObject();

    switch (role) {
    case Qt::DisplayRole:
    case RoleName:
        return json.value("display_name");
    case RoleDownloadCount:
        break; // TODO: Reinstate download numbers when they have more substance
        // return json.value("downloads");
    case RoleId:
        return json.value(EXTENSION_KEY_ID);
    case RoleDateUpdated:
        return QDate::fromString(json.value("updated_at").toString(), Qt::ISODate);
    case RoleStatus:
        return json.value("status");
    case RoleTags:
        return json.value("tags").toVariant().toStringList();
    case RoleVendor:
        return json.value("display_vendor");
    case RoleVendorId:
        return json.value("vendor_id");
    default:
        break;
    }

    const QJsonObject pluginObject = json.value("plugin").toObject();
    if (!pluginObject.isEmpty())
        return dataFromRemotePlugin(pluginObject, role);

    const QJsonObject packObject = json.value("pack").toObject();
    if (!packObject.isEmpty())
        return dataFromRemotePack(packObject, role);

    return {};
}

QVariant ExtensionsModelPrivate::dataFromLocalPlugin(int index, int role) const
{
    const PluginSpec *pluginSpec = localPlugins.at(index);

    switch (role) {
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
    return d->responseItems.count() + d->localPlugins.count();
}

static QString badgeText(const QModelIndex &index)
{
    if (index.data(RoleDownloadUrl).isNull())
        return {};

    const PluginSpec *ps = pluginSpecForId(index.data(RoleId).toString());
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

    const PluginSpec *ps = pluginSpecForId(index.data(RoleId).toString());
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

    const bool isRemoteExtension = index.row() < d->responseItems.count();
    const int itemIndex = index.row() - (isRemoteExtension ? 0 : d->responseItems.count());

    return isRemoteExtension ? d->dataFromRemoteExtension(itemIndex, role)
                             : d->dataFromLocalPlugin(itemIndex, role);
}

QModelIndex ExtensionsModel::indexOfId(const QString &extensionId) const
{
    const int localIndex = indexOf(d->localPlugins, equal(&PluginSpec::id, extensionId));
    if (localIndex >= 0)
        return index(d->responseItems.count() + localIndex);

    for (int remoteIndex = 0; const QJsonValueConstRef &value : std::as_const(d->responseItems)) {
        if (value.toObject().value(EXTENSION_KEY_ID) == extensionId)
            return index(remoteIndex);
        ++remoteIndex;
    }

    return {};
}

void ExtensionsModel::setExtensionsJson(const QByteArray &json)
{
    beginResetModel();
    QJsonParseError error;
    const QJsonObject jsonObj = QJsonDocument::fromJson(json, &error).object();
    qCDebug(modelLog) << "QJsonParseError:" << error.errorString();
    d->responseItems = jsonObj.value("items").toArray();
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

PluginSpec *pluginSpecForId(const QString &pluginId)
{
    return findOrDefault(PluginManager::plugins(), equal(&PluginSpec::id, pluginId));
}

QString statusDisplayString(const QModelIndex &index)
{
    const QString statusString = index.data(RoleStatus).toString();
    return statusString != "published" ? statusString : QString();
}

} // ExtensionManager::Internal
