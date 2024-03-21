// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibraryusermodel.h"

#include "contentlibrarybundleimporter.h"
#include "contentlibrarymaterial.h"
#include "contentlibrarymaterialscategory.h"
#include "contentlibrarywidget.h"

#include <designerpaths.h>
#include "qmldesignerconstants.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QUrl>

namespace QmlDesigner {

ContentLibraryUserModel::ContentLibraryUserModel(ContentLibraryWidget *parent)
    : QAbstractListModel(parent)
    , m_widget(parent)
{
    m_userCategories = {tr("Materials")/*, tr("Textures"), tr("3D"), tr("Effects"), tr("2D components")*/}; // TODO

    loadUserBundle();
}

int ContentLibraryUserModel::rowCount(const QModelIndex &) const
{
    return m_userCategories.size();
}

QVariant ContentLibraryUserModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_userCategories.size(), return {});
    QTC_ASSERT(roleNames().contains(role), return {});

    if (role == NameRole)
        return m_userCategories.at(index.row());

    if (role == ItemsRole) {
        if (index.row() == 0)
            return QVariant::fromValue(m_userMaterials);
        if (index.row() == 1)
            return QVariant::fromValue(m_userTextures);
        if (index.row() == 2)
            return QVariant::fromValue(m_user3DItems);
        if (index.row() == 3)
            return QVariant::fromValue(m_userEffects);
    }

    if (role == VisibleRole)
        return true; // TODO

    if (role == ExpandedRole)
        return true; // TODO

    return {};
}

bool ContentLibraryUserModel::isValidIndex(int idx) const
{
    return idx > -1 && idx < rowCount();
}

void ContentLibraryUserModel::updateIsEmpty()
{
    bool anyMatVisible = Utils::anyOf(m_userMaterials, [&](ContentLibraryMaterial *mat) {
        return mat->visible();
    });

    bool newEmpty = !anyMatVisible || !m_widget->hasMaterialLibrary() || !hasRequiredQuick3DImport();

    if (newEmpty != m_isEmpty) {
        m_isEmpty = newEmpty;
        emit isEmptyChanged();
    }
}

QHash<int, QByteArray> ContentLibraryUserModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        {NameRole, "categoryName"},
        {VisibleRole, "categoryVisible"},
        {ExpandedRole, "categoryExpanded"},
        {ItemsRole, "categoryItems"}
    };
    return roles;
}

void ContentLibraryUserModel::createImporter(const QString &bundlePath, const QString &bundleId,
                                             const QStringList &sharedFiles)
{
    m_importer = new Internal::ContentLibraryBundleImporter(bundlePath, bundleId, sharedFiles);
#ifdef QDS_USE_PROJECTSTORAGE
    connect(m_importer,
            &Internal::ContentLibraryBundleImporter::importFinished,
            this,
            [&](const QmlDesigner::TypeName &typeName) {
                m_importerRunning = false;
                emit importerRunningChanged();
                if (typeName.size())
                    emit bundleMaterialImported(typeName);
            });
#else
    connect(m_importer,
            &Internal::ContentLibraryBundleImporter::importFinished,
            this,
            [&](const QmlDesigner::NodeMetaInfo &metaInfo) {
                m_importerRunning = false;
                emit importerRunningChanged();
                if (metaInfo.isValid())
                    emit bundleMaterialImported(metaInfo);
            });
#endif

    connect(m_importer, &Internal::ContentLibraryBundleImporter::unimportFinished, this,
            [&](const QmlDesigner::NodeMetaInfo &metaInfo) {
                Q_UNUSED(metaInfo)
                m_importerRunning = false;
                emit importerRunningChanged();
                emit bundleMaterialUnimported(metaInfo);
            });

    resetModel();
    updateIsEmpty();
}

void ContentLibraryUserModel::loadUserBundle()
{
    if (m_matBundleExists)
        return;

    QDir bundleDir{Paths::bundlesPathSetting() + "/User/materials"};

    if (m_bundleObj.isEmpty()) {
        QFile matsJsonFile(bundleDir.filePath("user_materials_bundle.json"));

        if (!matsJsonFile.open(QIODevice::ReadOnly)) {
            qWarning("Couldn't open user_materials_bundle.json");
            return;
        }

        QJsonDocument matBundleJsonDoc = QJsonDocument::fromJson(matsJsonFile.readAll());
        if (matBundleJsonDoc.isNull()) {
            qWarning("Invalid user_materials_bundle.json file");
            return;
        } else {
            m_bundleObj = matBundleJsonDoc.object();
        }
    }

    QString bundleId = m_bundleObj.value("id").toString();

    // parse materials
    const QJsonObject matsObj = m_bundleObj.value("materials").toObject();
    const QStringList materialNames = matsObj.keys();
    for (const QString &matName : materialNames) {
        const QJsonObject matObj = matsObj.value(matName).toObject();

        QStringList files;
        const QJsonArray assetsArr = matObj.value("files").toArray();
        for (const auto /*QJson{Const,}ValueRef*/ &asset : assetsArr)
            files.append(asset.toString());

        QUrl icon = QUrl::fromLocalFile(bundleDir.filePath(matObj.value("icon").toString()));
        QString qml = matObj.value("qml").toString();

        TypeName type = QLatin1String("%1.%2.%3").arg(
                                QLatin1String(Constants::COMPONENT_BUNDLES_FOLDER).mid(1),
                                bundleId,
                                qml.chopped(4)).toLatin1(); // chopped(4): remove .qml

        auto userMat = new ContentLibraryMaterial(this, matName, qml, type, icon, files,
                                                  bundleDir.path(), "");

        m_userMaterials.append(userMat);
    }

    QStringList sharedFiles;
    const QJsonArray sharedFilesArr = m_bundleObj.value("sharedFiles").toArray();
    for (const auto /*QJson{Const,}ValueRef*/ &file : sharedFilesArr)
        sharedFiles.append(file.toString());

    createImporter(bundleDir.path(), bundleId, sharedFiles);

    m_matBundleExists = true;
    emit matBundleExistsChanged();
}

bool ContentLibraryUserModel::hasRequiredQuick3DImport() const
{
    return m_widget->hasQuick3DImport() && m_quick3dMajorVersion == 6 && m_quick3dMinorVersion >= 3;
}

bool ContentLibraryUserModel::matBundleExists() const
{
    return m_matBundleExists;
}

Internal::ContentLibraryBundleImporter *ContentLibraryUserModel::bundleImporter() const
{
    return m_importer;
}

void ContentLibraryUserModel::setSearchText(const QString &searchText)
{
    QString lowerSearchText = searchText.toLower();

    if (m_searchText == lowerSearchText)
        return;

    m_searchText = lowerSearchText;

    for (ContentLibraryMaterial *mat : std::as_const(m_userMaterials))
        mat->filter(m_searchText);

    updateIsEmpty();
}

void ContentLibraryUserModel::updateImportedState(const QStringList &importedMats)
{
    bool changed = false;

    for (ContentLibraryMaterial *mat : std::as_const(m_userMaterials))
        changed |= mat->setImported(importedMats.contains(mat->qml().chopped(4)));

    if (changed)
        resetModel();
}

void ContentLibraryUserModel::setQuick3DImportVersion(int major, int minor)
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

void ContentLibraryUserModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void ContentLibraryUserModel::applyToSelected(ContentLibraryMaterial *mat, bool add)
{
    emit applyToSelectedTriggered(mat, add);
}

void ContentLibraryUserModel::addToProject(ContentLibraryMaterial *mat)
{
    QString err = m_importer->importComponent(mat->qml(), mat->files());

    if (err.isEmpty()) {
        m_importerRunning = true;
        emit importerRunningChanged();
    } else {
        qWarning() << __FUNCTION__ << err;
    }
}

void ContentLibraryUserModel::removeFromProject(ContentLibraryMaterial *mat)
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

bool ContentLibraryUserModel::hasModelSelection() const
{
    return m_hasModelSelection;
}

void ContentLibraryUserModel::setHasModelSelection(bool b)
{
    if (b == m_hasModelSelection)
        return;

    m_hasModelSelection = b;
    emit hasModelSelectionChanged();
}

} // namespace QmlDesigner
