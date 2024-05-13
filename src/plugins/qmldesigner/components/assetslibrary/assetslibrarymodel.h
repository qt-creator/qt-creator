// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QSortFilterProxyModel>

namespace Utils {
class FileSystemWatcher;
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

    Q_PROPERTY(bool hasFiles READ hasFiles NOTIFY hasFilesChanged)

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
    Q_INVOKABLE void syncHasFiles();

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

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        int result = QSortFilterProxyModel::columnCount(parent);
        return std::min(result, 1);
    }

    bool hasFiles() const { return m_hasFiles; }

signals:
    void directoryLoaded(const QString &path);
    void rootPathChanged();
    void hasFilesChanged();
    void fileChanged(const QString &path);
    void effectsDeleted(const QStringList &effectNames);

private:
    void setHasFiles(bool value);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    void resetModel();
    void createBackendModel();
    void destroyBackendModel();
    bool checkHasFiles(const QModelIndex &parentIdx) const;
    bool checkHasFiles() const;

    QString m_searchText;
    QString m_rootPath;
    QFileSystemModel *m_sourceFsModel = nullptr;
    bool m_hasFiles = false;
    Utils::FileSystemWatcher *m_fileWatcher = nullptr;
};

} // namespace QmlDesigner
