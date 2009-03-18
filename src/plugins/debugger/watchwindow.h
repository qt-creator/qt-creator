/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
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
    enum Type { LocalsType, TooltipType, WatchersType };

    WatchWindow(Type type, QWidget *parent = 0);
    void setType(Type type) { m_type = type; }
    Type type() const { return m_type; }
    
public slots:
    void resizeColumnsToContents();
    void setAlwaysResizeColumnsToContents(bool on = true);
    void setModel(QAbstractItemModel *model);

signals:
    void requestWatchExpression(const QString &exp);
    void requestRemoveWatchExpression(const QString &exp);
    void requestAssignValue(const QString &exp, const QString &value);
    void requestExpandChildren(const QModelIndex &idx);
    void requestCollapseChildren(const QModelIndex &idx);

private slots:
    void handleChangedItem(QWidget *);
    void expandNode(const QModelIndex &index);
    void collapseNode(const QModelIndex &index);

private:
    void keyPressEvent(QKeyEvent *ev);
    void contextMenuEvent(QContextMenuEvent *ev);
    void editItem(const QModelIndex &idx);
    void reset(); /* reimpl */

    void resetHelper(const QModelIndex &idx);

    bool m_alwaysResizeColumnsToContents;
    Type m_type;
};


} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_WATCHWINDOW_H
