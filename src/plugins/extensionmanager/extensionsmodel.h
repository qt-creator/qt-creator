// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>

namespace ExtensionSystem {
class PluginSpec;
}

namespace ExtensionManager::Internal {

using QPairList = QList<QPair<QString, QString> >;

using ImagesData = QPairList; // { <caption, url>, ... }
using LinksData = QPairList; // { <name, url>, ... }
using PluginsData = QPairList; // { <name, url>, ... }
using TextData = QList<QPair<QString, QStringList> >; // { <header, text>, ... }

enum ItemType {
    ItemTypePack,
    ItemTypeExtension,
};

enum ExtensionState {
    None, // Not a plugin
    InstalledEnabled,
    InstalledDisabled,
    NotInstalled,
};

enum Role {
    RoleName = Qt::UserRole,
    RoleCompatVersion,
    RoleCopyright,
    RoleDependencies,
    RoleDescriptionImages,
    RoleDescriptionLinks,
    RoleDescriptionText,
    RoleDownloadCount,
    RoleExtensionState,
    RoleId,
    RoleItemType,
    RoleLicense,
    RoleLocation,
    RolePlatforms,
    RolePlugins,
    RoleSearchText,
    RoleSize,
    RoleTags,
    RoleVendor,
    RoleVersion,
};

class ExtensionsModel : public QAbstractListModel
{
public:
    ExtensionsModel(QObject *parent = nullptr);
    ~ExtensionsModel();

    int rowCount(const QModelIndex &parent = {}) const;
    QVariant data(const QModelIndex &index, int role) const;

    void setExtensionsJson(const QByteArray &json);

private:
    class ExtensionsModelPrivate *d = nullptr;
};

ExtensionSystem::PluginSpec *pluginSpecForName(const QString &pluginName);

#ifdef WITH_TESTS
QObject *createExtensionsModelTest();
#endif

} // ExtensionManager::Internal

Q_DECLARE_METATYPE(ExtensionManager::Internal::QPairList)
Q_DECLARE_METATYPE(ExtensionManager::Internal::TextData)
