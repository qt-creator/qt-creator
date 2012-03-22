/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_BASEWINDOW_H
#define DEBUGGER_BASEWINDOW_H

#include <QTreeView>

namespace Debugger {
namespace Internal {

class BaseWindow : public QWidget
{
    Q_OBJECT

public:
    BaseWindow(QWidget *parent = 0);
    virtual void setModel(QAbstractItemModel *model);
    QHeaderView *header() const { return m_treeView->header(); }
    QAbstractItemModel *model() const { return m_treeView->model(); }

protected slots:
    void resizeColumnsToContents();
    void setAlwaysResizeColumnsToContents(bool on);

private slots:
    void setAlternatingRowColorsHelper(bool on);
    void rowActivatedHelper(const QModelIndex &index);
    void headerSectionClicked(int logicalIndex);
    void reset();

protected:
    void setAlwaysAdjustColumnsAction(QAction *action);
    void addBaseContextActions(QMenu *menu);
    bool handleBaseContextAction(QAction *action);

    QTreeView *treeView() const { return m_treeView; }
    QModelIndex indexAt(const QPoint &p) const { return m_treeView->indexAt(p); }
    void resizeColumnToContents(int col) { m_treeView->resizeColumnToContents(col); }
    void mousePressEvent(QMouseEvent *ev);
    virtual void rowActivated(const QModelIndex &) {}
    QModelIndexList selectedIndices(QContextMenuEvent *ev = 0);

private:
    QTreeView *m_treeView;
    QAction *m_alwaysAdjustColumnsAction;
    QAction *m_adjustColumnsAction;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_BASEWINDOW_H
