// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "effectcomposeruniformsmodel.h"

#include <QAbstractTableModel>
#include <QPointer>

namespace EffectComposer {

class EffectComposerUniformsTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    using UniformRole = EffectComposerUniformsModel::Roles;

    struct SourceIndex
    {
        QVariant value() const { return index.data(role); }
        QString valueTypeString() const;
        QString display() const;

        QModelIndex index;
        UniformRole role = UniformRole::NameRole;
    };

    enum Role {
        ValueRole = Qt::UserRole + 1,
        ValueTypeRole,
        IsDescriptionRole,
        CanCopyRole,
    };

    explicit EffectComposerUniformsTableModel(
        EffectComposerUniformsModel *sourceModel, QObject *parent = nullptr);

    SourceIndex mapToSource(const QModelIndex &proxyIndex) const;

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(
        int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    EffectComposerUniformsModel *sourceModel() const;

private:
    void onSourceRowsAboutToBeInserted(const QModelIndex &parent, int first, int last);
    void onSourceRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onSourceRowsAboutToBeMoved(
        const QModelIndex &sourceParent,
        int sourceStart,
        int sourceEnd,
        const QModelIndex &destinationParent,
        int destinationRow);
    void onSourceDataChanged(
        const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles);

    QPointer<EffectComposerUniformsModel> m_sourceModel;
};

} // namespace EffectComposer
