/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef DEBUGGER_WATCHWINDOW_H
#define DEBUGGER_WATCHWINDOW_H

#include <utils/basetreeview.h>

namespace Debugger {
namespace Internal {

/////////////////////////////////////////////////////////////////////
//
// WatchWindow
//
/////////////////////////////////////////////////////////////////////

enum WatchType { LocalsType, InspectType, WatchersType, ReturnType, TooltipType };

class WatchTreeView : public Utils::BaseTreeView
{
    Q_OBJECT

public:
    explicit WatchTreeView(WatchType type);
    WatchType type() const { return m_type; }

    void setModel(QAbstractItemModel *model);
    void rowActivated(const QModelIndex &index);
    void reset();

    void fillFormatMenu(QMenu *, const QModelIndex &mi);
    static void reexpand(QTreeView *view, const QModelIndex &idx);

public slots:
    void watchExpression(const QString &exp);
    void watchExpression(const QString &exp, const QString &name);
    void handleItemIsExpanded(const QModelIndex &idx);

signals:
    void currentIndexChanged(const QModelIndex &currentIndex);

private:
    void resetHelper();
    void expandNode(const QModelIndex &idx);
    void collapseNode(const QModelIndex &idx);
    Q_SLOT void adjustSlider(); // Used by single-shot timer.

    void showUnprintable(int base);
    void doItemsLayout();
    void keyPressEvent(QKeyEvent *ev);
    void contextMenuEvent(QContextMenuEvent *ev);
    void dragEnterEvent(QDragEnterEvent *ev);
    void dropEvent(QDropEvent *ev);
    void dragMoveEvent(QDragMoveEvent *ev);
    void mouseDoubleClickEvent(QMouseEvent *ev);
    bool event(QEvent *ev);
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

    void inputNewExpression();
    void editItem(const QModelIndex &idx);

    void setModelData(int role, const QVariant &value = QVariant(),
        const QModelIndex &index = QModelIndex());

    WatchType m_type;
    bool m_grabbing;
    int m_sliderPosition;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_WATCHWINDOW_H
