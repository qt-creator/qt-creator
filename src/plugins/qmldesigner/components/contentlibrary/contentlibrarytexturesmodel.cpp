// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibrarytexturesmodel.h"

#include "contentlibrarytexturescategory.h"

#include <designerpaths.h>
#include <qmldesignerbase/qmldesignerbaseplugin.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QQmlEngine>
#include <QSize>
#include <QStandardPaths>
#include <QUrl>

namespace QmlDesigner {

ContentLibraryTexturesModel::ContentLibraryTexturesModel(const QString &category, QObject *parent)
    : QAbstractListModel(parent)
{
    m_category = category; // textures main category (ex: Textures, Environments)
}

int ContentLibraryTexturesModel::rowCount(const QModelIndex &) const
{
    return m_bundleCategories.size();
}

QVariant ContentLibraryTexturesModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_bundleCategories.size(), return {});
    QTC_ASSERT(roleNames().contains(role), return {});

    return m_bundleCategories.at(index.row())->property(roleNames().value(role));
}

bool ContentLibraryTexturesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || !roleNames().contains(role))
        return false;

    QByteArray roleName = roleNames().value(role);
    ContentLibraryTexturesCategory *bundleCategory = m_bundleCategories.at(index.row());
    QVariant currValue = bundleCategory->property(roleName);

    if (currValue != value) {
        bundleCategory->setProperty(roleName, value);

        emit dataChanged(index, index, {role});
        return true;
    }

    return false;
}

bool ContentLibraryTexturesModel::isValidIndex(int idx) const
{
    return idx > -1 && idx < rowCount();
}

void ContentLibraryTexturesModel::updateIsEmpty()
{
    const bool anyCatVisible = Utils::anyOf(m_bundleCategories,
                                            [&](ContentLibraryTexturesCategory *cat) {
        return cat->visible();
    });

    const bool newEmpty = !anyCatVisible || m_bundleCategories.isEmpty();

    if (newEmpty != m_isEmpty) {
        m_isEmpty = newEmpty;
        emit isEmptyChanged();
    }
}

QHash<int, QByteArray> ContentLibraryTexturesModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        {Qt::UserRole + 1, "bundleCategoryName"},
        {Qt::UserRole + 2, "bundleCategoryVisible"},
        {Qt::UserRole + 3, "bundleCategoryExpanded"},
        {Qt::UserRole + 4, "bundleCategoryTextures"}
    };
    return roles;
}

/**
 * @brief Load the bundle categorized icons. Actual textures are downloaded on demand
 *
 * @param bundlePath local path to the bundle folder and icons
 * @param metaData bundle textures metadata
 */
void ContentLibraryTexturesModel::loadTextureBundle(const QString &remoteUrl, const QString &iconsUrl,
                                                    const QString &bundleIconPath,
                                                    const QVariantMap &metaData)
{
    if (!m_bundleCategories.isEmpty())
        return;

    QDir bundleDir = QString("%1/%2").arg(bundleIconPath, m_category);
    if (!bundleDir.exists()) {
        qWarning() << __FUNCTION__ << "textures bundle folder doesn't exist." << bundleDir.absolutePath();
        return;
    }

    const QVariantMap imageItems = metaData.value("image_items").toMap();

    const QFileInfoList dirs = bundleDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &dir : dirs) {
        auto category = new ContentLibraryTexturesCategory(this, dir.fileName());
        const QFileInfoList texFiles = QDir(dir.filePath()).entryInfoList(QDir::Files);
        for (const QFileInfo &tex : texFiles) {
            QString textureUrl = QString("%1/%2/%3.zip").arg(remoteUrl, dir.fileName(), tex.baseName());
            QString iconUrl = QString("%1/%2/%3.png").arg(iconsUrl, dir.fileName(), tex.baseName());

            QString localDownloadPath = QString("%1/%2/%3")
                                            .arg(Paths::bundlesPathSetting(),
                                                 m_category,
                                                 dir.fileName());
            QString key = QString("%1/%2/%3").arg(m_category, dir.fileName(), tex.baseName());
            QString fileExt;
            QSize dimensions;
            qint64 sizeInBytes = -1;
            bool hasUpdate = false;
            bool isNew = false;

            if (imageItems.contains(key)) {
                QVariantMap dataMap = imageItems[key].toMap();
                fileExt = '.' + dataMap.value("format").toString();
                dimensions = QSize(dataMap.value("width").toInt(), dataMap.value("height").toInt());
                sizeInBytes = dataMap.value("file_size").toLongLong();
                hasUpdate = m_modifiedFiles.contains(key);
                isNew = m_newFiles.contains(key);
            }

            category->addTexture(tex, localDownloadPath, key, textureUrl, iconUrl, fileExt,
                                 dimensions, sizeInBytes, hasUpdate, isNew);
        }
        m_bundleCategories.append(category);
    }

    resetModel();
    updateIsEmpty();
}

void ContentLibraryTexturesModel::setModifiedFileEntries(const QVariantMap &files)
{
    m_modifiedFiles.clear();

    const QString prefix = m_category + "/";
    const QStringList keys = files.keys();

    for (const QString &key : keys) {
        if (key.startsWith(prefix))
            m_modifiedFiles[key] = files[key];
    }
}

void ContentLibraryTexturesModel::setNewFileEntries(const QStringList &newFiles)
{
    const QString prefix = m_category + "/";

    m_newFiles = Utils::filteredCast<QSet<QString>>(newFiles, [&prefix](const QString &key) {
        return key.startsWith(prefix);
    });
}

QString ContentLibraryTexturesModel::removeModifiedFileEntry(const QString &key)
{
    if (m_modifiedFiles.contains(key)) {
        QVariantMap item = m_modifiedFiles[key].toMap();
        m_modifiedFiles.remove(key);
        return item["checksum"].toString();
    } else {
        return {};
    }
}

void ContentLibraryTexturesModel::markTextureHasNoUpdates(const QString &subcategory,
                                                          const QString &textureKey)
{
    auto *categ = Utils::findOrDefault(m_bundleCategories,
                                       [&subcategory](ContentLibraryTexturesCategory *c) {
                                           return c->name() == subcategory;
                                       });
    if (!categ)
        return;

    categ->markTextureHasNoUpdate(textureKey);
}

bool ContentLibraryTexturesModel::texBundleExists() const
{
    return !m_bundleCategories.isEmpty();
}

bool ContentLibraryTexturesModel::hasSceneEnv() const
{
    return m_hasSceneEnv;
}

void ContentLibraryTexturesModel::setHasSceneEnv(bool b)
{
    if (b == m_hasSceneEnv)
        return;

    m_hasSceneEnv = b;
    emit hasSceneEnvChanged();
}

void ContentLibraryTexturesModel::setSearchText(const QString &searchText)
{
    QString lowerSearchText = searchText.toLower();

    if (m_searchText == lowerSearchText)
        return;

    m_searchText = lowerSearchText;

    for (int i = 0; i < m_bundleCategories.size(); ++i) {
        ContentLibraryTexturesCategory *cat = m_bundleCategories.at(i);
        bool catVisibilityChanged = cat->filter(m_searchText);
        if (catVisibilityChanged)
            emit dataChanged(index(i), index(i), {roleNames().keys("bundleCategoryVisible")});
    }

    updateIsEmpty();
}

void ContentLibraryTexturesModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

} // namespace QmlDesigner
