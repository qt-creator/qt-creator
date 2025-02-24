// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QSortFilterProxyModel>

namespace Utils {
class FileSystemWatcher;
class FilePath;
}

QT_FORWARD_DECLARE_CLASS(QFileSystemModel)

namespace QmlDesigner {

class AssetsLibraryModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    AssetsLibraryModel(QObject *parent = nullptr);

    void setRootPath(const QString &newPath);
    void setSearchText(const QString &searchText);

    Q_PROPERTY(bool isEmpty READ isEmpty NOTIFY isEmptyChanged)

    Q_INVOKABLE QString rootPath() const;
    Q_INVOKABLE QString filePath(const QModelIndex &index) const;
    Q_INVOKABLE QString fileName(const QModelIndex &index) const;

    Q_INVOKABLE QModelIndex indexForPath(const QString &path) const;
    Q_INVOKABLE QModelIndex rootIndex() const;
    Q_INVOKABLE bool isDirectory(const QString &path) const;
    Q_INVOKABLE bool isDirectory(const QModelIndex &index) const;
    Q_INVOKABLE QModelIndex parentDirIndex(const QString &path) const;
    Q_INVOKABLE QModelIndex parentDirIndex(const QModelIndex &index) const;
    Q_INVOKABLE QString parentDirPath(const QString &path) const;
    Q_INVOKABLE void syncIsEmpty();

    Q_INVOKABLE QList<QModelIndex> parentIndices(const QModelIndex &index) const;
    Q_INVOKABLE bool indexIsValid(const QModelIndex &index) const;
    Q_INVOKABLE bool urlPathExistsInModel(const QUrl &url) const;
    Q_INVOKABLE QString currentProjectDirPath() const;
    Q_INVOKABLE QString contentDirPath() const;
    Q_INVOKABLE bool requestDeleteFiles(const QStringList &filePaths);
    Q_INVOKABLE void deleteFiles(const QStringList &filePaths, bool dontAskAgain);
    Q_INVOKABLE bool renameFolder(const QString &folderPath, const QString &newName);
    Q_INVOKABLE QString addNewFolder(const QString &folderPath);
    Q_INVOKABLE bool deleteFolderRecursively(const QModelIndex &folderIndex);
    Q_INVOKABLE bool allFilePathsAreTextures(const QStringList &filePaths) const;
    Q_INVOKABLE bool allFilePathsAreComposedEffects(const QStringList &filePaths) const;
    Q_INVOKABLE bool isSameOrDescendantPath(const QUrl &source, const QString &target) const;
    Q_INVOKABLE bool folderExpandState(const QString &path) const;
    Q_INVOKABLE void initializeExpandState(const QString &path);
    Q_INVOKABLE void saveExpandState(const QString &path, bool expand);
    Q_INVOKABLE bool isDelegateEmpty(const QString &path) const;

    void updateExpandPath(const Utils::FilePath &oldPath, const Utils::FilePath &newPath);

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        int result = QSortFilterProxyModel::columnCount(parent);
        return std::min(result, 1);
    }

    bool isEmpty() const { return m_isEmpty; }

signals:
    void directoryLoaded(const QString &path);
    void rootPathChanged();
    void isEmptyChanged();
    void fileChanged(const QString &path);
    void generatedAssetsDeleted(const QHash<QString, Utils::FilePath> &assetData);

private:
    void setIsEmpty(bool value);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
    void resetModel();
    void createBackendModel();
    void destroyBackendModel();

    QString m_searchText;
    QString m_rootPath;
    QFileSystemModel *m_sourceFsModel = nullptr;
    bool m_isEmpty = true;
    Utils::FileSystemWatcher *m_fileWatcher = nullptr;
    inline static QHash<QString, bool> s_folderExpandStateHash;
};

} // namespace QmlDesigner
