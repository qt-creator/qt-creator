// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "assetslibrarymodel.h"

#include <modelnodeoperations.h>
#include <qmldesignerplugin.h>
#include <qmldesignertr.h>
#include <uniquename.h>

#include <qmldesigner/settings/designersettings.h>

#include <coreplugin/icore.h>

#include <qmldesignerutils/asset.h>
#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/filesystemwatcher.h>

#include <QFileInfo>
#include <QFileSystemModel>
#include <QMessageBox>

using namespace Utils;

namespace QmlDesigner {

AssetsLibraryModel::AssetsLibraryModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{
    setRootPath({}); // Initialize backend model
    setRecursiveFilteringEnabled(true);
    sort(0);
}

void AssetsLibraryModel::setSearchText(const QString &searchText)
{
    beginResetModel();
    m_searchText = searchText;
    endResetModel();
}

bool AssetsLibraryModel::indexIsValid(const QModelIndex &index) const
{
    static QModelIndex invalidIndex;
    return index != invalidIndex;
}

QList<QModelIndex> AssetsLibraryModel::parentIndices(const QModelIndex &index) const
{
    QModelIndex idx = index;
    QModelIndex rootIdx = rootIndex();
    QList<QModelIndex> result;

    while (idx.isValid() && idx != rootIdx) {
        result += idx;
        idx = idx.parent();
    }

    return result;
}

QString AssetsLibraryModel::currentProjectDirPath() const
{
    return DocumentManager::currentProjectDirPath().toUrlishString().append('/');
}

QString AssetsLibraryModel::contentDirPath() const
{
    return DocumentManager::currentResourcePath().toUrlishString().append('/');
}

bool AssetsLibraryModel::requestDeleteFiles(const QStringList &filePaths)
{
    bool askBeforeDelete = designerSettings().askBeforeDeletingAsset();

    if (askBeforeDelete)
        return false;

    deleteFiles(filePaths, false);
    return true;
}

void AssetsLibraryModel::deleteFiles(const QStringList &filePaths, bool dontAskAgain)
{
    if (dontAskAgain)
        designerSettings().askBeforeDeletingAsset.setValue(false);

    QHash<QString, Utils::FilePath> deletedAssets;
    const GeneratedComponentUtils &compUtils = QmlDesignerPlugin::instance()->documentManager()
                                             .generatedComponentUtils();
    const QString effectTypePrefix = compUtils.composedEffectsTypePrefix();
    const Utils::FilePath effectBasePath = compUtils.composedEffectsBasePath();

    for (const QString &filePath : filePaths) {
        Utils::FilePath fp = Utils::FilePath::fromString(filePath);
        if (fp.exists()) {
            // If a generated asset was removed, also remove its module from project
            Asset asset(filePath);
            QString fullType;
            if (asset.isEffect()) {
                QString effectName = fp.baseName();
                fullType = QString("%1.%2.%2").arg(effectTypePrefix, effectName, effectName);
                deletedAssets.insert(fullType, effectBasePath.resolvePath(effectName));
            } else if (asset.isImported3D()) {
                Utils::FilePath qmlFile = compUtils.getImported3dQml(filePath);
                if (qmlFile.exists()) {
                    QString importName = compUtils.getImported3dImportName(qmlFile);
                    fullType = QString("%1.%2").arg(importName, qmlFile.baseName());
                    deletedAssets.insert(fullType, qmlFile.absolutePath());
                }
            }

            if (!fp.removeFile()) {
                deletedAssets.remove(fullType);
                QMessageBox::warning(Core::ICore::dialogParent(),
                                     Tr::tr("Failed to Delete File"),
                                     Tr::tr("Could not delete \"%1\".").arg(filePath));
            }
        }
    }

    if (!deletedAssets.isEmpty())
        emit generatedAssetsDeleted(deletedAssets);
}

bool AssetsLibraryModel::renameFolder(const QString &folderPath, const QString &newName)
{
    QDir dir{folderPath};
    QString oldName = dir.dirName();

    if (oldName == newName)
        return true;

    dir.cdUp();

    return dir.rename(oldName, newName);
}

QString AssetsLibraryModel::addNewFolder(const QString &folderPath)
{
    Utils::FilePath uniqueDirPath = Utils::FilePath::fromString(UniqueName::generatePath(folderPath));

    const Utils::Result<> res = uniqueDirPath.ensureWritableDir();
    if (!res) {
        qWarning() << __FUNCTION__ << res.error();
        return {};
    }

    return uniqueDirPath.path();
}

bool AssetsLibraryModel::urlPathExistsInModel(const QUrl &url) const
{
    QModelIndex index = indexForPath(url.toLocalFile());
    return index.isValid();
}

bool AssetsLibraryModel::deleteFolderRecursively(const QModelIndex &folderIndex)
{
    auto idx = mapToSource(folderIndex);
    bool ok = m_sourceFsModel->remove(idx);

    if (ok) {
        Utils::FilePath parentPath = Utils::FilePath::fromString(filePath(folderIndex));
        const QStringList paths = s_folderExpandStateHash.keys();

        for (const QString &path : paths) {
            if (Utils::FilePath::fromString(path).isChildOf(parentPath))
                s_folderExpandStateHash.remove(path);
        }
    } else {
        qWarning() << __FUNCTION__ << " could not remove folder recursively: " << m_sourceFsModel->filePath(idx);
    }

    return ok;
}

bool AssetsLibraryModel::allFilePathsAreTextures(const QStringList &filePaths) const
{
    return Utils::allOf(filePaths, [](const QString &path) {
        return Asset(path).isValidTextureSource();
    });
}

bool AssetsLibraryModel::allFilePathsAreComposedEffects(const QStringList &filePaths) const
{
    return Utils::allOf(filePaths, [](const QString &path) {
        return Asset(path).isEffect();
    });
}

bool AssetsLibraryModel::isSameOrDescendantPath(const QUrl &source, const QString &target) const
{
    Utils::FilePath srcPath = Utils::FilePath::fromUrl(source);
    Utils::FilePath targetPath = Utils::FilePath::fromString(target);

    return srcPath == targetPath || targetPath.isChildOf(srcPath);
}

bool AssetsLibraryModel::folderExpandState(const QString &path) const
{
    return s_folderExpandStateHash.value(path);
}

void AssetsLibraryModel::initializeExpandState(const QString &path)
{
    if (!s_folderExpandStateHash.contains(path))
        saveExpandState(path, true);
}

void AssetsLibraryModel::saveExpandState(const QString &path, bool expand)
{
    s_folderExpandStateHash.insert(path, expand);
}

bool AssetsLibraryModel::isDelegateEmpty(const QString &path) const
{
    QModelIndex proxyIndex = indexForPath(path);

    if (!isDirectory(proxyIndex))
        return true;

    QModelIndex sourceIndex = mapToSource(proxyIndex);

    // Populates the folder's contents if it hasn't already (e.g., collapsed folder)
    sourceModel()->fetchMore(sourceIndex);

    return sourceModel()->rowCount(sourceIndex) == 0;
}

void AssetsLibraryModel::updateExpandPath(const Utils::FilePath &oldPath, const Utils::FilePath &newPath)
{
    // update parent folder expand state
    bool value = s_folderExpandStateHash.take(oldPath.toFSPathString());
    saveExpandState(newPath.toFSPathString(), value);

    const QStringList paths = s_folderExpandStateHash.keys();

    for (const QString &path : paths) {
        Utils::FilePath childPath = Utils::FilePath::fromString(path);

        // update subfolders expand states
        if (childPath.isChildOf(oldPath)) {
            QString relativePath = childPath.relativePathFromDir(oldPath);
            Utils::FilePath newChildPath = newPath.resolvePath(relativePath);

            value = s_folderExpandStateHash.take(path);
            saveExpandState(newChildPath.toFSPathString(), value);
        }
    }
}

bool AssetsLibraryModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QString path = m_sourceFsModel->filePath(sourceParent);

    QModelIndex sourceIdx = m_sourceFsModel->index(sourceRow, 0, sourceParent);
    QString sourcePath = m_sourceFsModel->filePath(sourceIdx);

    if (QFileInfo(sourcePath).isFile() && !m_fileWatcher->watchesFile(FilePath::fromString(sourcePath)))
        m_fileWatcher->addFile(FilePath::fromString(sourcePath), FileSystemWatcher::WatchModifiedDate);

    const QString rootPath = this->rootPath();
    if (!m_searchText.isEmpty() && path.startsWith(rootPath) && QFileInfo{path}.isDir()) {
        QString sourceName = m_sourceFsModel->fileName(sourceIdx);

        return QFileInfo{sourcePath}.isFile() && sourceName.contains(m_searchText, Qt::CaseInsensitive);
    } else {
        return sourcePath.startsWith(rootPath) || rootPath.startsWith(sourcePath);
    }
}

void AssetsLibraryModel::setIsEmpty(bool value)
{
    if (m_isEmpty != value) {
        m_isEmpty = value;
        emit isEmptyChanged();
    }
}

void AssetsLibraryModel::syncIsEmpty()
{
    QModelIndex rootIdx = indexForPath(rootPath());

    bool hasContent = rowCount(rootIdx);
    setIsEmpty(!hasContent);
}

static void disconnectAndDeleteObjects(QObject *from, const QList<QObject *> &objects)
{
    for (QObject *object : objects) {
        if (object) {
            object->disconnect(from);
            object->deleteLater();
        }
    }
}

void AssetsLibraryModel::setRootPath(const QString &newPath)
{
    disconnectAndDeleteObjects(this, {m_sourceFsModel, m_fileWatcher});

    m_sourceFsModel = createFsModel(newPath);
    m_fileWatcher = createFsWatcher();
    setSourceModel(m_sourceFsModel); // calls reset

    emit rootPathChanged();
}

QString AssetsLibraryModel::rootPath() const
{
    return m_sourceFsModel->rootPath();
}

QString AssetsLibraryModel::filePath(const QModelIndex &index) const
{
    QModelIndex fsIdx = mapToSource(index);
    return m_sourceFsModel->filePath(fsIdx);
}

QString AssetsLibraryModel::fileName(const QModelIndex &index) const
{
    QModelIndex fsIdx = mapToSource(index);
    return m_sourceFsModel->fileName(fsIdx);
}

QModelIndex AssetsLibraryModel::indexForPath(const QString &path) const
{
    QModelIndex idx = m_sourceFsModel->index(path, 0);
    return mapFromSource(idx);
}

QFileSystemModel *AssetsLibraryModel::createFsModel(const QString &path)
{
    QFileSystemModel *fsModel = new QFileSystemModel(parent());

    fsModel->setReadOnly(false);

    connect(fsModel, &QFileSystemModel::directoryLoaded, this, [this](const QString &dir) {
        emit directoryLoaded(dir);
        syncIsEmpty();
    });

    if (!path.isEmpty()) {
        fsModel->setRootPath(path);
        fsModel->setNameFilters(Asset::supportedSuffixes().values());
        fsModel->setNameFilterDisables(false);
    }

    return fsModel;
}

Utils::FileSystemWatcher *AssetsLibraryModel::createFsWatcher()
{
    Utils::FileSystemWatcher *fsWatcher = new Utils::FileSystemWatcher(parent());
    connect(fsWatcher, &Utils::FileSystemWatcher::fileChanged, this, [&](const Utils::FilePath &path) {
        emit fileChanged(path.toFSPathString());
    });
    return fsWatcher;
}

QModelIndex AssetsLibraryModel::rootIndex() const
{
    return indexForPath(rootPath());
}

bool AssetsLibraryModel::isDirectory(const QString &path) const
{
    QFileInfo fi{path};
    return fi.isDir();
}

bool AssetsLibraryModel::isDirectory(const QModelIndex &index) const
{
    QString path = filePath(index);
    return isDirectory(path);
}

QModelIndex AssetsLibraryModel::parentDirIndex(const QString &path) const
{
    QModelIndex idx = indexForPath(path);
    QModelIndex parentIdx = idx.parent();

    return parentIdx;
}

QModelIndex AssetsLibraryModel::parentDirIndex(const QModelIndex &index) const
{
    QModelIndex parentIdx = index.parent();
    return parentIdx;
}

QString AssetsLibraryModel::parentDirPath(const QString &path) const
{
    QModelIndex idx = indexForPath(path);
    QModelIndex parentIdx = idx.parent();
    return filePath(parentIdx);
}

bool AssetsLibraryModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    bool leftIsDir = m_sourceFsModel->isDir(left);
    bool rightIsDir = m_sourceFsModel->isDir(right);

    if (leftIsDir && !rightIsDir)
        return true;

    if (!leftIsDir && rightIsDir)
        return false;

    const QString leftName = m_sourceFsModel->fileName(left);
    const QString rightName = m_sourceFsModel->fileName(right);

    return QString::localeAwareCompare(leftName, rightName) < 0;
}

} // namespace QmlDesigner
