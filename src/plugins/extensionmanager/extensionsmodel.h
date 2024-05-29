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

enum Role {
    RoleName = Qt::UserRole,
    RoleCopyright,
    RoleDependencies,
    RoleDescriptionImages,
    RoleDescriptionLinks,
    RoleDescriptionText,
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
    static ExtensionSystem::PluginSpec *pluginSpecForName(const QString &pluginName);

private:
    class ExtensionsModelPrivate *d = nullptr;
};

#ifdef WITH_TESTS
QObject *createExtensionsModelTest();
#endif

} // ExtensionManager::Internal

Q_DECLARE_METATYPE(ExtensionManager::Internal::QPairList)
Q_DECLARE_METATYPE(ExtensionManager::Internal::TextData)
