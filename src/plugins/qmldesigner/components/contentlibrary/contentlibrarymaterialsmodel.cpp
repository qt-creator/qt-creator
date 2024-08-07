// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibrarymaterialsmodel.h"

#include "contentlibrarybundleimporter.h"
#include "contentlibrarymaterial.h"
#include "contentlibrarymaterialscategory.h"
#include "contentlibrarywidget.h"

#include "designerpaths.h"
#include "filedownloader.h"
#include "fileextractor.h"
#include "multifiledownloader.h"

#include <qmldesignerplugin.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QUrl>

namespace QmlDesigner {

ContentLibraryMaterialsModel::ContentLibraryMaterialsModel(ContentLibraryWidget *parent)
    : QAbstractListModel(parent)
    , m_widget(parent)
{
    m_bundlePath = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/Materials");

    m_baseUrl = QmlDesignerPlugin::settings()
                    .value(DesignerSettingsKey::DOWNLOADABLE_BUNDLES_URL)
                    .toString() + "/materials/v1";

    qmlRegisterType<QmlDesigner::FileDownloader>("WebFetcher", 1, 0, "FileDownloader");
    qmlRegisterType<QmlDesigner::MultiFileDownloader>("WebFetcher", 1, 0, "MultiFileDownloader");
}

void ContentLibraryMaterialsModel::loadBundle()
{
    if (fetchBundleJsonFile() && fetchBundleIcons())
        loadMaterialBundle();
}

int ContentLibraryMaterialsModel::rowCount(const QModelIndex &) const
{
    return m_bundleCategories.size();
}

QVariant ContentLibraryMaterialsModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_bundleCategories.size(), return {});
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

bool ContentLibraryMaterialsModel::fetchBundleIcons()
{
    Utils::FilePath iconsFilePath = m_bundlePath.pathAppended("icons");

    if (iconsFilePath.exists() && iconsFilePath.dirEntries(QDir::Files | QDir::Dirs
                                                           | QDir::NoDotAndDotDot).length() > 0) {
        return true;
    }

    QString zipFileUrl = m_baseUrl + "/icons.zip";

    FileDownloader *downloader = new FileDownloader(this);
    downloader->setUrl(zipFileUrl);
    downloader->setProbeUrl(false);
    downloader->setDownloadEnabled(true);

    QObject::connect(downloader, &FileDownloader::finishedChanged, this,
                     [this, downloader] {
        FileExtractor *extractor = new FileExtractor(this);
        extractor->setArchiveName(downloader->completeBaseName());
        extractor->setSourceFile(downloader->outputFile());
        extractor->setTargetPath(m_bundlePath.toFSPathString());
        extractor->setAlwaysCreateDir(false);
        extractor->setClearTargetPathContents(false);

        QObject::connect(extractor, &FileExtractor::finishedChanged, this,
                         [this, downloader, extractor] {
            downloader->deleteLater();
            extractor->deleteLater();

            loadMaterialBundle();
        });

        extractor->extract();
    });

    downloader->start();
    return false;
}

bool ContentLibraryMaterialsModel::fetchBundleJsonFile()
{
    Utils::FilePath jsonFilePath = m_bundlePath.pathAppended("material_bundle.json");

    if (jsonFilePath.exists() && jsonFilePath.fileSize() > 0)
        return true;

    QString bundleJsonUrl = m_baseUrl + "/material_bundle.json";

    FileDownloader *downloader = new FileDownloader(this);
    downloader->setUrl(bundleJsonUrl);
    downloader->setProbeUrl(false);
    downloader->setDownloadEnabled(true);
    downloader->setTargetFilePath(jsonFilePath.toFSPathString());

    QObject::connect(downloader, &FileDownloader::finishedChanged, this,
                     [this, downloader] {
        if (fetchBundleIcons())
            loadMaterialBundle();
        downloader->deleteLater();
    });

    downloader->start();
    return false;
}

void ContentLibraryMaterialsModel::downloadSharedFiles()
{
    QString metaFileUrl = m_baseUrl + "/shared_files.zip";
    FileDownloader *downloader = new FileDownloader(this);
    downloader->setUrl(metaFileUrl);
    downloader->setProbeUrl(false);
    downloader->setDownloadEnabled(true);

    QObject::connect(downloader, &FileDownloader::finishedChanged, this,
                     [this, downloader] {
        FileExtractor *extractor = new FileExtractor(this);
        extractor->setArchiveName(downloader->completeBaseName());
        extractor->setSourceFile(downloader->outputFile());
        extractor->setTargetPath(m_bundlePath.toFSPathString());
        extractor->setAlwaysCreateDir(false);
        extractor->setClearTargetPathContents(false);

        QObject::connect(extractor, &FileExtractor::finishedChanged, this, [downloader, extractor]() {
            downloader->deleteLater();
            extractor->deleteLater();
        });

        extractor->extract();
    });

    downloader->start();
}

QString ContentLibraryMaterialsModel::bundleId() const
{
    return m_bundleId;
}

void ContentLibraryMaterialsModel::loadMaterialBundle(bool forceReload)
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();

    if (!forceReload && m_bundleExists && m_bundleId == compUtils.materialsBundleId())
        return;

    // clean up
    qDeleteAll(m_bundleCategories);
    m_bundleCategories.clear();
    m_bundleExists = false;
    m_isEmpty = true;
    m_bundleObj = {};
    m_bundleId.clear();

    Utils::FilePath jsonFilePath = m_bundlePath.pathAppended("material_bundle.json");

    Utils::expected_str<QByteArray> jsonContents = jsonFilePath.fileContents();
    if (!jsonContents.has_value()) {
        qWarning() << __FUNCTION__ << jsonContents.error();
        resetModel();
        return;
    }

    QJsonDocument bundleJsonDoc = QJsonDocument::fromJson(jsonContents.value());
    if (bundleJsonDoc.isNull()) {
        qWarning() << __FUNCTION__ << "Invalid json file" << jsonFilePath;
        resetModel();
        return;
    }

    m_bundleObj = bundleJsonDoc.object();

    QString bundleType = compUtils.materialsBundleType();
    m_bundleId = compUtils.materialsBundleId();

    const QJsonObject catsObj = m_bundleObj.value("categories").toObject();
    const QStringList categories = catsObj.keys();
    for (const QString &cat : categories) {
        auto category = new ContentLibraryMaterialsCategory(this, cat);

        const QJsonObject matsObj = catsObj.value(cat).toObject();
        const QStringList matsNames = matsObj.keys();
        for (const QString &matName : matsNames) {
            const QJsonObject matObj = matsObj.value(matName).toObject();

            if (matObj.contains("minQtVersion")) {
                const Version minQtVersion = Version::fromString(
                    matObj.value("minQtVersion").toString());
                if (minQtVersion > m_quick3dVersion)
                    continue;
            }

            QStringList files;
            const QJsonArray assetsArr = matObj.value("files").toArray();
            for (const QJsonValueConstRef &asset : assetsArr)
                files.append(asset.toString());

            QUrl icon = m_bundlePath.pathAppended(matObj.value("icon").toString()).toUrl();
            QString qml = matObj.value("qml").toString();
            TypeName type = QLatin1String("%1.%2").arg(bundleType, qml.chopped(4)).toLatin1(); // chopped(4): remove .qml

            auto bundleMat = new ContentLibraryMaterial(category, matName, qml, type, icon, files,
                                                        m_bundleId);

            category->addBundleMaterial(bundleMat);
        }
        m_bundleCategories.append(category);
    }

    m_bundleSharedFiles = m_bundleObj.value("sharedFiles").toVariant().toStringList();
    for (const QString &file : std::as_const(m_bundleSharedFiles)) {
        if (!m_bundlePath.pathAppended(file).exists()) {
            downloadSharedFiles();
            break;
        }
    }

    m_bundleExists = true;
    updateIsEmpty();
    resetModel();
}

bool ContentLibraryMaterialsModel::hasRequiredQuick3DImport() const
{
    return m_widget->hasQuick3DImport() && m_quick3dVersion >= Version{6, 3};
}

bool ContentLibraryMaterialsModel::matBundleExists() const
{
    return m_bundleExists;
}

QString ContentLibraryMaterialsModel::bundlePath() const
{
    return m_bundlePath.toFSPathString();
}

void ContentLibraryMaterialsModel::setSearchText(const QString &searchText)
{
    QString lowerSearchText = searchText.toLower();

    if (m_searchText == lowerSearchText)
        return;

    m_searchText = lowerSearchText;

    for (int i = 0; i < m_bundleCategories.size(); ++i) {
        ContentLibraryMaterialsCategory *cat = m_bundleCategories.at(i);
        bool catVisibilityChanged = cat->filter(m_searchText);
        if (catVisibilityChanged)
            emit dataChanged(index(i), index(i), {roleNames().keys("bundleCategoryVisible")});
    }

    updateIsEmpty();
}

void ContentLibraryMaterialsModel::updateImportedState(const QStringList &importedItems)
{
    bool changed = false;
    for (ContentLibraryMaterialsCategory *cat : std::as_const(m_bundleCategories))
        changed |= cat->updateImportedState(importedItems);

    if (changed)
        resetModel();
}

void ContentLibraryMaterialsModel::setQuick3DImportVersion(int major, int minor)
{
    bool oldRequiredImport = hasRequiredQuick3DImport();

    const Version newVersion{major, minor};
    if (m_quick3dVersion != newVersion) {
        m_quick3dVersion = newVersion;
        loadMaterialBundle(true);
    }

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
    QString err = m_widget->importer()->importComponent(m_bundlePath.toFSPathString(), mat->type(),
                                                        mat->qml(), mat->files() + m_bundleSharedFiles);

    if (err.isEmpty())
        m_widget->setImporterRunning(true);
    else
        qWarning() << __FUNCTION__ << err;
}

void ContentLibraryMaterialsModel::removeFromProject(ContentLibraryMaterial *mat)
{
    QString err = m_widget->importer()->unimportComponent(mat->type(), mat->qml());

    if (err.isEmpty())
        m_widget->setImporterRunning(true);
    else
        qWarning() << __FUNCTION__ << err;
}

bool ContentLibraryMaterialsModel::isMaterialDownloaded(ContentLibraryMaterial *mat) const
{
    return m_bundlePath.pathAppended(mat->qml()).exists();
}

} // namespace QmlDesigner
