// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"
#include "filepath.h"

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

QT_BEGIN_NAMESPACE
class QMimeData;
QT_END_NAMESPACE

#include <memory>

namespace Utils {

class FileSystemModelPrivate;

class QTCREATOR_UTILS_EXPORT FileSystemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Column {
        NameColumn = 0,
        SizeColumn,
        TypeColumn,
        DateModifiedColumn,
        NumColumns,
    };

    enum Role {
        FilePathRole = Qt::UserRole,
        FileNameRole,
        FileFlagsRole,
        FileSortRole,
    };

    explicit FileSystemModel(QObject *parent = nullptr);
    ~FileSystemModel() override;

    FilePath rootPath() const;
    void setRootPath(const FilePath &rootPath);

    FilePath filePath(const QModelIndex &index) const;
    QModelIndex index(const FilePath &path) const;
    bool isDir(const QModelIndex &index) const;

    QModelIndex mkdir(const QModelIndex &parent, const QString &name);

    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    bool hasChildren(const QModelIndex &parent = {}) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;

signals:
    void rootPathChanged(const Utils::FilePath &newPath);
    void directoryLoaded(const Utils::FilePath &path);
    void directoryLoadFailed(const Utils::FilePath &path, const QString &error);
    // Brackets the asynchronous listing of a directory. The end notification
    // is not delivered for fetches a setRootPath() discards while in flight.
    void fetchingChanged(const Utils::FilePath &path, bool fetching);

private:
    friend class FileSystemModelPrivate;
    std::unique_ptr<FileSystemModelPrivate> d;
};

class QTCREATOR_UTILS_EXPORT FileSystemProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit FileSystemProxyModel(QObject *parent = nullptr);

    void setShowHiddenFiles(bool show);
    bool showHiddenFiles() const { return m_showHiddenFiles; }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override;

private:
    bool m_showHiddenFiles = false;
};

} // namespace Utils
