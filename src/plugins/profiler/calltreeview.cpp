// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "calltreeview.h"

#include "calltreemodel.h"

#include "profilertr.h"

#include <coreplugin/minisplitter.h>

#include <utils/itemviews.h>

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
    connect(m_heaviest, &QTreeWidget::itemActivated,
            this, &CallTreeView::activateHeaviestFrame);
    connect(model, &QAbstractItemModel::modelReset, this, [this] { updateHeaviestStack(); });

    updateHeaviestStack();
}

void CallTreeView::updateHeaviestStack()
{
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

void CallTreeView::activateHeaviestFrame(QTreeWidgetItem *item)
{
    const QModelIndex index = item->data(0, Qt::UserRole).value<QPersistentModelIndex>();
    if (!index.isValid())
        return;
    m_tree->setCurrentIndex(index);
    m_tree->scrollTo(index);
}

} // namespace QmlProfiler::Internal
