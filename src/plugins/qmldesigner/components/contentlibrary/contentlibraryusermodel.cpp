// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibraryusermodel.h"

#include "contentlibrarybundleimporter.h"
#include "contentlibraryitem.h"
#include "contentlibrarytexture.h"
#include "contentlibrarywidget.h"

#include <designerpaths.h>
#include <imageutils.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <uniquename.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>

namespace QmlDesigner {

ContentLibraryUserModel::ContentLibraryUserModel(ContentLibraryWidget *parent)
    : QAbstractListModel(parent)
    , m_widget(parent)
{
    m_userCategories = {tr("Materials"), tr("Textures"), tr("3D"), /*tr("Effects"), tr("2D components")*/}; // TODO
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
        if (index.row() == MaterialsSectionIdx)
            return QVariant::fromValue(m_userMaterials);
        if (index.row() == TexturesSectionIdx)
            return QVariant::fromValue(m_userTextures);
        if (index.row() == Items3DSectionIdx)
            return QVariant::fromValue(m_user3DItems);
        if (index.row() == EffectsSectionIdx)
            return QVariant::fromValue(m_userEffects);
    }

    if (role == NoMatchRole) {
        if (index.row() == MaterialsSectionIdx)
            return m_noMatchMaterials;
        if (index.row() == TexturesSectionIdx)
            return m_noMatchTextures;
        if (index.row() == Items3DSectionIdx)
            return m_noMatch3D;
        if (index.row() == EffectsSectionIdx)
            return m_noMatchEffects;
    }

    if (role == VisibleRole) {
        if (index.row() == MaterialsSectionIdx)
            return !m_userMaterials.isEmpty();
        if (index.row() == TexturesSectionIdx)
            return !m_userTextures.isEmpty();
        if (index.row() == Items3DSectionIdx)
            return !m_user3DItems.isEmpty();
        if (index.row() == EffectsSectionIdx)
            return !m_userEffects.isEmpty();
    }

    return {};
}

void ContentLibraryUserModel::updateNoMatchMaterials()
{
    m_noMatchMaterials = Utils::allOf(m_userMaterials, [&](ContentLibraryItem *item) {
        return !item->visible();
    });
}

void ContentLibraryUserModel::updateNoMatchTextures()
{
    m_noMatchTextures = Utils::allOf(m_userTextures, [&](ContentLibraryTexture *item) {
        return !item->visible();
    });
}

void ContentLibraryUserModel::updateNoMatch3D()
{
    m_noMatch3D = Utils::allOf(m_user3DItems, [&](ContentLibraryItem *item) {
        return !item->visible();
    });
}

void ContentLibraryUserModel::addMaterial(const QString &name, const QString &qml,
                                          const QUrl &icon, const QStringList &files)
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();

    QString typePrefix = compUtils.userMaterialsBundleType();
    TypeName type = QLatin1String("%1.%2").arg(typePrefix, qml.chopped(4)).toLatin1();

    auto libMat = new ContentLibraryItem(this, name, qml, type, icon, files, "material");
    m_userMaterials.append(libMat);

    emit dataChanged(index(MaterialsSectionIdx), index(MaterialsSectionIdx));
}

void ContentLibraryUserModel::add3DItem(const QString &name, const QString &qml,
                                        const QUrl &icon, const QStringList &files)
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();

    QString typePrefix = compUtils.user3DBundleType();
    TypeName type = QLatin1String("%1.%2").arg(typePrefix, qml.chopped(4)).toLatin1();

    m_user3DItems.append(new ContentLibraryItem(this, name, qml, type, icon, files, "3d"));
}

void ContentLibraryUserModel::refreshSection(SectionIndex sectionIndex)
{
    emit dataChanged(index(sectionIndex), index(sectionIndex));
}

void ContentLibraryUserModel::addTextures(const QStringList &paths)
{
    QDir bundleDir{Paths::bundlesPathSetting() + "/User/textures"};
    bundleDir.mkpath(".");
    bundleDir.mkdir("icons");

    for (const QString &path : paths) {
        QFileInfo fileInfo(path);
        QString suffix = '.' + fileInfo.suffix();
        auto iconFileInfo = QFileInfo(fileInfo.path().append("/icons/").append(fileInfo.baseName() + ".png"));
        QPair<QSize, qint64> info = ImageUtils::imageInfo(path);
        QString dirPath = fileInfo.path();
        QSize imgDims = info.first;
        qint64 imgFileSize = info.second;

        auto tex = new ContentLibraryTexture(this, iconFileInfo, dirPath, suffix, imgDims, imgFileSize);
        m_userTextures.append(tex);
    }

    emit dataChanged(index(TexturesSectionIdx), index(TexturesSectionIdx));
}

void ContentLibraryUserModel::add3DInstance(ContentLibraryItem *bundleItem)
{
    QString err = m_widget->importer()->importComponent(m_bundlePath3D.path(), bundleItem->type(),
                                                        bundleItem->qml(),
                                                        bundleItem->files() + m_bundle3DSharedFiles);

    if (err.isEmpty())
        m_widget->setImporterRunning(true);
    else
        qWarning() << __FUNCTION__ << err;
}

void ContentLibraryUserModel::removeTexture(ContentLibraryTexture *tex)
{
    // remove resources
    Utils::FilePath::fromString(tex->texturePath()).removeFile();
    Utils::FilePath::fromString(tex->iconPath()).removeFile();

    // remove from model
    m_userTextures.removeOne(tex);
    tex->deleteLater();

    // update model
    emit dataChanged(index(TexturesSectionIdx), index(TexturesSectionIdx));
}

void ContentLibraryUserModel::removeFromContentLib(QObject *item)
{
    auto castedItem = qobject_cast<ContentLibraryItem *>(item);

    if (castedItem->itemType() == "material")
        removeMaterialFromContentLib(castedItem);
    else if (castedItem->itemType() == "3d")
        remove3DFromContentLib(castedItem);
}

void ContentLibraryUserModel::removeMaterialFromContentLib(ContentLibraryItem *item)
{
    QJsonArray itemsArr = m_bundleObjMaterial.value("items").toArray();

    // remove qml and icon files
    m_bundlePathMaterial.pathAppended(item->qml()).removeFile();
    Utils::FilePath::fromUrl(item->icon()).removeFile();

    // remove from the bundle json file
    for (int i = 0; i < itemsArr.size(); ++i) {
        if (itemsArr.at(i).toObject().value("qml") == item->qml()) {
            itemsArr.removeAt(i);
            break;
        }
    }
    m_bundleObjMaterial.insert("items", itemsArr);

    auto result = m_bundlePathMaterial.pathAppended(Constants::BUNDLE_JSON_FILENAME)
                      .writeFileContents(QJsonDocument(m_bundleObjMaterial).toJson());
    if (!result)
        qWarning() << __FUNCTION__ << result.error();

    // delete dependency files if they are only used by the deleted material
    QStringList allFiles;
    for (const QJsonValueConstRef &itemRef : std::as_const(itemsArr))
        allFiles.append(itemRef.toObject().value("files").toVariant().toStringList());

    const QStringList itemFiles = item->files();
    for (const QString &file : itemFiles) {
        if (allFiles.count(file) == 0) // only used by the deleted item
            m_bundlePathMaterial.pathAppended(file).removeFile();
    }

    // remove from model
    m_userMaterials.removeOne(item);
    item->deleteLater();

    // update model
    emit dataChanged(index(MaterialsSectionIdx), index(MaterialsSectionIdx));
}

void ContentLibraryUserModel::remove3DFromContentLibByName(const QString &qmlFileName)
{
    ContentLibraryItem *itemToRemove = Utils::findOr(m_user3DItems, nullptr,
                                             [&qmlFileName](ContentLibraryItem *item) {
        return item->qml() == qmlFileName;
    });

    if (itemToRemove)
        remove3DFromContentLib(itemToRemove);
}

void ContentLibraryUserModel::removeMaterialFromContentLibByName(const QString &qmlFileName)
{
    ContentLibraryItem *itemToRemove = Utils::findOr(m_userMaterials, nullptr,
                                                     [&qmlFileName](ContentLibraryItem *item) {
                                                         return item->qml() == qmlFileName;
                                                     });

    if (itemToRemove)
        removeMaterialFromContentLib(itemToRemove);
}

void ContentLibraryUserModel::remove3DFromContentLib(ContentLibraryItem *item)
{
    QJsonArray itemsArr = m_bundleObj3D.value("items").toArray();

    // remove qml and icon files
    m_bundlePath3D.pathAppended(item->qml()).removeFile();
    Utils::FilePath::fromUrl(item->icon()).removeFile();

    // remove from the bundle json file
    for (int i = 0; i < itemsArr.size(); ++i) {
        if (itemsArr.at(i).toObject().value("qml") == item->qml()) {
            itemsArr.removeAt(i);
            break;
        }
    }
    m_bundleObj3D.insert("items", itemsArr);

    auto result = m_bundlePath3D.pathAppended(Constants::BUNDLE_JSON_FILENAME)
                      .writeFileContents(QJsonDocument(m_bundleObj3D).toJson());
    if (!result)
        qWarning() << __FUNCTION__ << result.error();

    // delete dependency files if they are only used by the deleted item
    QStringList allFiles;
    for (const QJsonValueConstRef &itemRef : std::as_const(itemsArr))
        allFiles.append(itemRef.toObject().value("files").toVariant().toStringList());

    const QStringList itemFiles = item->files();
    for (const QString &file : itemFiles) {
        if (allFiles.count(file) == 0) // only used by the deleted item
            m_bundlePath3D.pathAppended(file).removeFile();
    }

    // remove from model
    m_user3DItems.removeOne(item);
    item->deleteLater();

    // update model
    emit dataChanged(index(Items3DSectionIdx), index(Items3DSectionIdx));
}

/**
 * @brief Gets unique Qml component and icon file material names from a given name
 * @param defaultName input name
 * @return <Qml, icon> file names
 */
QPair<QString, QString> ContentLibraryUserModel::getUniqueLibMaterialNames(const QString &defaultName) const
{
    return getUniqueLibItemNames(defaultName, m_bundleObjMaterial);
}

/**
 * @brief Gets unique Qml component and icon file 3d item names from a given name
 * @param defaultName input name
 * @return <Qml, icon> file names
 */
QPair<QString, QString> ContentLibraryUserModel::getUniqueLib3DNames(const QString &defaultName) const
{
    return getUniqueLibItemNames(defaultName, m_bundleObj3D);
}

QPair<QString, QString> ContentLibraryUserModel::getUniqueLibItemNames(const QString &defaultName,
                                                                       const QJsonObject &bundleObj) const
{
    QString uniqueQml = UniqueName::generateId(defaultName);
    uniqueQml[0] = uniqueQml.at(0).toUpper();
    uniqueQml.prepend("My");

    QString uniqueIcon = defaultName;

    if (!bundleObj.isEmpty()) {
        const QJsonArray itemsArr = bundleObj.value("items").toArray();

        QStringList itemQmls, itemIcons;
        for (const QJsonValueConstRef &itemRef : itemsArr) {
            const QJsonObject &obj = itemRef.toObject();
            itemQmls.append(obj.value("qml").toString().chopped(4)); // remove .qml
            itemIcons.append(QFileInfo(obj.value("icon").toString()).baseName());
        }

        uniqueQml = UniqueName::generate(uniqueQml, [&] (const QString &name) {
            return itemQmls.contains(name);
        });

        uniqueIcon = UniqueName::generate(uniqueIcon, [&] (const QString &name) {
            return itemIcons.contains(name);
        });
    }

    return {uniqueQml + ".qml", uniqueIcon + ".png"};
}

QHash<int, QByteArray> ContentLibraryUserModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        {NameRole, "categoryName"},
        {VisibleRole, "categoryVisible"},
        {ItemsRole, "categoryItems"},
        {NoMatchRole, "categoryNoMatch"}
    };
    return roles;
}

QJsonObject &ContentLibraryUserModel::bundleJsonMaterialObjectRef()
{
    return m_bundleObjMaterial;
}

QJsonObject &ContentLibraryUserModel::bundleJson3DObjectRef()
{
    return m_bundleObj3D;
}

void ContentLibraryUserModel::loadBundles()
{
    loadMaterialBundle();
    load3DBundle();
    loadTextureBundle();
}

void ContentLibraryUserModel::loadMaterialBundle()
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    if (m_matBundleExists && m_bundleIdMaterial == compUtils.userMaterialsBundleId())
        return;

    // clean up
    qDeleteAll(m_userMaterials);
    m_userMaterials.clear();
    m_matBundleExists = false;
    m_noMatchMaterials = true;
    m_bundleObjMaterial = {};
    m_bundleIdMaterial.clear();

    m_bundlePathMaterial = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User/materials");
    m_bundlePathMaterial.ensureWritableDir();
    m_bundlePathMaterial.pathAppended("icons").ensureWritableDir();

    auto jsonFilePath = m_bundlePathMaterial.pathAppended(Constants::BUNDLE_JSON_FILENAME);
    if (!jsonFilePath.exists()) {
        QString jsonContent = "{\n";
        jsonContent += "    \"id\": \"UserMaterials\",\n";
        jsonContent += "    \"items\": []\n";
        jsonContent += "}";
        Utils::expected_str<qint64> res = jsonFilePath.writeFileContents(jsonContent.toLatin1());
        if (!res.has_value()) {
            qWarning() << __FUNCTION__ << res.error();
            emit dataChanged(index(MaterialsSectionIdx), index(MaterialsSectionIdx));
            return;
        }
    }

    Utils::expected_str<QByteArray> jsonContents = jsonFilePath.fileContents();
    if (!jsonContents.has_value()) {
        qWarning() << __FUNCTION__ << jsonContents.error();
        emit dataChanged(index(MaterialsSectionIdx), index(MaterialsSectionIdx));
        return;
    }

    QJsonDocument bundleJsonDoc = QJsonDocument::fromJson(jsonContents.value());
    if (bundleJsonDoc.isNull()) {
        qWarning() << __FUNCTION__ << "Invalid json file" << jsonFilePath;
        emit dataChanged(index(MaterialsSectionIdx), index(MaterialsSectionIdx));
        return;
    }

    m_bundleIdMaterial = compUtils.userMaterialsBundleId();
    m_bundleObjMaterial = bundleJsonDoc.object();
    m_bundleObjMaterial["id"] = m_bundleIdMaterial;

    // parse items
    QString typePrefix = compUtils.userMaterialsBundleType();
    const QJsonArray itemsArr = m_bundleObjMaterial.value("items").toArray();
    for (const QJsonValueConstRef &itemRef : itemsArr) {
        const QJsonObject itemObj = itemRef.toObject();

        QString name = itemObj.value("name").toString();
        QString qml = itemObj.value("qml").toString();
        TypeName type = QLatin1String("%1.%2").arg(typePrefix, qml.chopped(4)).toLatin1();
        QUrl icon = m_bundlePathMaterial.pathAppended(itemObj.value("icon").toString()).toUrl();
        QStringList files = itemObj.value("files").toVariant().toStringList();

        m_userMaterials.append(new ContentLibraryItem(this, name, qml, type, icon, files, "material"));
    }

    m_bundleMaterialSharedFiles.clear();
    const QJsonArray sharedFilesArr = m_bundleObjMaterial.value("sharedFiles").toArray();
    for (const QJsonValueConstRef &file : sharedFilesArr)
        m_bundleMaterialSharedFiles.append(file.toString());

    m_matBundleExists = true;
    updateNoMatchMaterials();
    emit dataChanged(index(MaterialsSectionIdx), index(MaterialsSectionIdx));
}

void ContentLibraryUserModel::load3DBundle()
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();

    if (m_bundle3DExists && m_bundleId3D == compUtils.user3DBundleId())
        return;

    // clean up
    qDeleteAll(m_user3DItems);
    m_user3DItems.clear();
    m_bundle3DExists = false;
    m_noMatch3D = true;
    m_bundleObj3D = {};
    m_bundleId3D.clear();

    m_bundlePath3D = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User/3d");
    m_bundlePath3D.ensureWritableDir();
    m_bundlePath3D.pathAppended("icons").ensureWritableDir();

    auto jsonFilePath = m_bundlePath3D.pathAppended(Constants::BUNDLE_JSON_FILENAME);
    if (!jsonFilePath.exists()) {
        QByteArray jsonContent = "{\n";
        jsonContent += "    \"id\": \"User3D\",\n";
        jsonContent += "    \"items\": []\n";
        jsonContent += "}";
        Utils::expected_str<qint64> res = jsonFilePath.writeFileContents(jsonContent);
        if (!res.has_value()) {
            qWarning() << __FUNCTION__ << res.error();
            emit dataChanged(index(Items3DSectionIdx), index(Items3DSectionIdx));
            return;
        }
    }

    Utils::expected_str<QByteArray> jsonContents = jsonFilePath.fileContents();
    if (!jsonContents.has_value()) {
        qWarning() << __FUNCTION__ << jsonContents.error();
        emit dataChanged(index(Items3DSectionIdx), index(Items3DSectionIdx));
        return;
    }

    QJsonDocument bundleJsonDoc = QJsonDocument::fromJson(jsonContents.value());
    if (bundleJsonDoc.isNull()) {
        qWarning() << __FUNCTION__ << "Invalid json file" << jsonFilePath;
        emit dataChanged(index(Items3DSectionIdx), index(Items3DSectionIdx));
        return;
    }

    m_bundleId3D = compUtils.user3DBundleId();
    m_bundleObj3D = bundleJsonDoc.object();
    m_bundleObj3D["id"] = m_bundleId3D;

    // parse items
    QString typePrefix = compUtils.user3DBundleType();
    const QJsonArray itemsArr = m_bundleObj3D.value("items").toArray();
    for (const QJsonValueConstRef &itemRef : itemsArr) {
        const QJsonObject itemObj = itemRef.toObject();

        QString name = itemObj.value("name").toString();
        QString qml = itemObj.value("qml").toString();
        TypeName type = QLatin1String("%1.%2").arg(typePrefix, qml.chopped(4)).toLatin1();
        QUrl icon = m_bundlePath3D.pathAppended(itemObj.value("icon").toString()).toUrl();
        QStringList files = itemObj.value("files").toVariant().toStringList();

        m_user3DItems.append(new ContentLibraryItem(nullptr, name, qml, type, icon, files, "3d"));
    }

    m_bundle3DSharedFiles.clear();
    const QJsonArray sharedFilesArr = m_bundleObj3D.value("sharedFiles").toArray();
    for (const QJsonValueConstRef &file : sharedFilesArr)
        m_bundle3DSharedFiles.append(file.toString());

    m_bundle3DExists = true;
    updateNoMatch3D();
    emit dataChanged(index(Items3DSectionIdx), index(Items3DSectionIdx));
}

void ContentLibraryUserModel::loadTextureBundle()
{
    if (!m_userTextures.isEmpty())
        return;

    QDir bundleDir{Paths::bundlesPathSetting() + "/User/textures"};
    bundleDir.mkpath(".");
    bundleDir.mkdir("icons");

    const QFileInfoList fileInfos = bundleDir.entryInfoList(QDir::Files);
    for (const QFileInfo &fileInfo : fileInfos) {
        QString suffix = '.' + fileInfo.suffix();
        auto iconFileInfo = QFileInfo(fileInfo.path().append("/icons/").append(fileInfo.baseName() + ".png"));
        QPair<QSize, qint64> info = ImageUtils::imageInfo(fileInfo.path());
        QString dirPath = fileInfo.path();
        QSize imgDims = info.first;
        qint64 imgFileSize = info.second;

        auto tex = new ContentLibraryTexture(this, iconFileInfo, dirPath, suffix, imgDims, imgFileSize);
        m_userTextures.append(tex);
    }

    updateNoMatchTextures();
    emit dataChanged(index(TexturesSectionIdx), index(TexturesSectionIdx));
}

bool ContentLibraryUserModel::hasRequiredQuick3DImport() const
{
    return m_widget->hasQuick3DImport() && m_quick3dMajorVersion == 6 && m_quick3dMinorVersion >= 3;
}

bool ContentLibraryUserModel::matBundleExists() const
{
    return m_matBundleExists;
}

void ContentLibraryUserModel::setSearchText(const QString &searchText)
{
    QString lowerSearchText = searchText.toLower();

    if (m_searchText == lowerSearchText)
        return;

    m_searchText = lowerSearchText;

    for (ContentLibraryItem *item : std::as_const(m_userMaterials))
        item->filter(m_searchText);

    for (ContentLibraryTexture *item : std::as_const(m_userTextures))
        item->filter(m_searchText);

    for (ContentLibraryItem *item : std::as_const(m_user3DItems))
        item->filter(m_searchText);

    updateNoMatchMaterials();
    updateNoMatchTextures();
    updateNoMatch3D();
    resetModel();
}

void ContentLibraryUserModel::updateMaterialsImportedState(const QStringList &importedItems)
{
    bool changed = false;
    for (ContentLibraryItem *mat : std::as_const(m_userMaterials))
        changed |= mat->setImported(importedItems.contains(mat->qml().chopped(4)));

    if (changed)
        emit dataChanged(index(MaterialsSectionIdx), index(MaterialsSectionIdx));
}

void ContentLibraryUserModel::update3DImportedState(const QStringList &importedItems)
{
    bool changed = false;
    for (ContentLibraryItem *item : std::as_const(m_user3DItems))
        changed |= item->setImported(importedItems.contains(item->qml().chopped(4)));

    if (changed)
        emit dataChanged(index(Items3DSectionIdx), index(Items3DSectionIdx));
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
}

void ContentLibraryUserModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void ContentLibraryUserModel::applyToSelected(ContentLibraryItem *mat, bool add)
{
    emit applyToSelectedTriggered(mat, add);
}

void ContentLibraryUserModel::addToProject(QObject *item)
{
    auto castedItem = qobject_cast<ContentLibraryItem *>(item);
    QString bundleDir;
    TypeName type = castedItem->type();
    QString qmlFile = castedItem->qml();
    QStringList files = castedItem->files();

    if (castedItem->itemType() == "material") {
        bundleDir = m_bundlePathMaterial.toFSPathString();
        files << m_bundleMaterialSharedFiles;
    } else if (castedItem->itemType() == "3d") {
        bundleDir = m_bundlePath3D.toFSPathString();
        files << m_bundle3DSharedFiles;
    } else {
        qWarning() << __FUNCTION__ << "Unsupported Item";
        return;
    }

    QString err = m_widget->importer()->importComponent(bundleDir, type, qmlFile, files);

    if (err.isEmpty())
        m_widget->setImporterRunning(true);
    else
        qWarning() << __FUNCTION__ << err;
}

void ContentLibraryUserModel::removeFromProject(QObject *item)
{
    auto castedItem = qobject_cast<ContentLibraryItem *>(item);
    QString err = m_widget->importer()->unimportComponent(castedItem->type(), castedItem->qml());

    if (err.isEmpty())
        m_widget->setImporterRunning(true);
    else
        qWarning() << __FUNCTION__ << err;
}

} // namespace QmlDesigner
