// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectmakermodel.h"

#include "compositionnode.h"

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

QVariant EffectMakerModel::data(const QModelIndex &index, int /*role*/) const
{
    if (index.row() < 0 || index.row() >= m_nodes.count())
        return {};

    // TODO

    return {};
}

void EffectMakerModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void EffectMakerModel::addNode(const QString &nodeQenPath)
{
    static int id = 0;

    auto *node = new CompositionNode(nodeQenPath);
    m_nodes.insert(id++, node);
//    TODO: update model
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
