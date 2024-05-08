// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibraryusermodel.h"

#include "contentlibrarybundleimporter.h"
#include "contentlibraryitem.h"
#include "contentlibrarymaterial.h"
#include "contentlibrarymaterialscategory.h"
#include "contentlibrarytexture.h"
#include "contentlibrarywidget.h"

#include <designerpaths.h>
#include <imageutils.h>
#include <qmldesignerplugin.h>

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

    return {};
}

bool ContentLibraryUserModel::isValidIndex(int idx) const
{
    return idx > -1 && idx < rowCount();
}

void ContentLibraryUserModel::updateIsEmptyMaterials()
{
    bool anyMatVisible = Utils::anyOf(m_userMaterials, [&](ContentLibraryMaterial *mat) {
        return mat->visible();
    });

    bool newEmpty = !anyMatVisible || !m_widget->hasMaterialLibrary() || !hasRequiredQuick3DImport();

    if (newEmpty != m_isEmptyMaterials) {
        m_isEmptyMaterials = newEmpty;
        emit isEmptyMaterialsChanged();
    }
}

void ContentLibraryUserModel::updateIsEmpty3D()
{
    bool anyItemVisible = Utils::anyOf(m_user3DItems, [&](ContentLibraryItem *item) {
        return item->visible();
    });

    bool newEmpty = !anyItemVisible || !m_widget->hasMaterialLibrary() || !hasRequiredQuick3DImport();

    if (newEmpty != m_isEmpty3D) {
        m_isEmpty3D = newEmpty;
        emit isEmpty3DChanged();
    }
}

void ContentLibraryUserModel::addMaterial(const QString &name, const QString &qml,
                                          const QUrl &icon, const QStringList &files)
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();

    QString typePrefix = compUtils.userMaterialsBundleType();
    TypeName type = QLatin1String("%1.%2").arg(typePrefix, qml.chopped(4)).toLatin1();

    auto libMat = new ContentLibraryMaterial(this, name, qml, type, icon, files,
                                             Paths::bundlesPathSetting().append("/User/materials"));
    m_userMaterials.append(libMat);

    int matSectionIdx = 0;
    emit dataChanged(index(matSectionIdx), index(matSectionIdx));
}

void ContentLibraryUserModel::add3DItem(const QString &name, const QString &qml,
                                        const QUrl &icon, const QStringList &files)
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();

    QString typePrefix = compUtils.user3DBundleType();
    TypeName type = QLatin1String("%1.%2").arg(typePrefix, qml.chopped(4)).toLatin1();

    m_user3DItems.append(new ContentLibraryItem(this, name, qml, type, icon, files));
}

void ContentLibraryUserModel::refresh3DSection()
{
    int sectionIdx = 2;
    emit dataChanged(index(sectionIdx), index(sectionIdx));
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

    int texSectionIdx = 1;
    emit dataChanged(index(texSectionIdx), index(texSectionIdx));
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
    int texSectionIdx = 1;
    emit dataChanged(index(texSectionIdx), index(texSectionIdx));
}

void ContentLibraryUserModel::removeFromContentLib(ContentLibraryMaterial *mat)
{
    auto bundlePath = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User/materials/");

    QJsonObject matsObj = m_bundleObjMaterial.value("materials").toObject();

    // remove qml and icon files
    Utils::FilePath::fromString(mat->qmlFilePath()).removeFile();
    Utils::FilePath::fromUrl(mat->icon()).removeFile();

    // remove from the bundle json file
    matsObj.remove(mat->name());
    m_bundleObjMaterial.insert("materials", matsObj);
    auto result = bundlePath.pathAppended("user_materials_bundle.json")
                      .writeFileContents(QJsonDocument(m_bundleObjMaterial).toJson());
    if (!result)
        qWarning() << __FUNCTION__ << result.error();

    // delete dependency files if they are only used by the deleted material
    QStringList allFiles;
    for (const QJsonValueConstRef &mat : std::as_const(matsObj))
         allFiles.append(mat.toObject().value("files").toVariant().toStringList());

    const QStringList matFiles = mat->files();
    for (const QString &matFile : matFiles) {
        if (allFiles.count(matFile) == 0) // only used by the deleted material
            bundlePath.pathAppended(matFile).removeFile();
    }

    // remove from model
    m_userMaterials.removeOne(mat);
    mat->deleteLater();

    // update model
    int matSectionIdx = 0;
    emit dataChanged(index(matSectionIdx), index(matSectionIdx));
}

// returns unique library material's name and qml component
QPair<QString, QString> ContentLibraryUserModel::getUniqueLibMaterialNameAndQml(const QString &defaultName) const
{
    QTC_ASSERT(!m_bundleObjMaterial.isEmpty(), return {});

    const QJsonObject matsObj = m_bundleObjMaterial.value("materials").toObject();
    const QStringList matNames = matsObj.keys();

    QStringList matQmls;
    for (const QString &matName : matNames)
        matQmls.append(matsObj.value(matName).toObject().value("qml").toString().chopped(4)); // remove .qml

    QString retName = defaultName.isEmpty() ? "Material" : defaultName.trimmed();
    QString retQml = retName;

    retQml.remove(' ');
    if (retQml.at(0).isLower())
        retQml[0] = retQml.at(0).toUpper();
    retQml.prepend("My");

    int num = 1;
    if (matNames.contains(retName) || matQmls.contains(retQml)) {
        while (matNames.contains(retName + QString::number(num))
             || matQmls.contains(retQml + QString::number(num))) {
            ++num;
        }

        retName += QString::number(num);
        retQml += QString::number(num);
    }

    return {retName, retQml + ".qml"};
}

QString ContentLibraryUserModel::getUniqueLib3DQmlName(const QString &defaultName) const
{
    QTC_ASSERT(!m_bundleObj3D.isEmpty(), return {});

    const QJsonArray itemsArr = m_bundleObj3D.value("items").toArray();

    QStringList itemQmls;
    for (const QJsonValueConstRef &itemRef : itemsArr)
        itemQmls.append(itemRef.toObject().value("qml").toString().chopped(4)); // remove .qml

    QString baseQml = defaultName.isEmpty() ? "Item" : defaultName.trimmed();
    baseQml.remove(' ');
    baseQml[0] = baseQml.at(0).toUpper();
    baseQml.prepend("My");

    QString uniqueQml = baseQml;

    int counter = 1;
    while (itemQmls.contains(uniqueQml)) {
        uniqueQml = QString("%1%2").arg(uniqueQml).arg(counter);
        ++counter;
    }

    return uniqueQml + ".qml";
}

QHash<int, QByteArray> ContentLibraryUserModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        {NameRole, "categoryName"},
        {VisibleRole, "categoryVisible"},
        {ItemsRole, "categoryItems"}
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
    m_isEmptyMaterials = true;
    m_bundleObjMaterial = {};
    m_bundleIdMaterial.clear();

    int matSectionIdx = 0;

    QDir bundleDir{Paths::bundlesPathSetting() + "/User/materials"};
    bundleDir.mkpath(".");

    auto jsonFilePath = Utils::FilePath::fromString(bundleDir.filePath("user_materials_bundle.json"));
    if (!jsonFilePath.exists()) {
        QString jsonContent = "{\n";
        jsonContent += "    \"id\": \"UserMaterials\",\n";
        jsonContent += "    \"materials\": {\n";
        jsonContent += "    }\n";
        jsonContent += "}";
        jsonFilePath.writeFileContents(jsonContent.toLatin1());
    }

    QFile jsonFile(jsonFilePath.path());
    if (!jsonFile.open(QIODevice::ReadOnly)) {
        qWarning() << __FUNCTION__ << "Couldn't open user_materials_bundle.json";
        emit dataChanged(index(matSectionIdx), index(matSectionIdx));
        return;
    }

    QJsonDocument matBundleJsonDoc = QJsonDocument::fromJson(jsonFile.readAll());
    if (matBundleJsonDoc.isNull()) {
        qWarning() << __FUNCTION__ << "Invalid user_materials_bundle.json file";
        emit dataChanged(index(matSectionIdx), index(matSectionIdx));
        return;
    }

    m_bundleObjMaterial = matBundleJsonDoc.object();
    m_bundleObjMaterial["id"] = compUtils.userMaterialsBundleId();
    m_bundleIdMaterial = compUtils.userMaterialsBundleId();

    // parse materials
    const QJsonObject matsObj = m_bundleObjMaterial.value("materials").toObject();
    const QStringList materialNames = matsObj.keys();
    QString typePrefix = compUtils.userMaterialsBundleType();
    for (const QString &matName : materialNames) {
        const QJsonObject matObj = matsObj.value(matName).toObject();

        QStringList files;
        const QJsonArray assetsArr = matObj.value("files").toArray();
        for (const QJsonValueConstRef &asset : assetsArr)
            files.append(asset.toString());

        QUrl icon = QUrl::fromLocalFile(bundleDir.filePath(matObj.value("icon").toString()));
        QString qml = matObj.value("qml").toString();
        TypeName type = QLatin1String("%1.%2").arg(typePrefix, qml.chopped(4)).toLatin1();

        auto userMat = new ContentLibraryMaterial(this, matName, qml, type, icon, files,
                                                  bundleDir.path(), "");

        m_userMaterials.append(userMat);
    }

    m_bundleMaterialSharedFiles.clear();
    const QJsonArray sharedFilesArr = m_bundleObjMaterial.value("sharedFiles").toArray();
    for (const QJsonValueConstRef &file : sharedFilesArr)
        m_bundleMaterialSharedFiles.append(file.toString());

    m_matBundleExists = true;
    emit matBundleExistsChanged();
    emit dataChanged(index(matSectionIdx), index(matSectionIdx));

    m_matBundleExists = true;
    updateIsEmptyMaterials();
    resetModel();
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
    m_isEmpty3D = true;
    m_bundleObj3D = {};
    m_bundleId3D.clear();

    int section3DIdx = 2;

    m_bundlePath3D = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User/3d");
    m_bundlePath3D.ensureWritableDir();
    m_bundlePath3D.pathAppended("icons").ensureWritableDir();

    auto jsonFilePath = m_bundlePath3D.pathAppended("user_3d_bundle.json");

    if (!jsonFilePath.exists()) {
        QByteArray jsonContent = "{\n";
        jsonContent += "    \"id\": \"User3D\",\n";
        jsonContent += "    \"items\": []\n";
        jsonContent += "}";
        Utils::expected_str<qint64> res = jsonFilePath.writeFileContents(jsonContent);
        if (!res.has_value()) {
            qWarning() << __FUNCTION__ << res.error();
            emit dataChanged(index(section3DIdx), index(section3DIdx));
            return;
        }
    }

    Utils::expected_str<QByteArray> jsonContents = jsonFilePath.fileContents();
    if (!jsonContents.has_value()) {
        qWarning() << __FUNCTION__ << jsonContents.error();
        emit dataChanged(index(section3DIdx), index(section3DIdx));
        return;
    }

    QJsonDocument bundleJsonDoc = QJsonDocument::fromJson(jsonContents.value());
    if (bundleJsonDoc.isNull()) {
        qWarning() << __FUNCTION__ << "Invalid user_3d_bundle.json file";
        emit dataChanged(index(section3DIdx), index(section3DIdx));
        return;
    }

    m_bundleId3D = compUtils.user3DBundleId();
    m_bundleObj3D = bundleJsonDoc.object();
    m_bundleObj3D["id"] = m_bundleId3D;

    // parse 3d items
    QString typePrefix = compUtils.user3DBundleType();
    const QJsonArray itemsArr = m_bundleObj3D.value("items").toArray();
    for (const QJsonValueConstRef &itemRef : itemsArr) {
        const QJsonObject itemObj = itemRef.toObject();

        QString name = itemObj.value("name").toString();
        QString qml = itemObj.value("qml").toString();
        TypeName type = QLatin1String("%1.%2").arg(typePrefix, qml.chopped(4)).toLatin1();
        QUrl icon = m_bundlePath3D.pathAppended(itemObj.value("icon").toString()).toUrl();
        QStringList files;
        const QJsonArray assetsArr = itemObj.value("files").toArray();
        for (const QJsonValueConstRef &asset : assetsArr)
            files.append(asset.toString());

        m_user3DItems.append(new ContentLibraryItem(nullptr, name, qml, type, icon, files));
    }

    m_bundle3DSharedFiles.clear();
    const QJsonArray sharedFilesArr = m_bundleObj3D.value("sharedFiles").toArray();
    for (const QJsonValueConstRef &file : sharedFilesArr)
        m_bundle3DSharedFiles.append(file.toString());

    m_bundle3DExists = true;
    updateIsEmpty3D();
    emit dataChanged(index(section3DIdx), index(section3DIdx));
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

    int texSectionIdx = 1;
    emit dataChanged(index(texSectionIdx), index(texSectionIdx));
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

    for (ContentLibraryMaterial *mat : std::as_const(m_userMaterials))
        mat->filter(m_searchText);

    updateIsEmptyMaterials();
    updateIsEmpty3D();
}

void ContentLibraryUserModel::updateImportedState(const QStringList &importedItems)
{
    bool changed = false;
    for (ContentLibraryMaterial *mat : std::as_const(m_userMaterials))
        changed |= mat->setImported(importedItems.contains(mat->qml().chopped(4)));

    if (changed) {
        int matSectionIdx = 0;
        emit dataChanged(index(matSectionIdx), index(matSectionIdx));
    }
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

    updateIsEmptyMaterials();
    updateIsEmpty3D();
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
    QString err = m_widget->importer()->importComponent(mat->dirPath(), mat->type(), mat->qml(),
                                                        mat->files() + m_bundleMaterialSharedFiles);

    if (err.isEmpty())
        m_widget->setImporterRunning(true);
    else
        qWarning() << __FUNCTION__ << err;
}

void ContentLibraryUserModel::removeFromProject(ContentLibraryMaterial *mat)
{
    QString err = m_widget->importer()->unimportComponent(mat->type(), mat->qml());

    if (err.isEmpty())
        m_widget->setImporterRunning(true);
    else
        qWarning() << __FUNCTION__ << err;
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
