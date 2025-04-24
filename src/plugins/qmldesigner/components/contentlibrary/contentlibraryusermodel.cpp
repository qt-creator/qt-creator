// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibraryusermodel.h"
#include "useritemcategory.h"
#include "usertexturecategory.h"

#include "contentlibraryitem.h"
#include "contentlibrarytexture.h"
#include "contentlibrarywidget.h"

#include <asset.h>
#include <bundleimporter.h>
#include <designerpaths.h>
#include <imageutils.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <utils/algorithm.h>
#include <utils/filesystemwatcher.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>

namespace QmlDesigner {

ContentLibraryUserModel::ContentLibraryUserModel(ContentLibraryWidget *parent)
    : QAbstractListModel(parent)
    , m_widget(parent)
    , m_fileWatcher(Utils::makeUniqueObjectPtr<Utils::FileSystemWatcher>(parent))
{
    createCategories();

    connect(m_fileWatcher.get(), &Utils::FileSystemWatcher::directoryChanged,
            this, [this] (const auto &dirPath) {
        reloadTextureCategory(dirPath);
    });
}

ContentLibraryUserModel::~ContentLibraryUserModel() = default;

int ContentLibraryUserModel::rowCount(const QModelIndex &) const
{
    return m_userCategories.size();
}

QVariant ContentLibraryUserModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_userCategories.size(), return {});
    QTC_ASSERT(roleNames().contains(role), return {});

    UserCategory *currCat = m_userCategories.at(index.row());

    switch (role) {
    case TitleRole:
        return currCat->title();

    case BundlePathRole:
        return currCat->bundlePath().toVariant();

    case ItemsRole:
        return QVariant::fromValue(currCat->items());

    case NoMatchRole:
        return currCat->noMatch();

    case EmptyRole:
        return currCat->isEmpty();
    }

    return {};
}

void ContentLibraryUserModel::createCategories()
{
    QTC_ASSERT(m_userCategories.isEmpty(), return);

    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    auto userBundlePath = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User");

    auto catMaterial = new UserItemCategory{tr("Materials"),
                                            userBundlePath.pathAppended("materials"),
                                            compUtils.userMaterialsBundleId()};

    auto catTexture = new UserTextureCategory{tr("Textures"), userBundlePath.pathAppended("textures")};

    auto cat3D = new UserItemCategory{tr("3D"), userBundlePath.pathAppended("3d"),
                                      compUtils.user3DBundleId()};

    m_userCategories << catMaterial << catTexture << cat3D;

    loadCustomCategories(userBundlePath);
}

void ContentLibraryUserModel::loadCustomCategories(const Utils::FilePath &userBundlePath)
{
    auto jsonFilePath = userBundlePath.pathAppended(Constants::CUSTOM_BUNDLES_JSON_FILENAME);
    if (!jsonFilePath.exists()) {
        const QString jsonContent = QStringLiteral(R"({ "version": "%1", "items": {}})")
                                        .arg(CUSTOM_BUNDLES_JSON_FILE_VERSION);
        Utils::Result<qint64> res = jsonFilePath.writeFileContents(jsonContent.toLatin1());
        QTC_ASSERT_RESULT(res, return);
    }

    Utils::Result<QByteArray> jsonContents = jsonFilePath.fileContents();
    QTC_ASSERT_RESULT(jsonContents, return);

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonContents.value());
    QTC_ASSERT(!jsonDoc.isNull(), return);

    m_customCatsRootObj = jsonDoc.object();
    m_customCatsObj = m_customCatsRootObj.value("items").toObject();

    for (auto it = m_customCatsObj.constBegin(); it != m_customCatsObj.constEnd(); ++it) {
        auto dirPath = Utils::FilePath::fromString(it.key());
        if (!dirPath.exists())
            continue;

        addBundleDir(dirPath);
    }
}

bool ContentLibraryUserModel::bundleDirExists(const QString &dirPath) const
{
    return m_customCatsObj.contains(dirPath);
}

void ContentLibraryUserModel::addBundleDir(const Utils::FilePath &dirPath)
{
    QTC_ASSERT(!dirPath.isEmpty(), return);

    // TODO: detect if a bundle exists in the dir, determine its type, and create a matching category.
    // For now we consider a custom folder as a texture bundle

    auto newCat = new UserTextureCategory{dirPath.fileName(), dirPath};
    newCat->loadBundle();

    beginInsertRows({}, m_userCategories.size(), m_userCategories.size());
    m_userCategories << newCat;
    endInsertRows();

    m_fileWatcher->addDirectory(dirPath, Utils::FileSystemWatcher::WatchAllChanges);

    // add the folder to custom bundles json file if it is missing
    const QString dirPathStr = dirPath.toFSPathString();
    if (!m_customCatsObj.contains(dirPathStr)) {
        m_customCatsObj.insert(dirPathStr, QJsonObject{});

        m_customCatsRootObj["items"] = m_customCatsObj;

        auto userBundlePath = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User");
        auto jsonFilePath = userBundlePath.pathAppended(Constants::CUSTOM_BUNDLES_JSON_FILENAME);
        auto result = jsonFilePath.writeFileContents(QJsonDocument(m_customCatsRootObj).toJson());
        QTC_CHECK_RESULT(result);
    }
}

void ContentLibraryUserModel::addItem(const QString &bundleId, const QString &name,
                                      const QString &qml, const QUrl &icon, const QStringList &files)
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();

    QString typePrefix = compUtils.userBundleType(bundleId);
    TypeName type = QLatin1String("%1.%2").arg(typePrefix, qml.section('.', 0, 0)).toLatin1();

    SectionIndex sectionIndex = bundleIdToSectionIndex(bundleId);

    UserCategory *cat = m_userCategories[sectionIndex];
    cat->addItem(new ContentLibraryItem(cat, name, qml, type, icon, files, bundleId));
    updateIsEmpty();
}

void ContentLibraryUserModel::refreshSection(const QString &bundleId)
{
    QTC_ASSERT(!bundleId.isEmpty(), return);

    SectionIndex sectionIdx = bundleIdToSectionIndex(bundleId);
    emit dataChanged(index(sectionIdx), index(sectionIdx), {ItemsRole, EmptyRole});
    updateIsEmpty();
}

void ContentLibraryUserModel::addTextures(const Utils::FilePaths &paths, const Utils::FilePath &bundlePath)
{
    int catIdx = bundlePathToIndex(bundlePath);
    UserTextureCategory *texCat = qobject_cast<UserTextureCategory *>(m_userCategories.at(catIdx));
    QTC_ASSERT(texCat, return);

    texCat->addItems(paths);

    emit dataChanged(index(catIdx), index(catIdx), {ItemsRole, EmptyRole});
    updateIsEmpty();
}

void ContentLibraryUserModel::reloadTextureCategory(const Utils::FilePath &dirPath)
{
    int catIdx = bundlePathToIndex(dirPath);
    UserTextureCategory *texCat = qobject_cast<UserTextureCategory *>(m_userCategories.at(catIdx));
    QTC_ASSERT(texCat, return);

    const Utils::FilePaths &paths = dirPath.dirEntries({Asset::supportedImageSuffixes(), QDir::Files});

    texCat->clearItems();
    addTextures(paths, dirPath);
}

void ContentLibraryUserModel::removeTextures(const QStringList &fileNames, const Utils::FilePath &bundlePath)
{
    // note: this method doesn't refresh the model after textures removal

    int catIdx = bundlePathToIndex(bundlePath);
    UserTextureCategory *texCat = qobject_cast<UserTextureCategory *>(m_userCategories.at(catIdx));
    QTC_ASSERT(texCat, return);

    const QObjectList items = texCat->items();
    for (QObject *item : items) {
        ContentLibraryTexture *castedItem = qobject_cast<ContentLibraryTexture *>(item);
        QTC_ASSERT(castedItem, continue);

        if (fileNames.contains(castedItem->fileName()))
            removeTexture(castedItem, false);
    }
}

void ContentLibraryUserModel::removeTexture(ContentLibraryTexture *tex, bool refresh)
{
    // remove resources
    Utils::FilePath::fromString(tex->texturePath()).removeFile();
    Utils::FilePath::fromString(tex->iconPath()).removeFile();

    // remove from model
    UserTextureCategory *itemCat = qobject_cast<UserTextureCategory *>(tex->parent());
    QTC_ASSERT(itemCat, return);
    itemCat->removeItem(tex);

    // update model
    if (refresh) {
        int catIdx = bundlePathToIndex(itemCat->bundlePath());
        emit dataChanged(index(catIdx), index(catIdx));
        updateIsEmpty();
    }

    const QString bundlePathStr = itemCat->bundlePath().toFSPathString();
    if (m_customCatsObj.contains(bundlePathStr)) {
        m_customCatsObj.remove(bundlePathStr);

        m_customCatsRootObj["items"] = m_customCatsObj;

        auto userBundlePath = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User");
        auto jsonFilePath = userBundlePath.pathAppended(Constants::CUSTOM_BUNDLES_JSON_FILENAME);
        auto result = jsonFilePath.writeFileContents(QJsonDocument(m_customCatsRootObj).toJson());
        QTC_CHECK_RESULT(result);
    }
}

void ContentLibraryUserModel::removeFromContentLib(QObject *item)
{
    auto castedItem = qobject_cast<ContentLibraryItem *>(item);
    QTC_ASSERT(castedItem, return);

    removeItem(castedItem);
}

void ContentLibraryUserModel::removeBundleDir(int catIdx)
{
    auto texCat = qobject_cast<UserTextureCategory *>(m_userCategories.at(catIdx));
    QTC_ASSERT(texCat, return);

    QString dirPath = texCat->bundlePath().toFSPathString();

    // remove from json
    QTC_ASSERT(m_customCatsObj.contains(dirPath), return);
    m_customCatsObj.remove(dirPath);
    m_customCatsRootObj["items"] = m_customCatsObj;

    auto userBundlePath = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User");
    auto jsonFilePath = userBundlePath.pathAppended(Constants::CUSTOM_BUNDLES_JSON_FILENAME);
    auto result = jsonFilePath.writeFileContents(QJsonDocument(m_customCatsRootObj).toJson());
    QTC_ASSERT_RESULT(result, return);

    // remove from model
    beginRemoveRows({}, catIdx, catIdx);
    delete texCat;
    m_userCategories.removeAt(catIdx);
    endRemoveRows();
}

void ContentLibraryUserModel::removeItemByName(const QString &qmlFileName, const QString &bundleId)
{
    ContentLibraryItem *itemToRemove = nullptr;
    const QObjectList items = m_userCategories[bundleIdToSectionIndex(bundleId)]->items();

    for (QObject *item : items) {
        ContentLibraryItem *castedItem = qobject_cast<ContentLibraryItem *>(item);
        QTC_ASSERT(castedItem, return);

        if (castedItem->qml() == qmlFileName) {
            itemToRemove = castedItem;
            break;
        }
    }

    if (itemToRemove)
        removeItem(itemToRemove);
}

void ContentLibraryUserModel::removeItem(ContentLibraryItem *item)
{
    UserItemCategory *itemCat = qobject_cast<UserItemCategory *>(item->parent());
    QTC_ASSERT(itemCat, return);

    Utils::FilePath bundlePath = itemCat->bundlePath();
    QJsonObject &bundleObj = itemCat->bundleObjRef();

    QJsonArray itemsArr = bundleObj.value("items").toArray();

    // remove qml and icon files
    bundlePath.pathAppended(item->qml()).removeFile();
    Utils::FilePath::fromUrl(item->icon()).removeFile();

    // remove from the bundle json file
    for (int i = 0; i < itemsArr.size(); ++i) {
        if (itemsArr.at(i).toObject().value("qml") == item->qml()) {
            itemsArr.removeAt(i);
            break;
        }
    }
    bundleObj.insert("items", itemsArr);

    auto result = bundlePath.pathAppended(Constants::BUNDLE_JSON_FILENAME)
                      .writeFileContents(QJsonDocument(bundleObj).toJson());
    if (!result)
        qWarning() << __FUNCTION__ << result.error();

    // delete dependency files if they are only used by the deleted item
    QStringList allFiles;
    for (const QJsonValueConstRef &itemRef : std::as_const(itemsArr))
        allFiles.append(itemRef.toObject().value("files").toVariant().toStringList());

    const QStringList itemFiles = item->files();
    for (const QString &file : itemFiles) {
        if (allFiles.count(file) == 0) // only used by the deleted item
            bundlePath.pathAppended(file).removeFile();
    }

    // remove from model
    itemCat->removeItem(item);

    // update model
    SectionIndex sectionIdx = bundleIdToSectionIndex(item->bundleId());
    emit dataChanged(index(sectionIdx), index(sectionIdx), {ItemsRole, EmptyRole});
    updateIsEmpty();
}

ContentLibraryUserModel::SectionIndex ContentLibraryUserModel::bundleIdToSectionIndex(
    const QString &bundleId) const
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();

    if (bundleId == compUtils.userMaterialsBundleId())
        return MaterialsSectionIdx;

    if (bundleId == compUtils.user3DBundleId())
        return Items3DSectionIdx;

    if (bundleId == compUtils.userEffectsBundleId())
        return EffectsSectionIdx;

    qWarning() << __FUNCTION__ << "Invalid section index for bundleId:" << bundleId;
    return {};
}

int ContentLibraryUserModel::bundlePathToIndex(const QString &bundlePath) const
{
    return bundlePathToIndex(Utils::FilePath::fromString(bundlePath));
}

int ContentLibraryUserModel::bundlePathToIndex(const Utils::FilePath &bundlePath) const
{
    for (int i = 0; i < m_userCategories.size(); ++i) {
        if (m_userCategories.at(i)->bundlePath() == bundlePath)
            return i;
    }

    qWarning() << __FUNCTION__ << "Invalid bundlePath:" << bundlePath;
    return -1;
}

QHash<int, QByteArray> ContentLibraryUserModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        {TitleRole, "categoryTitle"},
        {BundlePathRole, "categoryBundlePath"},
        {EmptyRole, "categoryEmpty"},
        {ItemsRole, "categoryItems"},
        {NoMatchRole, "categoryNoMatch"}
    };
    return roles;
}

QJsonObject &ContentLibraryUserModel::bundleObjectRef(const QString &bundleId)
{
    auto secIdx = bundleIdToSectionIndex(bundleId);
    return qobject_cast<UserItemCategory *>(m_userCategories[secIdx])->bundleObjRef();
}

void ContentLibraryUserModel::loadBundles(bool force)
{
    for (UserCategory *cat : std::as_const(m_userCategories))
        cat->loadBundle(force);

    resetModel();
    updateIsEmpty();
}

bool ContentLibraryUserModel::hasRequiredQuick3DImport() const
{
    return m_widget->hasQuick3DImport() && m_quick3dMajorVersion == 6 && m_quick3dMinorVersion >= 3;
}

void ContentLibraryUserModel::updateIsEmpty() {

    bool newIsEmpty = Utils::allOf(std::as_const(m_userCategories), [](UserCategory *cat) {
        return cat->isEmpty();
    });

    if (m_isEmpty == newIsEmpty)
        return;

    m_isEmpty = newIsEmpty;
    emit isEmptyChanged();
}

void ContentLibraryUserModel::setSearchText(const QString &searchText)
{
    QString lowerSearchText = searchText.toLower();

    if (m_searchText == lowerSearchText)
        return;

    m_searchText = lowerSearchText;

    for (UserCategory *cat : std::as_const(m_userCategories))
        cat->filter(m_searchText);

    resetModel();
}

void ContentLibraryUserModel::updateImportedState(const QStringList &importedItems,
                                                  const QString &bundleId)
{
    SectionIndex secIdx = bundleIdToSectionIndex(bundleId);
    UserItemCategory *cat = qobject_cast<UserItemCategory *>(m_userCategories.at(secIdx));
    const QObjectList items = cat->items();

    bool changed = false;
    for (QObject *item : items) {
        ContentLibraryItem *castedItem = qobject_cast<ContentLibraryItem *>(item);
        changed |= castedItem->setImported(importedItems.contains(castedItem->qml().section('.', 0, 0)));
    }

    if (changed)
        emit dataChanged(index(secIdx), index(secIdx), {ItemsRole});
}

bool ContentLibraryUserModel::jsonPropertyExists(const QString &propName, const QString &propValue,
                                                 const QString &bundleId) const
{
    SectionIndex secIdx = bundleIdToSectionIndex(bundleId);
    UserItemCategory *cat = qobject_cast<UserItemCategory *>(m_userCategories.at(secIdx));
    QTC_ASSERT(cat, return false);

    QJsonObject &bundleObj = cat->bundleObjRef();
    const QJsonArray itemsArr = bundleObj.value("items").toArray();

    for (const QJsonValueConstRef &itemRef : itemsArr) {
        const QJsonObject &obj = itemRef.toObject();
        if (obj.value(propName).toString() == propValue)
            return true;
    }

    return false;
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

void ContentLibraryUserModel::addToProject(ContentLibraryItem *item)
{
    UserItemCategory *itemCat = qobject_cast<UserItemCategory *>(item->parent());
    QTC_ASSERT(itemCat, return);

    QString bundlePath = itemCat->bundlePath().toFSPathString();
    TypeName type = item->type();
    QString qmlFile = item->qml();
    QStringList files = item->files() + itemCat->sharedFiles();

    QString err = m_widget->importer()->importComponent(bundlePath, type, qmlFile, files);

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
