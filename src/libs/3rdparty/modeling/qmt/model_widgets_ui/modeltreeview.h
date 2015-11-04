/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMT_MODELTREEVIEW_H
#define QMT_MODELTREEVIEW_H

#include <QTreeView>
#include "qmt/infrastructure/qmt_global.h"
#include "qmt/model_ui/modeltreeviewinterface.h"

#include <QTime>


namespace qmt {

class SortedTreeModel;
class IElementTasks;


class QMT_EXPORT ModelTreeView :
        public QTreeView,
        public ModelTreeViewInterface
{
    Q_OBJECT

public:

    explicit ModelTreeView(QWidget *parent = 0);

    ~ModelTreeView();

signals:

    void treeViewActivated();

public:

    QModelIndex currentSourceModelIndex() const;

    QList<QModelIndex> selectedSourceModelIndexes() const;

public:

    void setTreeModel(SortedTreeModel *model);

    void setElementTasks(IElementTasks *elementTasks);

public:

    QModelIndex mapToSourceModelIndex(const QModelIndex &index) const;

    void selectFromSourceModelIndex(const QModelIndex &index);

protected:

    void startDrag(Qt::DropActions supportedActions);

    void dragEnterEvent(QDragEnterEvent *event);

    void dragMoveEvent(QDragMoveEvent *event);

    void dragLeaveEvent(QDragLeaveEvent *event);

    void dropEvent(QDropEvent *event);

    void focusInEvent(QFocusEvent *event);

    void contextMenuEvent(QContextMenuEvent *event);

private:

    SortedTreeModel *m_sortedTreeModel;

    IElementTasks *m_elementTasks;

    QModelIndex m_autoDelayIndex;

    QTime m_autoDelayStartTime;
};

}

#endif // QMT_MODELTREEVIEW_H
