// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/qmt_global.h"
#include "qmt/model_ui/modeltreeviewinterface.h"

#include <QElapsedTimer>
#include <QTreeView>

namespace qmt {

class SortedTreeModel;
class IElementTasks;

class QMT_EXPORT ModelTreeView : public QTreeView, public ModelTreeViewInterface
{
    Q_OBJECT

public:
    explicit ModelTreeView(QWidget *parent = nullptr);
    ~ModelTreeView() override;

signals:
    void treeViewActivated();

public:
    QModelIndex currentSourceModelIndex() const override;
    QList<QModelIndex> selectedSourceModelIndexes() const override;

    void setTreeModel(SortedTreeModel *model);
    void setElementTasks(IElementTasks *elementTasks);

    QModelIndex mapToSourceModelIndex(const QModelIndex &index) const;
    void selectFromSourceModelIndex(const QModelIndex &index);

protected:
    void startDrag(Qt::DropActions supportedActions) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    SortedTreeModel *m_sortedTreeModel = nullptr;
    IElementTasks *m_elementTasks = nullptr;
    QModelIndex m_autoDelayIndex;
    QElapsedTimer m_autoDelayStartTimer;
};

} // namespace qmt
