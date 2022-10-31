// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include <QCheckBox>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QImageReader>
#include <QMessageBox>
#include <QSortFilterProxyModel>

#include "assetslibrarymodel.h"

#include <qmldesignerplugin.h>
#include <modelnodeoperations.h>

#include <coreplugin/icore.h>
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
    QObject::connect(m_sourceFsModel, &QFileSystemModel::dataChanged, this, &AssetsLibraryModel::onDataChanged);

    QObject::connect(m_sourceFsModel, &QFileSystemModel::directoryLoaded, this,
                     [this]([[maybe_unused]] const QString &dir) {
                         syncHaveFiles();
                     });
}

bool AssetsLibraryModel::isEffectQmlExist(const QString &effectName)
{
    Utils::FilePath effectsResDir = ModelNodeOperations::getEffectsDirectory();
    Utils::FilePath qmlPath = effectsResDir.resolvePath(effectName + "/" + effectName + ".qml");
    return qmlPath.exists();
}

void AssetsLibraryModel::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                                       [[maybe_unused]] const QList<int> &roles)
{
    for (int i = topLeft.row(); i <= bottomRight.row(); ++i) {
        QModelIndex index = m_sourceFsModel->index(i, 0, topLeft.parent());
        QString path = m_sourceFsModel->filePath(index);

        if (!isDirectory(path))
            emit fileChanged(path);
    }
}

void AssetsLibraryModel::destroyBackendModel()
{
    setSourceModel(nullptr);
    m_sourceFsModel->disconnect(this);
    m_sourceFsModel->deleteLater();
    m_sourceFsModel = nullptr;
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
        if (QFile::exists(filePath) && !QFile::remove(filePath)) {
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

bool AssetsLibraryModel::addNewFolder(const QString &folderPath)
{
    QString iterPath = folderPath;
    static QRegularExpression rgx("\\d+$"); // matches a number at the end of a string
    QDir dir{folderPath};

    while (dir.exists()) {
        // if the folder name ends with a number, increment it
        QRegularExpressionMatch match = rgx.match(iterPath);
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

            iterPath = folderPath.mid(0, match.capturedStart())
                       + QString('0').repeated(nPaddingZeros)
                       + QString::number(num);
        } else {
            iterPath = folderPath + '1';
        }

        dir.setPath(iterPath);
    }

    return dir.mkpath(iterPath);
}

bool AssetsLibraryModel::deleteFolderRecursively(const QModelIndex &folderIndex)
{
    auto idx = mapToSource(folderIndex);
    bool ok = m_sourceFsModel->remove(idx);
    if (!ok)
        qWarning() << __FUNCTION__ << " could not remove folder recursively: " << m_sourceFsModel->filePath(idx);

    return ok;
}

bool AssetsLibraryModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QString path = m_sourceFsModel->filePath(sourceParent);

    QModelIndex sourceIdx = m_sourceFsModel->index(sourceRow, 0, sourceParent);
    QString sourcePath = m_sourceFsModel->filePath(sourceIdx);

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

void AssetsLibraryModel::setRootPath(const QString &newPath)
{
    beginResetModel();

    destroyBackendModel();
    createBackendModel();

    m_rootPath = newPath;
    m_sourceFsModel->setRootPath(newPath);

    m_sourceFsModel->setNameFilters(supportedSuffixes().values());
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

const QStringList &AssetsLibraryModel::supportedImageSuffixes()
{
    static QStringList retList;
    if (retList.isEmpty()) {
        const QList<QByteArray> suffixes = QImageReader::supportedImageFormats();
        for (const QByteArray &suffix : suffixes)
            retList.append("*." + QString::fromUtf8(suffix));
    }
    return retList;
}

const QStringList &AssetsLibraryModel::supportedFragmentShaderSuffixes()
{
    static const QStringList retList {"*.frag", "*.glsl", "*.glslf", "*.fsh"};
    return retList;
}

const QStringList &AssetsLibraryModel::supportedShaderSuffixes()
{
    static const QStringList retList {"*.frag", "*.vert",
                                      "*.glsl", "*.glslv", "*.glslf",
                                      "*.vsh", "*.fsh"};
    return retList;
}

const QStringList &AssetsLibraryModel::supportedFontSuffixes()
{
    static const QStringList retList {"*.ttf", "*.otf"};
    return retList;
}

const QStringList &AssetsLibraryModel::supportedAudioSuffixes()
{
    static const QStringList retList {"*.wav", "*.mp3"};
    return retList;
}

const QStringList &AssetsLibraryModel::supportedVideoSuffixes()
{
    static const QStringList retList {"*.mp4"};
    return retList;
}

const QStringList &AssetsLibraryModel::supportedTexture3DSuffixes()
{
    // These are file types only supported by 3D textures
    static QStringList retList {"*.hdr", "*.ktx"};
    return retList;
}

const QStringList &AssetsLibraryModel::supportedEffectMakerSuffixes()
{
    // These are file types only supported by Effect Maker
    static QStringList retList {"*.qep"};
    return retList;
}

const QSet<QString> &AssetsLibraryModel::supportedSuffixes()
{
    static QSet<QString> allSuffixes;
    if (allSuffixes.isEmpty()) {
        auto insertSuffixes = [](const QStringList &suffixes) {
            for (const auto &suffix : suffixes)
                allSuffixes.insert(suffix);
        };
        insertSuffixes(supportedImageSuffixes());
        insertSuffixes(supportedShaderSuffixes());
        insertSuffixes(supportedFontSuffixes());
        insertSuffixes(supportedAudioSuffixes());
        insertSuffixes(supportedVideoSuffixes());
        insertSuffixes(supportedTexture3DSuffixes());
        insertSuffixes(supportedEffectMakerSuffixes());
    }
    return allSuffixes;
}

} // namespace QmlDesigner
