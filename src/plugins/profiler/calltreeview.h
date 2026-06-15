// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QModelIndex>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace Utils { class TreeView; }

namespace QmlProfiler::Internal {

class CallTreeModel;

// The "Call Stacks" view: the merged call tree on the left, the heaviest
// stack of the current tree selection on the right (the chain of heaviest
// children, recomputed whenever the selection or the model changes).
class CallTreeView : public QWidget
{
    Q_OBJECT

public:
    explicit CallTreeView(CallTreeModel *model, QWidget *parent = nullptr);

signals:
    void gotoSourceLocation(const QString &file, int line, int column,
                            const QString &module, quint64 offset);

private:
    void updateHeaviestStack();
    void emitGotoForIndex(const QModelIndex &index);
    void onHeaviestCurrentChanged(QTreeWidgetItem *item); // single-click: navigate tree
    void onHeaviestDoubleClicked(QTreeWidgetItem *item);  // double-click: go to source

    CallTreeModel *m_model = nullptr;
    Utils::TreeView *m_tree = nullptr;
    QTreeWidget *m_heaviest = nullptr;
    bool m_syncingSelection = false; // suppress pane rebuild during heaviest->tree sync
};

} // namespace QmlProfiler::Internal
