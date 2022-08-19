// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>

namespace QmlDesigner {

class AssetsLibraryFilesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    AssetsLibraryFilesModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addFile(const QString &filePath);

private:
    enum Roles {FileNameRole = Qt::UserRole + 1,
                FilePathRole,
                FileDirRole};

    QStringList m_files;
    QHash<int, QByteArray> m_roleNames;
};

} // QmlDesigner
