// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "contentlibrarytexturesmodel.h"

#include "contentlibrarytexturescategory.h"

#include "utils/algorithm.h"
#include "utils/qtcassert.h"

#include <qmldesigner/utils/fileextractor.h>
#include <qmldesigner/utils/filedownloader.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QQmlEngine>
#include <QSize>
#include <QStandardPaths>

namespace QmlDesigner {

ContentLibraryTexturesModel::ContentLibraryTexturesModel(const QString &bundleSubPath, QObject *parent)
    : QAbstractListModel(parent)
    , m_bundleSubPath(bundleSubPath)
{
    qmlRegisterType<QmlDesigner::FileDownloader>("WebFetcher", 1, 0, "FileDownloader");
    qmlRegisterType<QmlDesigner::FileExtractor>("WebFetcher", 1, 0, "FileExtractor");

    static const QString baseDownloadPath =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        + "/QtDesignStudio/bundles";

    m_downloadPath = baseDownloadPath + '/' + bundleSubPath;
}

int ContentLibraryTexturesModel::rowCount(const QModelIndex &) const
{
    return m_bundleCategories.size();
}

QVariant ContentLibraryTexturesModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_bundleCategories.count(), return {});
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

void ContentLibraryTexturesModel::loadTextureBundle(const QString &bundlePath, const QString &baseUrl,
                                                    const QVariantMap &metaData)
{
    QDir bundleDir = QDir(bundlePath);
    if (!bundleDir.exists()) {
        qWarning() << __FUNCTION__ << "textures bundle folder doesn't exist." << bundlePath;
        return;
    }

    if (!m_bundleCategories.isEmpty())
        return;

    const QFileInfoList dirs = bundleDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &dir : dirs) {
        auto category = new ContentLibraryTexturesCategory(this, dir.fileName());
        const QFileInfoList texFiles = QDir(dir.filePath() + "/icon").entryInfoList(QDir::Files);
        for (const QFileInfo &tex : texFiles) {
            QString zipPath = '/' + dir.fileName() + '/' + tex.baseName() + ".zip";
            QString urlPath = baseUrl + zipPath;
            QString downloadPath = m_downloadPath + '/' + dir.fileName();
            QString fullZipPath = m_bundleSubPath + zipPath;
            QString fileExt;
            QSize dimensions;
            qint64 sizeInBytes = -1;

            if (metaData.contains(fullZipPath)) {
                QVariantMap dataMap = metaData[fullZipPath].toMap();
                fileExt = '.' + dataMap.value("format").toString();
                dimensions = QSize(dataMap.value("width").toInt(), dataMap.value("height").toInt());
                sizeInBytes = dataMap.value("size").toLongLong();
            }

            category->addTexture(tex, downloadPath, urlPath, fileExt, dimensions, sizeInBytes);
        }
        m_bundleCategories.append(category);
    }

    updateIsEmpty();
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

    bool catVisibilityChanged = false;

    for (ContentLibraryTexturesCategory *cat : std::as_const(m_bundleCategories))
        catVisibilityChanged |= cat->filter(m_searchText);

    updateIsEmpty();

    if (catVisibilityChanged)
        resetModel();
}

void ContentLibraryTexturesModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

} // namespace QmlDesigner
