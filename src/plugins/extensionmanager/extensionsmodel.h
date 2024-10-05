// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/osspecificaspects.h>

#include <QAbstractListModel>

namespace ExtensionSystem {
class PluginSpec;
}

namespace ExtensionManager::Internal {

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
    RoleBadge,
    RoleCopyright,
    RoleDateUpdated,
    RoleDependencies,
    RoleDescriptionLong,
    RoleDescriptionShort,
    RoleDownloadCount,
    RoleDownloadUrl,
    RoleExtensionState,
    RoleId,
    RoleItemType,
    RoleLicense,
    RolePlatforms,
    RolePlugins,
    RoleSearchText,
    RoleStatus,
    RoleTags,
    RoleVendor,
    RoleVendorId,
    RoleVersion,
};

class ExtensionsModel : public QAbstractListModel
{
public:
    ExtensionsModel(QObject *parent = nullptr);
    ~ExtensionsModel();

    int rowCount(const QModelIndex &parent = {}) const;
    QVariant data(const QModelIndex &index, int role) const;

    QModelIndex indexOfId(const QString &extensionId) const;
    void setExtensionsJson(const QByteArray &json);

private:
    class ExtensionsModelPrivate *d = nullptr;
};

QString customOsTypeToString(Utils::OsType osType);
ExtensionSystem::PluginSpec *pluginSpecForId(const QString &pluginId);
QString statusDisplayString(const QModelIndex &index);

#ifdef WITH_TESTS
QObject *createExtensionsModelTest();
#endif

} // ExtensionManager::Internal
