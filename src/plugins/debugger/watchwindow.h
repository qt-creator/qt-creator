/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_WATCHWINDOW_H
#define DEBUGGER_WATCHWINDOW_H

#include <QtGui/QTreeView>

namespace Debugger {
namespace Internal {

/////////////////////////////////////////////////////////////////////
//
// WatchWindow
//
/////////////////////////////////////////////////////////////////////

class WatchWindow : public QTreeView
{
    Q_OBJECT

public:
    enum Type { ReturnType, LocalsType, TooltipType, WatchersType };

    explicit WatchWindow(Type type, QWidget *parent = 0);
    Type type() const { return m_type; }

public slots:
    void resizeColumnsToContents();
    void setAlwaysResizeColumnsToContents(bool on = true);
    void setModel(QAbstractItemModel *model);
    void setAlternatingRowColorsHelper(bool on) { setAlternatingRowColors(on); }
    void watchExpression(const QString &exp);
    void removeWatchExpression(const QString &exp);

private:
    Q_SLOT void resetHelper();
    Q_SLOT void expandNode(const QModelIndex &idx);
    Q_SLOT void collapseNode(const QModelIndex &idx);
    Q_SLOT void setUpdatesEnabled(bool enable);

    void keyPressEvent(QKeyEvent *ev);
    void contextMenuEvent(QContextMenuEvent *ev);
    void dragEnterEvent(QDragEnterEvent *ev);
    void dropEvent(QDropEvent *ev);
    void dragMoveEvent(QDragMoveEvent *ev);
    void mouseDoubleClickEvent(QMouseEvent *ev);
    bool event(QEvent *ev);

    void editItem(const QModelIndex &idx);
    void resetHelper(const QModelIndex &idx);
    void setWatchpoint(quint64 address);

    void setModelData(int role, const QVariant &value = QVariant(),
        const QModelIndex &index = QModelIndex());

    bool m_alwaysResizeColumnsToContents;
    Type m_type;
    bool m_grabbing;
};


} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_WATCHWINDOW_H
