// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "calltreeview.h"

#include "calltreemodel.h"

#include "profilertr.h"

#include <coreplugin/minisplitter.h>

#include <utils/itemviews.h>

#include <QAbstractItemView>
#include <QHeaderView>
#include <QTreeWidget>
#include <QVBoxLayout>

using namespace Profiler;

namespace QmlProfiler::Internal {

CallTreeView::CallTreeView(CallTreeModel *model, QWidget *parent)
    : QWidget(parent)
    , m_model(model)
{
    m_tree = new Utils::TreeView(this);
    m_tree->setModel(model);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setUniformRowHeights(true);
    // The symbol fills all remaining space; the numeric columns just fit
    // their content. Without disabling stretchLastSection, "Self" would
    // stretch as well.
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(CallTreeModel::SymbolColumn, QHeaderView::Stretch);

    m_heaviest = new QTreeWidget(this);
    m_heaviest->setColumnCount(2);
    m_heaviest->setHeaderLabels({Tr::tr("Weight"), Tr::tr("Heaviest Stack")});
    m_heaviest->setRootIsDecorated(false);
    m_heaviest->setUniformRowHeights(true);
    m_heaviest->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_heaviest->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_heaviest->header()->setStretchLastSection(false);

    auto splitter = new Core::MiniSplitter(Qt::Horizontal);
    splitter->addWidget(m_tree);
    splitter->addWidget(m_heaviest);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter);

    connect(m_tree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this] { updateHeaviestStack(); });
    connect(m_tree, &QAbstractItemView::doubleClicked,
            this, &CallTreeView::emitGotoForIndex);
    connect(m_heaviest, &QTreeWidget::currentItemChanged,
            this, [this](QTreeWidgetItem *cur, QTreeWidgetItem *) { onHeaviestCurrentChanged(cur); });
    connect(m_heaviest, &QTreeWidget::itemDoubleClicked,
            this, [this](QTreeWidgetItem *item, int) { onHeaviestDoubleClicked(item); });
    connect(model, &QAbstractItemModel::modelReset, this, [this] { updateHeaviestStack(); });

    updateHeaviestStack();
}

void CallTreeView::updateHeaviestStack()
{
    if (m_syncingSelection) // a heaviest->tree click must not re-root the pane
        return;
    m_heaviest->clear();
    const CallTreeModel::Node *from = m_model->node(m_tree->currentIndex());
    const QList<const CallTreeModel::Node *> path = m_model->heaviestPath(from);
    for (const CallTreeModel::Node *node : path) {
        auto item = new QTreeWidgetItem(m_heaviest);
        item->setText(0, QString::number(node->weight));
        item->setTextAlignment(0, Qt::AlignRight | Qt::AlignVCenter);
        item->setText(1, m_model->symbol(node));
        item->setData(0, Qt::UserRole,
                      QVariant::fromValue(QPersistentModelIndex(m_model->indexFor(node))));
    }
}

void CallTreeView::emitGotoForIndex(const QModelIndex &index)
{
    const CallTreeModel::Node *node = m_model->node(index);
    if (!node)
        return;
    const CallTreeModel::SourceLocation loc = m_model->location(node);
    if (!loc.file.isEmpty())
        emit gotoSourceLocation(loc.file, loc.line, 0);
}

void CallTreeView::onHeaviestCurrentChanged(QTreeWidgetItem *item)
{
    if (!item)
        return;
    const QModelIndex index = item->data(0, Qt::UserRole).value<QPersistentModelIndex>();
    if (!index.isValid())
        return;
    // Navigate the tree to the frame without rebuilding (and thus re-rooting)
    // the heaviest pane, so a double-click's two clicks land on the same row.
    m_syncingSelection = true;
    m_tree->setCurrentIndex(index);
    m_tree->scrollTo(index);
    m_syncingSelection = false;
}

void CallTreeView::onHeaviestDoubleClicked(QTreeWidgetItem *item)
{
    if (!item)
        return;
    emitGotoForIndex(item->data(0, Qt::UserRole).value<QPersistentModelIndex>());
}

} // namespace QmlProfiler::Internal
