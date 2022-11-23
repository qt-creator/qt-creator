// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QFileInfo>

#include <utils/qtcassert.h>

namespace QmlDesigner {

class AssetsLibraryModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    AssetsLibraryModel(QObject *parent = nullptr);

    void setRootPath(const QString &newPath);
    void setSearchText(const QString &searchText);

    Q_PROPERTY(bool haveFiles READ haveFiles NOTIFY haveFilesChanged);

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
    Q_INVOKABLE void syncHaveFiles();

    Q_INVOKABLE QList<QModelIndex> parentIndices(const QModelIndex &index) const;
    Q_INVOKABLE bool indexIsValid(const QModelIndex &index) const;
    Q_INVOKABLE void deleteFiles(const QStringList &filePaths);
    Q_INVOKABLE bool renameFolder(const QString &folderPath, const QString &newName);
    Q_INVOKABLE bool addNewFolder(const QString &folderPath);
    Q_INVOKABLE bool deleteFolderRecursively(const QModelIndex &folderIndex);

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        int result = QSortFilterProxyModel::columnCount(parent);
        return std::min(result, 1);
    }

    bool haveFiles() const { return m_haveFiles; }

    static const QStringList &supportedImageSuffixes();
    static const QStringList &supportedFragmentShaderSuffixes();
    static const QStringList &supportedShaderSuffixes();
    static const QStringList &supportedFontSuffixes();
    static const QStringList &supportedAudioSuffixes();
    static const QStringList &supportedVideoSuffixes();
    static const QStringList &supportedTexture3DSuffixes();
    static const QStringList &supportedEffectMakerSuffixes();
    static const QSet<QString> &supportedSuffixes();

    static bool isEffectQmlExist(const QString &effectName);

signals:
    void directoryLoaded(const QString &path);
    void rootPathChanged();
    void haveFilesChanged();
    void fileChanged(const QString &path);

private:
    void setHaveFiles(bool value);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles);
    void resetModel();
    void createBackendModel();
    void destroyBackendModel();
    bool checkHaveFiles(const QModelIndex &parentIdx) const;
    bool checkHaveFiles() const;

    QString m_searchText;
    QString m_rootPath;
    QFileSystemModel *m_sourceFsModel = nullptr;
    bool m_haveFiles = false;
};

} // namespace QmlDesigner
