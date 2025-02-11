// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectcomposereditablenodesmodel.h"

#include "effectcomposermodel.h"
#include "effectcomposertr.h"

namespace EffectComposer {

EffectComposerEditableNodesModel::EffectComposerEditableNodesModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int EffectComposerEditableNodesModel::rowCount(const QModelIndex &) const
{
    return m_data.size();
}

QVariant EffectComposerEditableNodesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (role == Qt::DisplayRole)
        return m_data.at(index.row()).nodeName;

    return {};
}

void EffectComposerEditableNodesModel::setSourceModel(EffectComposerModel *sourceModel)
{
    m_sourceModel = sourceModel;
    if (sourceModel) {
        connect(
            sourceModel,
            &QAbstractItemModel::modelReset,
            this,
            &EffectComposerEditableNodesModel::reload);
        connect(
            sourceModel,
            &QAbstractItemModel::rowsInserted,
            this,
            &EffectComposerEditableNodesModel::reload);
        connect(
            sourceModel,
            &QAbstractItemModel::rowsRemoved,
            this,
            &EffectComposerEditableNodesModel::reload);
        connect(
            sourceModel,
            &QAbstractItemModel::rowsMoved,
            this,
            &EffectComposerEditableNodesModel::reload);
        connect(
            sourceModel,
            &QAbstractItemModel::dataChanged,
            this,
            &EffectComposerEditableNodesModel::onSourceDataChanged);
        connect(
            sourceModel,
            &EffectComposerModel::codeEditorIndexChanged,
            this,
            &EffectComposerEditableNodesModel::onCodeEditorIndexChanged);
    }
    reload();
}

QModelIndex EffectComposerEditableNodesModel::proxyIndex(int sourceIndex) const
{
    if (!m_sourceModel)
        return {};

    QModelIndex sourceIdx = m_sourceModel->index(sourceIndex, 0, {});
    if (!sourceIdx.isValid())
        return {};

    return index(m_sourceToItemMap.value(sourceIndex, -1), 0, {});
}

void EffectComposerEditableNodesModel::openCodeEditor(int proxyIndex)
{
    if (!m_sourceModel)
        return;

    if (proxyIndex < 0 || proxyIndex >= m_data.size())
        return;

    m_sourceModel->openCodeEditor(m_data.at(proxyIndex).sourceId);
}

void EffectComposerEditableNodesModel::onSourceDataChanged(
    const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    using Roles = EffectComposerModel::Roles;

    if (!m_sourceModel)
        return;

    const int startItem = topLeft.row();
    const int endItem = bottomRight.row();

    if (roles.contains(Roles::Dependency)) {
        reload();
        return;
    }

    if (!roles.contains(Roles::NameRole))
        return;

    for (int i = startItem; i < endItem; ++i) {
        QModelIndex sourceIdx = m_sourceModel->index(i);
        QModelIndex proxyIdx = proxyIndex(i);
        if (proxyIdx.isValid()) {
            m_data[proxyIdx.row()].nodeName = sourceIdx.data(Roles::NameRole).toString();
            emit dataChanged(proxyIdx, proxyIdx, {Qt::DisplayRole});
        }
    }
}

void EffectComposerEditableNodesModel::onCodeEditorIndexChanged(int sourceIndex)
{
    int newSelectedIndex = m_sourceToItemMap.value(sourceIndex, -1);
    if (m_selectedIndex != newSelectedIndex) {
        m_selectedIndex = newSelectedIndex;
        emit selectedIndexChanged(m_selectedIndex);
    }
}

void EffectComposerEditableNodesModel::reload()
{
    using Roles = EffectComposerModel::Roles;
    beginResetModel();
    m_data.clear();
    m_sourceToItemMap.clear();

    if (!m_sourceModel) {
        endResetModel();
        return;
    }

    const int mainIdx = m_sourceModel->mainCodeEditorIndex();

    m_data.append(Item{Tr::tr("Main"), mainIdx});
    m_sourceToItemMap.insert(mainIdx, 0);
    const int sourceSize = m_sourceModel->rowCount();
    for (int i = 0; i < sourceSize; ++i) {
        QModelIndex idx = m_sourceModel->index(i, 0, {});
        bool isDependency = idx.data(Roles::Dependency).toBool();
        if (!isDependency) {
            Item item;
            item.nodeName = idx.data(Roles::NameRole).toString();
            item.sourceId = i;
            m_data.append(item);
            m_sourceToItemMap.insert(i, m_sourceToItemMap.size());
        }
    }
    endResetModel();
}

} // namespace EffectComposer
