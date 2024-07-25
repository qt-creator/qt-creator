// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionsmodel.h"

#include "extensionmanagertr.h"

#include "utils/algorithm.h"

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

struct Dependency
{
    QString name;
    QString version;
};
using Dependencies = QList<Dependency>;

struct Plugin
{
    QString copyright;
    Dependencies dependencies;
    bool isInternal = false;
    QString name;
    QString packageUrl;
    QString vendor;
    QString version;
};
using Plugins = QList<Plugin>;

struct Description {
    ImagesData images;
    LinksData links;
    TextData text;
};

struct Extension {
    QString compatVersion;
    QString copyright;
    Description description;
    int downloadCount = -1;
    QString id;
    QString license;
    QString name;
    QStringList platforms;
    Plugins plugins;
    qint64 size = 0;
    QStringList tags;
    ItemType type = ItemTypePack;
    QString vendor;
    QString version;
};
using Extensions = QList<Extension>;

static const Dependencies dependenciesFromJson(const QJsonObject &obj)
{
    const QJsonArray dependenciesArray = obj.value("Dependencies").toArray();
    Dependencies dependencies;
    for (const QJsonValueConstRef &dependencyVal : dependenciesArray) {
        const QJsonObject dependencyObj = dependencyVal.toObject();
        const QJsonObject metaDataObj = dependencyObj.value("meta_data").toObject();
        dependencies.append({
            .name = metaDataObj.value("Name").toString(),
            .version = metaDataObj.value("Version").toString(),
        });
    }

    return dependencies;
}

static Plugin pluginFromJson(const QJsonObject &obj)
{
    const QJsonObject metaDataObj = obj.value("meta_data").toObject();

    return {
        .copyright = metaDataObj.value("Copyright").toString(),
        .dependencies = dependenciesFromJson(metaDataObj),
        .isInternal = obj.value("is_internal").toBool(false),
        .name = metaDataObj.value("Name").toString(),
        .packageUrl = obj.value("url").toString(),
        .vendor = metaDataObj.value("Vendor").toString(),
        .version = metaDataObj.value("Version").toString(),
    };
}

static Description descriptionFromJson(const QJsonObject &obj)
{
    TextData descriptionText;
    const QJsonArray paragraphsArray = obj.value("paragraphs").toArray();
    for (const QJsonValueConstRef &paragraphVal : paragraphsArray) {
        const QJsonObject &paragraphObj = paragraphVal.toObject();
        const QJsonArray &textArray = paragraphObj.value("text").toArray();
        QStringList textLines;
        for (const QJsonValueConstRef &textVal : textArray)
            textLines.append(textVal.toString());
        descriptionText.append({
            paragraphObj.value("header").toString(),
            textLines,
        });
    }

    LinksData links;
    const QJsonArray linksArray = obj.value("links").toArray();
    for (const QJsonValueConstRef &linkVal : linksArray) {
        const QJsonObject &linkObj = linkVal.toObject();
        links.append({
            linkObj.value("link_text").toString(),
            linkObj.value("url").toString(),
        });
    }

    ImagesData images;
    const QJsonArray imagesArray = obj.value("images").toArray();
    for (const QJsonValueConstRef &imageVal : imagesArray) {
        const QJsonObject &imageObj = imageVal.toObject();
        images.append({
            imageObj.value("image_label").toString(),
            imageObj.value("url").toString(),
        });
    }

    const Description description = {
        .images = images,
        .links = links,
        .text = descriptionText,
    };

    return description;
}

static Extension extensionFromJson(const QJsonObject &obj)
{
    Plugins plugins;
    const QJsonArray pluginsArray = obj.value("plugins").toArray();
    for (const QJsonValueConstRef &pluginVal : pluginsArray)
        plugins.append(pluginFromJson(pluginVal.toObject()));

    QStringList tags;
    const QJsonArray tagsArray = obj.value("tags").toArray();
    for (const QJsonValueConstRef &tagVal : tagsArray)
        tags.append(tagVal.toString());

    QStringList platforms;
    const QJsonArray platformsArray = obj.value("platforms").toArray();
    for (const QJsonValueConstRef &platformsVal : platformsArray)
        platforms.append(platformsVal.toString());

    const QJsonObject descriptionObj = obj.value("description").toObject();
    const Description description = descriptionFromJson(descriptionObj);

    const Extension extension = {
        .compatVersion = obj.value("compatibility").toString(),
        .copyright = obj.value("copyright").toString(),
        .description = description,
        .downloadCount = obj.value("download_count").toInt(-1),
        .id = obj.value("id").toString(),
        .license = obj.value("license").toString(),
        .name = obj.value("name").toString(),
        .platforms = platforms,
        .plugins = plugins,
        .size = obj.value("total_size").toInteger(),
        .tags = tags,
        .type = obj.value("is_pack").toBool(true) ? ItemTypePack : ItemTypeExtension,
        .vendor = obj.value("vendor").toString(),
        .version = obj.value("version").toString(),
    };

    return extension;
}

static Extensions parseExtensionsRepoReply(const QByteArray &jsonData)
{
    // https://qc-extensions.qt.io/api-docs
    Extensions parsedExtensions;
    const QJsonObject jsonObj = QJsonDocument::fromJson(jsonData).object();
    const QJsonArray items = jsonObj.value("items").toArray();
    for (const QJsonValueConstRef &itemVal : items) {
        const QJsonObject itemObj = itemVal.toObject();
        const Extension extension = extensionFromJson(itemObj);
        parsedExtensions.append(extension);
    }
    return parsedExtensions;
}

static Extension extensionFromPluginSpec(const PluginSpec *pluginSpec)
{
    const Dependencies dependencies = transform(pluginSpec->dependencies(),
                                                [](const PluginDependency &pd) -> Dependency {
        return {
            .name = pd.name,
            .version = pd.version,
        };
    });
    const Plugin plugin = {
        .copyright = pluginSpec->copyright(),
        .dependencies = dependencies,
        .name = pluginSpec->name(),
        .packageUrl = {},
        .vendor = pluginSpec->vendor(),
        .version = pluginSpec->version(),
    };

    const QStringList lines = pluginSpec->description().split('\n')
                              + pluginSpec->longDescription().split('\n');
    const TextData text = {{ pluginSpec->name(), lines }};
    LinksData links;
    if (const QString url = pluginSpec->url(); !url.isEmpty())
        links.append({{}, url});
    if (const QString docUrl = pluginSpec->documentationUrl(); !docUrl.isEmpty())
        links.append({{Tr::tr("Documentation")}, docUrl});
    const Description description = {
        .images = {},
        .links = links,
        .text = text,
    };

    const QString platformsPattern = pluginSpec->platformSpecification().pattern();
    const QStringList platforms = platformsPattern.isEmpty()
                                      ? QStringList({"macOS", "Windows", "Linux"})
                                      : QStringList(platformsPattern);

    const Extension extension = {
        .compatVersion = pluginSpec->compatVersion(),
        .copyright = pluginSpec->copyright(),
        .description = description,
        .id = {},
        .license = pluginSpec->license(),
        .name = pluginSpec->name(),
        .platforms = platforms,
        .plugins = {plugin},
        .tags = {},
        .type = ItemTypeExtension,
        .vendor = pluginSpec->vendor(),
        .version = pluginSpec->version(),
    };
    return extension;
}

class ExtensionsModelPrivate
{
public:
    void setExtensions(const Extensions &extensions);
    void addUnlistedLocalExtensions();

    Extensions extensions;
};

void ExtensionsModelPrivate::setExtensions(const Extensions &extensions)
{
    this->extensions = extensions;
    qCDebug(modelLog) << "Number of extensions from JSON:" << this->extensions.count();
    addUnlistedLocalExtensions();
    qCDebug(modelLog) << "Number of extensions with added local ones:" << this->extensions.count();
}

void ExtensionsModelPrivate::addUnlistedLocalExtensions()
{
    const QStringList listedModelExtensions = transform(extensions, &Extension::name);
    for (const PluginSpec *plugin : PluginManager::plugins())
        if (!listedModelExtensions.contains(plugin->name()))
            extensions.append(extensionFromPluginSpec(plugin));
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
    return d->extensions.count();
}

static QStringList dependenciesFromExtension(const Extension &extension)
{
    QStringList dependencies;
    for (const Plugin &plugin : extension.plugins) {
        for (const Dependency &dependency : plugin.dependencies) {
            const QString withVersion = QString::fromLatin1("%1 (%2)").arg(dependency.name)
            .arg(dependency.version);
            dependencies.append(withVersion);
        }
    }
    dependencies.sort();
    dependencies.removeDuplicates();
    return dependencies;
}

static QVariant dataFromExtension(const Extension &extension, int role)
{
    switch (role) {
    case Qt::DisplayRole:
    case RoleName:
        return extension.name;
    case RoleCompatVersion:
        return extension.compatVersion;
    case RoleCopyright:
        return !extension.copyright.isEmpty() ? extension.copyright : QVariant();
    case RoleDependencies:
        return dependenciesFromExtension(extension);
    case RoleDescriptionImages:
        return QVariant::fromValue(extension.description.images);
    case RoleDescriptionLinks:
        return QVariant::fromValue(extension.description.links);
    case RoleDescriptionText:
        return QVariant::fromValue(extension.description.text);
    case RoleDownloadCount:
        return extension.downloadCount;
    case RoleId:
        return extension.id;
    case RoleItemType:
        return extension.type;
    case RoleLicense:
        return extension.license;
    case RoleLocation:
        break;
    case RolePlatforms:
        return extension.platforms;
    case RolePlugins: {
        PluginsData plugins;
        for (const Plugin &plugin : extension.plugins)
            plugins.append(qMakePair(plugin.name, plugin.packageUrl));
        return QVariant::fromValue(plugins);
    }
    case RoleSize:
        return extension.size;
    case RoleTags:
        return extension.tags;
    case RoleVendor:
        return !extension.vendor.isEmpty() ? extension.vendor : QVariant();
    case RoleVersion:
        return !extension.version.isEmpty() ? extension.version : QVariant();
    default:
        break;
    }
    return {};
}

ExtensionState extensionState(const QModelIndex &index)
{
    if (index.data(RoleItemType) != ItemTypeExtension)
        return None;

    const PluginSpec *ps = pluginSpecForName(index.data(RoleName).toString());
    if (!ps)
        return NotInstalled;

    return ps->isEffectivelyEnabled() ? InstalledEnabled : InstalledDisabled;
}

static QString searchText(const QModelIndex &index)
{
    QStringList searchTexts;
    searchTexts.append(index.data(RoleName).toString());
    searchTexts.append(index.data(RoleTags).toStringList());
    for (const auto &data : index.data(RoleDescriptionText).value<TextData>()) {
        searchTexts.append(data.first);
        searchTexts.append(data.second);
    }
    searchTexts.append(index.data(RoleVendor).toString());
    return searchTexts.join(" ");
}

QVariant ExtensionsModel::data(const QModelIndex &index, int role) const
{
    if (role == RoleExtensionState)
        return extensionState(index);
    if (role == RoleSearchText)
        return searchText(index);

    const Extension &extension = d->extensions.at(index.row());
    const QVariant extensionData = dataFromExtension(extension, role);
    // If data is unavailable, retrieve it from the first contained plugin
    if (extensionData.isNull() && !extension.plugins.isEmpty()) {
        const QString firstPluginName = extension.plugins.constFirst().name;
        const Extension firstPluginExtension =
            findOrDefault(d->extensions, Utils::equal(&Extension::name, firstPluginName));
        if (firstPluginExtension.name.isEmpty())
            return {};
        return dataFromExtension(firstPluginExtension, role);
    }
    return extensionData;
}

void ExtensionsModel::setExtensionsJson(const QByteArray &json)
{
    const Extensions extensions = parseExtensionsRepoReply(json);
    beginResetModel();
    d->setExtensions(extensions);
    endResetModel();
}

PluginSpec *pluginSpecForName(const QString &pluginName)
{
    return findOrDefault(PluginManager::plugins(), equal(&PluginSpec::name, pluginName));
}

} // ExtensionManager::Internal
