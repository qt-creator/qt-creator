// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibrarymaterialsmodel.h"

#include "contentlibrarybundleimporter.h"
#include "contentlibrarymaterial.h"
#include "contentlibrarymaterialscategory.h"
#include "contentlibrarywidget.h"
#include "qmldesignerconstants.h"

#include "utils/algorithm.h"
#include "utils/qtcassert.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>

namespace QmlDesigner {

ContentLibraryMaterialsModel::ContentLibraryMaterialsModel(ContentLibraryWidget *parent)
    : QAbstractListModel(parent)
    , m_widget(parent)
{
    loadMaterialBundle();
}

int ContentLibraryMaterialsModel::rowCount(const QModelIndex &) const
{
    return m_bundleCategories.size();
}

QVariant ContentLibraryMaterialsModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_bundleCategories.count(), return {});
    QTC_ASSERT(roleNames().contains(role), return {});

    return m_bundleCategories.at(index.row())->property(roleNames().value(role));
}

bool ContentLibraryMaterialsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || !roleNames().contains(role))
        return false;

    QByteArray roleName = roleNames().value(role);
    ContentLibraryMaterialsCategory *bundleCategory = m_bundleCategories.at(index.row());
    QVariant currValue = bundleCategory->property(roleName);

    if (currValue != value) {
        bundleCategory->setProperty(roleName, value);

        emit dataChanged(index, index, {role});
        return true;
    }

    return false;
}

bool ContentLibraryMaterialsModel::isValidIndex(int idx) const
{
    return idx > -1 && idx < rowCount();
}

void ContentLibraryMaterialsModel::updateIsEmpty()
{
    const bool anyCatVisible = Utils::anyOf(m_bundleCategories,
                                            [&](ContentLibraryMaterialsCategory *cat) {
        return cat->visible();
    });

    const bool newEmpty = !anyCatVisible || m_bundleCategories.isEmpty()
            || !m_widget->hasMaterialLibrary() || !hasRequiredQuick3DImport();

    if (newEmpty != m_isEmpty) {
        m_isEmpty = newEmpty;
        emit isEmptyChanged();
    }
}

QHash<int, QByteArray> ContentLibraryMaterialsModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        {Qt::UserRole + 1, "bundleCategoryName"},
        {Qt::UserRole + 2, "bundleCategoryVisible"},
        {Qt::UserRole + 3, "bundleCategoryExpanded"},
        {Qt::UserRole + 4, "bundleCategoryMaterials"}
    };
    return roles;
}

void ContentLibraryMaterialsModel::loadMaterialBundle()
{
    if (m_matBundleExists || m_probeMatBundleDir)
        return;

    QDir matBundleDir(qEnvironmentVariable("MATERIAL_BUNDLE_PATH"));

    // search for matBundleDir from exec dir and up
    if (matBundleDir.dirName() == ".") {
        m_probeMatBundleDir = true; // probe only once

        matBundleDir.setPath(QCoreApplication::applicationDirPath());
        while (!matBundleDir.cd("material_bundle") && matBundleDir.cdUp())
            ; // do nothing

        if (matBundleDir.dirName() != "material_bundle") // bundlePathDir not found
            return;
    }

    QString matBundlePath = matBundleDir.filePath("material_bundle.json");

    if (m_matBundleObj.isEmpty()) {
        QFile matPropsFile(matBundlePath);

        if (!matPropsFile.open(QIODevice::ReadOnly)) {
            qWarning("Couldn't open material_bundle.json");
            return;
        }

        QJsonDocument matBundleJsonDoc = QJsonDocument::fromJson(matPropsFile.readAll());
        if (matBundleJsonDoc.isNull()) {
            qWarning("Invalid material_bundle.json file");
            return;
        } else {
            m_matBundleObj = matBundleJsonDoc.object();
        }
    }

    m_matBundleExists = true;

    QString bundleId = m_matBundleObj.value("id").toString();

    const QJsonObject catsObj = m_matBundleObj.value("categories").toObject();
    const QStringList categories = catsObj.keys();
    for (const QString &cat : categories) {
        auto category = new ContentLibraryMaterialsCategory(this, cat);

        const QJsonObject matsObj = catsObj.value(cat).toObject();
        const QStringList mats = matsObj.keys();
        for (const QString &mat : mats) {
            const QJsonObject matObj = matsObj.value(mat).toObject();

            QStringList files;
            const QJsonArray assetsArr = matObj.value("files").toArray();
            for (const auto /*QJson{Const,}ValueRef*/ &asset : assetsArr)
                files.append(asset.toString());

            QUrl icon = QUrl::fromLocalFile(matBundleDir.filePath(matObj.value("icon").toString()));
            QString qml = matObj.value("qml").toString();
            TypeName type = QLatin1String("%1.%2.%3").arg(
                                    QLatin1String(Constants::COMPONENT_BUNDLES_FOLDER).mid(1),
                                    bundleId,
                                    qml.chopped(4)).toLatin1(); // chopped(4): remove .qml

            auto bundleMat = new ContentLibraryMaterial(category, mat, qml, type, icon, files);

            category->addBundleMaterial(bundleMat);
        }
        m_bundleCategories.append(category);
    }

    QStringList sharedFiles;
    const QJsonArray sharedFilesArr = m_matBundleObj.value("sharedFiles").toArray();
    for (const auto /*QJson{Const,}ValueRef*/ &file : sharedFilesArr)
        sharedFiles.append(file.toString());

    m_importer = new Internal::ContentLibraryBundleImporter(matBundleDir.path(), bundleId, sharedFiles);
    connect(m_importer, &Internal::ContentLibraryBundleImporter::importFinished, this,
            [&](const QmlDesigner::NodeMetaInfo &metaInfo) {
        m_importerRunning = false;
        emit importerRunningChanged();
        if (metaInfo.isValid())
            emit bundleMaterialImported(metaInfo);
    });

    connect(m_importer, &Internal::ContentLibraryBundleImporter::unimportFinished, this,
            [&](const QmlDesigner::NodeMetaInfo &metaInfo) {
        Q_UNUSED(metaInfo)
        m_importerRunning = false;
        emit importerRunningChanged();
        emit bundleMaterialUnimported(metaInfo);
    });

    updateIsEmpty();
}

bool ContentLibraryMaterialsModel::hasRequiredQuick3DImport() const
{
    return m_widget->hasQuick3DImport() && m_quick3dMajorVersion == 6 && m_quick3dMinorVersion >= 3;
}

bool ContentLibraryMaterialsModel::matBundleExists() const
{
    return m_matBundleExists;
}

Internal::ContentLibraryBundleImporter *ContentLibraryMaterialsModel::bundleImporter() const
{
    return m_importer;
}

void ContentLibraryMaterialsModel::setSearchText(const QString &searchText)
{
    QString lowerSearchText = searchText.toLower();

    if (m_searchText == lowerSearchText)
        return;

    m_searchText = lowerSearchText;

    bool catVisibilityChanged = false;
    for (ContentLibraryMaterialsCategory *cat : std::as_const(m_bundleCategories))
        catVisibilityChanged |= cat->filter(m_searchText);

    updateIsEmpty();

    if (catVisibilityChanged)
        resetModel();
}

void ContentLibraryMaterialsModel::updateImportedState(const QStringList &importedMats)
{
    bool changed = false;
    for (ContentLibraryMaterialsCategory *cat : std::as_const(m_bundleCategories))
        changed |= cat->updateImportedState(importedMats);

    if (changed)
        resetModel();
}

void ContentLibraryMaterialsModel::setQuick3DImportVersion(int major, int minor)
{
    bool oldRequiredImport = hasRequiredQuick3DImport();

    m_quick3dMajorVersion = major;
    m_quick3dMinorVersion = minor;

    bool newRequiredImport = hasRequiredQuick3DImport();

    if (oldRequiredImport == newRequiredImport)
        return;

    emit hasRequiredQuick3DImportChanged();

    updateIsEmpty();
}

void ContentLibraryMaterialsModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void ContentLibraryMaterialsModel::applyToSelected(ContentLibraryMaterial *mat, bool add)
{
    emit applyToSelectedTriggered(mat, add);
}

void ContentLibraryMaterialsModel::addToProject(ContentLibraryMaterial *mat)
{
    QString err = m_importer->importComponent(mat->qml(), mat->files());

    if (err.isEmpty()) {
        m_importerRunning = true;
        emit importerRunningChanged();
    } else {
        qWarning() << __FUNCTION__ << err;
    }
}

void ContentLibraryMaterialsModel::removeFromProject(ContentLibraryMaterial *mat)
{
    emit bundleMaterialAboutToUnimport(mat->type());

     QString err = m_importer->unimportComponent(mat->qml());

    if (err.isEmpty()) {
        m_importerRunning = true;
        emit importerRunningChanged();
    } else {
        qWarning() << __FUNCTION__ << err;
    }
}

bool ContentLibraryMaterialsModel::hasModelSelection() const
{
    return m_hasModelSelection;
}

void ContentLibraryMaterialsModel::setHasModelSelection(bool b)
{
    if (b == m_hasModelSelection)
        return;

    m_hasModelSelection = b;
    emit hasModelSelectionChanged();
}

} // namespace QmlDesigner
