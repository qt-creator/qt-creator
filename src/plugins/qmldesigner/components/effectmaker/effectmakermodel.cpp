// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectmakermodel.h"

#include "compositionnode.h"

#include <utils/qtcassert.h>

namespace QmlDesigner {

EffectMakerModel::EffectMakerModel(QObject *parent)
    : QAbstractListModel{parent}
{
}

QHash<int, QByteArray> EffectMakerModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "nodeName";
    return roles;
}

int EffectMakerModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_nodes.count();
}

QVariant EffectMakerModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_nodes.size(), return {});
    QTC_ASSERT(roleNames().contains(role), return {});

    return m_nodes.values().at(index.row())->property(roleNames().value(role));
}

void EffectMakerModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void EffectMakerModel::addNode(const QString &nodeQenPath)
{
    static int id = 0;

    beginInsertRows({}, m_nodes.size(), m_nodes.size());
    auto *node = new CompositionNode(nodeQenPath);
    m_nodes.insert(id++, node);
    endInsertRows();
}

void EffectMakerModel::selectEffect(int idx, bool force)
{
    Q_UNUSED(idx)
    Q_UNUSED(force)

    // TODO
}

void EffectMakerModel::applyToSelected(qint64 internalId, bool add)
{
    Q_UNUSED(internalId)
    Q_UNUSED(add)

    // TODO: remove?
}

} // namespace QmlDesigner
