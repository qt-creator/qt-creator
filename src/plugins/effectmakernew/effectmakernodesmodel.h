// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "effectnodescategory.h"

#include <utils/filepath.h>

#include <QStandardItemModel>

namespace EffectMaker {

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
    void findNodesPath();

    QList<EffectNodesCategory *> m_categories;
    Utils::FilePath m_nodesPath;
    bool m_probeNodesDir = false;
};

} // namespace EffectMaker

