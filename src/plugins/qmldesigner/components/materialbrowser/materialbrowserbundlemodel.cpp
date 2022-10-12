/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "materialbrowserbundlemodel.h"

#include "bundleimporter.h"
#include "bundlematerial.h"
#include "bundlematerialcategory.h"
#include "qmldesignerconstants.h"
#include "utils/qtcassert.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>

namespace QmlDesigner {

MaterialBrowserBundleModel::MaterialBrowserBundleModel(QObject *parent)
    : QAbstractListModel(parent)
{
    loadMaterialBundle();
}

int MaterialBrowserBundleModel::rowCount(const QModelIndex &) const
{
    return m_bundleCategories.size();
}

QVariant MaterialBrowserBundleModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_bundleCategories.count(), return {});
    QTC_ASSERT(roleNames().contains(role), return {});

    QByteArray roleName = roleNames().value(role);
    if (roleName == "bundleCategory")
        return m_bundleCategories.at(index.row())->name();

    if (roleName == "bundleCategoryVisible")
        return m_bundleCategories.at(index.row())->visible();

    if (roleName == "bundleMaterialsModel")
        return QVariant::fromValue(m_bundleCategories.at(index.row())->categoryMaterials());

    return {};
}

bool MaterialBrowserBundleModel::isValidIndex(int idx) const
{
    return idx > -1 && idx < rowCount();
}

QHash<int, QByteArray> MaterialBrowserBundleModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        {Qt::UserRole + 1, "bundleCategory"},
        {Qt::UserRole + 2, "bundleCategoryVisible"},
        {Qt::UserRole + 3, "bundleMaterialsModel"}
    };
    return roles;
}

void MaterialBrowserBundleModel::loadMaterialBundle()
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
        auto category = new BundleMaterialCategory(this, cat);

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

            auto bundleMat = new BundleMaterial(category, mat, qml, type, icon, files);

            category->addBundleMaterial(bundleMat);
        }
        m_bundleCategories.append(category);
    }

    QStringList sharedFiles;
    const QJsonArray sharedFilesArr = m_matBundleObj.value("sharedFiles").toArray();
    for (const auto /*QJson{Const,}ValueRef*/ &file : sharedFilesArr)
        sharedFiles.append(file.toString());

    m_importer = new Internal::BundleImporter(matBundleDir.path(), bundleId, sharedFiles);
    connect(m_importer, &Internal::BundleImporter::importFinished, this, [&](const QmlDesigner::NodeMetaInfo &metaInfo) {
        m_importerRunning = false;
        emit importerRunningChanged();
        if (metaInfo.isValid())
            emit bundleMaterialImported(metaInfo);
    });

    connect(m_importer, &Internal::BundleImporter::unimportFinished, this, [&](const QmlDesigner::NodeMetaInfo &metaInfo) {
        Q_UNUSED(metaInfo)
        m_importerRunning = false;
        emit importerRunningChanged();
        emit bundleMaterialUnimported(metaInfo);
    });
}

bool MaterialBrowserBundleModel::hasQuick3DImport() const
{
    return m_hasQuick3DImport;
}

void MaterialBrowserBundleModel::setHasQuick3DImport(bool b)
{
    if (b == m_hasQuick3DImport)
        return;

    m_hasQuick3DImport = b;
    emit hasQuick3DImportChanged();
}

bool MaterialBrowserBundleModel::hasMaterialRoot() const
{
    return m_hasMaterialRoot;
}

void MaterialBrowserBundleModel::setHasMaterialRoot(bool b)
{
    if (m_hasMaterialRoot == b)
        return;

    m_hasMaterialRoot = b;
    emit hasMaterialRootChanged();
}

Internal::BundleImporter *MaterialBrowserBundleModel::bundleImporter() const
{
    return m_importer;
}

void MaterialBrowserBundleModel::setSearchText(const QString &searchText)
{
    QString lowerSearchText = searchText.toLower();

    if (m_searchText == lowerSearchText)
        return;

    m_searchText = lowerSearchText;

    bool anyCatVisible = false;
    bool catVisibilityChanged = false;

    for (BundleMaterialCategory *cat : std::as_const(m_bundleCategories)) {
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

void MaterialBrowserBundleModel::updateImportedState(const QStringList &importedMats)
{
    bool changed = false;
    for (BundleMaterialCategory *cat : std::as_const(m_bundleCategories))
        changed |= cat->updateImportedState(importedMats);

    if (changed)
        resetModel();
}

void MaterialBrowserBundleModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void MaterialBrowserBundleModel::applyToSelected(BundleMaterial *mat, bool add)
{
    emit applyToSelectedTriggered(mat, add);
}

void MaterialBrowserBundleModel::addToProject(BundleMaterial *mat)
{
    QString err = m_importer->importComponent(mat->qml(), mat->files());

    if (err.isEmpty()) {
        m_importerRunning = true;
        emit importerRunningChanged();
    } else {
        qWarning() << __FUNCTION__ << err;
    }
}

void MaterialBrowserBundleModel::removeFromProject(BundleMaterial *mat)
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

} // namespace QmlDesigner
