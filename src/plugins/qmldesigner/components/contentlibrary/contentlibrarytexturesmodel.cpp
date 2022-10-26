// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "contentlibrarytexturesmodel.h"

#include "contentlibrarytexturescategory.h"
#include "utils/qtcassert.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QUrl>

namespace QmlDesigner {

ContentLibraryTexturesModel::ContentLibraryTexturesModel(QObject *parent)
    : QAbstractListModel(parent)
{
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

void ContentLibraryTexturesModel::loadTextureBundle(const QString &bundlePath)
{
    QDir bundleDir = QDir(bundlePath);
    if (!bundleDir.exists()) {
        qWarning() << __FUNCTION__ << "textures bundle folder doesn't exist." << bundlePath;
        return;
    }

    if (m_texBundleLoaded)
        return;

    const QFileInfoList dirs = bundleDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &dir : dirs) {
        auto category = new ContentLibraryTexturesCategory(this, dir.fileName());
        const QFileInfoList texFiles = QDir(dir.filePath()).entryInfoList(QDir::Files);
        for (const QFileInfo &tex : texFiles)
            category->addTexture(tex);
        m_bundleCategories.append(category);
    }
}

bool ContentLibraryTexturesModel::hasQuick3DImport() const
{
    return m_hasQuick3DImport;
}

void ContentLibraryTexturesModel::setHasQuick3DImport(bool b)
{
    if (b == m_hasQuick3DImport)
        return;

    m_hasQuick3DImport = b;
    emit hasQuick3DImportChanged();
}

bool ContentLibraryTexturesModel::hasMaterialRoot() const
{
    return m_hasMaterialRoot;
}

void ContentLibraryTexturesModel::setHasMaterialRoot(bool b)
{
    if (m_hasMaterialRoot == b)
        return;

    m_hasMaterialRoot = b;
    emit hasMaterialRootChanged();
}

bool ContentLibraryTexturesModel::matBundleExists() const
{
    return m_texBundleLoaded && m_quick3dMajorVersion == 6 && m_quick3dMinorVersion >= 3;
}

void ContentLibraryTexturesModel::setSearchText(const QString &searchText)
{
    QString lowerSearchText = searchText.toLower();

    if (m_searchText == lowerSearchText)
        return;

    m_searchText = lowerSearchText;

    bool anyCatVisible = false;
    bool catVisibilityChanged = false;

    for (ContentLibraryTexturesCategory *cat : std::as_const(m_bundleCategories)) {
        catVisibilityChanged |= cat->filter(m_searchText);
        anyCatVisible |= cat->visible();
    }

    if (anyCatVisible == m_isEmpty) {
        m_isEmpty = !anyCatVisible;
        emit isEmptyChanged();
    }

    if (catVisibilityChanged)
        resetModel();
}

void ContentLibraryTexturesModel::setQuick3DImportVersion(int major, int minor)
{
    bool bundleExisted = matBundleExists();

    m_quick3dMajorVersion = major;
    m_quick3dMinorVersion = minor;

    if (bundleExisted != matBundleExists())
        emit matBundleExistsChanged();
}

void ContentLibraryTexturesModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void ContentLibraryTexturesModel::addToProject(const QString &mat)
{
    // TODO: import asset
}

bool ContentLibraryTexturesModel::hasModelSelection() const
{
    return m_hasModelSelection;
}

void ContentLibraryTexturesModel::setHasModelSelection(bool b)
{
    if (b == m_hasModelSelection)
        return;

    m_hasModelSelection = b;
    emit hasModelSelectionChanged();
}

} // namespace QmlDesigner
