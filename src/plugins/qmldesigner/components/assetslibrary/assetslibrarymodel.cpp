// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QCheckBox>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QMessageBox>
#include <QSortFilterProxyModel>

#include "asset.h"
#include "assetslibrarymodel.h"

#include <modelnodeoperations.h>
#include <qmldesignerplugin.h>

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

AssetsLibraryModel::AssetsLibraryModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{
    createBackendModel();

    setRecursiveFilteringEnabled(true);
}

void AssetsLibraryModel::createBackendModel()
{
    m_sourceFsModel = new QFileSystemModel(parent());

    m_sourceFsModel->setReadOnly(false);

    setSourceModel(m_sourceFsModel);
    QObject::connect(m_sourceFsModel, &QFileSystemModel::directoryLoaded, this, &AssetsLibraryModel::directoryLoaded);

    QObject::connect(m_sourceFsModel, &QFileSystemModel::directoryLoaded, this,
                     [this]([[maybe_unused]] const QString &dir) {
        syncHaveFiles();
    });

    m_fileWatcher = new Utils::FileSystemWatcher(parent());
    QObject::connect(m_fileWatcher, &Utils::FileSystemWatcher::fileChanged, this,
                     [this] (const QString &path) {
        emit fileChanged(path);
    });
}

void AssetsLibraryModel::destroyBackendModel()
{
    setSourceModel(nullptr);
    m_sourceFsModel->disconnect(this);
    m_sourceFsModel->deleteLater();
    m_sourceFsModel = nullptr;

    m_fileWatcher->disconnect(this);
    m_fileWatcher->deleteLater();
    m_fileWatcher = nullptr;
}

void AssetsLibraryModel::setSearchText(const QString &searchText)
{
    m_searchText = searchText;
    resetModel();
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
    return DocumentManager::currentProjectDirPath().toString().append('/');
}

QString AssetsLibraryModel::contentDirPath() const
{
    return DocumentManager::currentResourcePath().toString().append('/');
}

bool AssetsLibraryModel::requestDeleteFiles(const QStringList &filePaths)
{
    bool askBeforeDelete = QmlDesignerPlugin::settings()
                               .value(DesignerSettingsKey::ASK_BEFORE_DELETING_ASSET)
                               .toBool();

    if (askBeforeDelete)
        return false;

    deleteFiles(filePaths, false);
    return true;
}

void AssetsLibraryModel::deleteFiles(const QStringList &filePaths, bool dontAskAgain)
{
    if (dontAskAgain)
        QmlDesignerPlugin::settings().insert(DesignerSettingsKey::ASK_BEFORE_DELETING_ASSET, false);

    for (const QString &filePath : filePaths) {
        if (QFileInfo::exists(filePath) && !QFile::remove(filePath)) {
            QMessageBox::warning(Core::ICore::dialogParent(),
                                 tr("Failed to Delete File"),
                                 tr("Could not delete \"%1\".").arg(filePath));
        }
    }
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
    QString iterPath = folderPath;
    QDir dir{folderPath};

    while (dir.exists()) {
        iterPath = getUniqueName(iterPath);

        dir.setPath(iterPath);
    }

    return dir.mkpath(iterPath) ? iterPath : "";
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
    if (!ok)
        qWarning() << __FUNCTION__ << " could not remove folder recursively: " << m_sourceFsModel->filePath(idx);

    return ok;
}

bool AssetsLibraryModel::allFilePathsAreTextures(const QStringList &filePaths) const
{
    return Utils::allOf(filePaths, [](const QString &path) {
        return Asset(path).isValidTextureSource();
    });
}

bool AssetsLibraryModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QString path = m_sourceFsModel->filePath(sourceParent);

    QModelIndex sourceIdx = m_sourceFsModel->index(sourceRow, 0, sourceParent);
    QString sourcePath = m_sourceFsModel->filePath(sourceIdx);

    if (QFileInfo(sourcePath).isFile() && !m_fileWatcher->watchesFile(sourcePath))
        m_fileWatcher->addFile(sourcePath, Utils::FileSystemWatcher::WatchModifiedDate);

    if (!m_searchText.isEmpty() && path.startsWith(m_rootPath) && QFileInfo{path}.isDir()) {
        QString sourceName = m_sourceFsModel->fileName(sourceIdx);

        return QFileInfo{sourcePath}.isFile() && sourceName.contains(m_searchText, Qt::CaseInsensitive);
    } else {
        return sourcePath.startsWith(m_rootPath) || m_rootPath.startsWith(sourcePath);
    }
}

bool AssetsLibraryModel::checkHaveFiles(const QModelIndex &parentIdx) const
{
    if (!parentIdx.isValid())
        return false;

    const int rowCount = this->rowCount(parentIdx);
    for (int i = 0; i < rowCount; ++i) {
        auto newIdx = this->index(i, 0, parentIdx);
        if (!isDirectory(newIdx))
            return true;

        if (checkHaveFiles(newIdx))
            return true;
    }

    return false;
}

void AssetsLibraryModel::setHaveFiles(bool value)
{
    if (m_haveFiles != value) {
        m_haveFiles = value;
        emit haveFilesChanged();
    }
}

bool AssetsLibraryModel::checkHaveFiles() const
{
    auto rootIdx = indexForPath(m_rootPath);
    return checkHaveFiles(rootIdx);
}

void AssetsLibraryModel::syncHaveFiles()
{
    setHaveFiles(checkHaveFiles());
}

QString AssetsLibraryModel::getUniqueName(const QString &oldName) {
    static QRegularExpression rgx("\\d+$"); // matches a number at the end of a string

    QString uniqueName = oldName;
    // if the folder name ends with a number, increment it
    QRegularExpressionMatch match = rgx.match(uniqueName);
    if (match.hasMatch()) { // ends with a number
        QString numStr = match.captured(0);
        int num = match.captured(0).toInt();

        // get number of padding zeros, ex: for "005" = 2
        int nPaddingZeros = 0;
        for (; nPaddingZeros < numStr.size() && numStr[nPaddingZeros] == '0'; ++nPaddingZeros);

        ++num;

        // if the incremented number's digits increased, decrease the padding zeros
        if (std::fmod(std::log10(num), 1.0) == 0)
            --nPaddingZeros;

        uniqueName = oldName.mid(0, match.capturedStart())
                   + QString('0').repeated(nPaddingZeros)
                   + QString::number(num);
    } else {
        uniqueName = oldName + '1';
    }

    return uniqueName;
}

void AssetsLibraryModel::setRootPath(const QString &newPath)
{
    beginResetModel();

    destroyBackendModel();
    createBackendModel();

    m_rootPath = newPath;
    m_sourceFsModel->setRootPath(newPath);

    m_sourceFsModel->setNameFilters(Asset::supportedSuffixes().values());
    m_sourceFsModel->setNameFilterDisables(false);

    endResetModel();

    emit rootPathChanged();
}

QString AssetsLibraryModel::rootPath() const
{
    return m_rootPath;
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

void AssetsLibraryModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

QModelIndex AssetsLibraryModel::rootIndex() const
{
    return indexForPath(m_rootPath);
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

} // namespace QmlDesigner
