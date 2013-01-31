/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_WATCHWINDOW_H
#define DEBUGGER_WATCHWINDOW_H

#include "basewindow.h"

namespace Debugger {
namespace Internal {

/////////////////////////////////////////////////////////////////////
//
// WatchWindow
//
/////////////////////////////////////////////////////////////////////

class WatchTreeView : public BaseTreeView
{
    Q_OBJECT

public:
    enum Type { LocalsType, InspectType, WatchersType, ReturnType, TooltipType };

    explicit WatchTreeView(Type type, QWidget *parent = 0);
    Type type() const { return m_type; }
    void setModel(QAbstractItemModel *model);
    void rowClicked(const QModelIndex &index);
    void reset();

public slots:
    void watchExpression(const QString &exp);
    void watchExpression(const QString &exp, const QString &name);
    void handleItemIsExpanded(const QModelIndex &idx);

signals:
    void currentIndexChanged(const QModelIndex &currentIndex);

private:
    Q_SLOT void resetHelper();
    Q_SLOT void expandNode(const QModelIndex &idx);
    Q_SLOT void collapseNode(const QModelIndex &idx);

    void keyPressEvent(QKeyEvent *ev);
    void contextMenuEvent(QContextMenuEvent *ev);
    void dragEnterEvent(QDragEnterEvent *ev);
    void dropEvent(QDropEvent *ev);
    void dragMoveEvent(QDragMoveEvent *ev);
    void mouseDoubleClickEvent(QMouseEvent *ev);
    bool event(QEvent *ev);
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

    void editItem(const QModelIndex &idx);
    void resetHelper(const QModelIndex &idx);

    void setModelData(int role, const QVariant &value = QVariant(),
        const QModelIndex &index = QModelIndex());

    Type m_type;
    bool m_grabbing;
};

class WatchWindow : public BaseWindow
{
    Q_OBJECT

public:
    explicit WatchWindow(WatchTreeView::Type type)
        : BaseWindow(new WatchTreeView(type))
    {
        setWindowTitle(tr("Locals and Expressions"));
    }
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_WATCHWINDOW_H
