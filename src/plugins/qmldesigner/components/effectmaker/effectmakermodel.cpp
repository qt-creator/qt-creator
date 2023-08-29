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
    roles[UniformsRole] = "nodeUniformsModel";
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

    return m_nodes.at(index.row())->property(roleNames().value(role));
}

void EffectMakerModel::addNode(const QString &nodeQenPath)
{
    beginInsertRows({}, m_nodes.size(), m_nodes.size());
    auto *node = new CompositionNode(nodeQenPath);
    m_nodes.append(node);
    endInsertRows();
}

void EffectMakerModel::removeNode(int idx)
{
    beginRemoveRows({}, idx, idx);
    m_nodes.removeAt(idx);
    endRemoveRows();
}

} // namespace QmlDesigner
