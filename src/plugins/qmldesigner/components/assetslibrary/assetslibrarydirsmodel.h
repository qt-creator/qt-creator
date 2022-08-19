// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include "assetslibrarydir.h"

namespace QmlDesigner {

class AssetsLibraryDirsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    AssetsLibraryDirsModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addDir(AssetsLibraryDir *assetsDir);

    const QList<AssetsLibraryDir *> assetsDirs() const;

private:
    QList<AssetsLibraryDir *> m_dirs;
    QHash<int, QByteArray> m_roleNames;
};

} // namespace QmlDesigner
