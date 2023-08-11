// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStandardItemModel>

#include "effectnodescategory.h"

namespace Utils {
class FilePath;
}

namespace QmlDesigner {

class EffectMakerNodesModel : public QAbstractListModel
{
    Q_OBJECT

    enum Roles {
        CategoryNameRole = Qt::UserRole + 1,
        CategoryNodesRole
    };

public:
    EffectMakerNodesModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void loadModel();
    void resetModel();

    QList<EffectNodesCategory *> categories() const { return  m_categories; }

private:
    static Utils::FilePath getQmlEffectNodesPath();

    QList<EffectNodesCategory *> m_categories;
};

} // namespace QmlDesigner
