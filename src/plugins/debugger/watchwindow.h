/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
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
    void requestRemoveWatchExpression(const QString &iname);
    void requestAssignValue(const QString &exp, const QString &value);
    void requestExpandChildren(const QModelIndex &idx);
    void requestCollapseChildren(const QModelIndex &idx);

private slots:
    void handleChangedItem(QWidget *);
    void expandNode(const QModelIndex &index);
    void collapseNode(const QModelIndex &index);
    void modelAboutToBeReset();
    void modelReset();

private:
    void contextMenuEvent(QContextMenuEvent *ev);
    void editItem(const QModelIndex &idx);
    void reset(); /* reimpl */

    void modelAboutToBeResetHelper(const QModelIndex &idx);
    void modelResetHelper(const QModelIndex &idx);

    bool m_alwaysResizeColumnsToContents;
    Type m_type;
    bool m_blocked;
    QSet<QString> m_expandedItems;
};


} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_WATCHWINDOW_H
