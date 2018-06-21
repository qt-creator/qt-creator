/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QTreeView>
#include "qmt/infrastructure/qmt_global.h"
#include "qmt/model_ui/modeltreeviewinterface.h"

#include <QTime>

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
    QTime m_autoDelayStartTime;
};

} // namespace qmt
