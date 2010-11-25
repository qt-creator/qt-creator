/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGGER_BREAKWINDOW_H
#define DEBUGGER_BREAKWINDOW_H

#include "breakhandler.h"

#include <QtGui/QTreeView>

namespace Debugger {
namespace Internal {

class BreakWindow : public QTreeView
{
    Q_OBJECT

public:
    explicit BreakWindow(QWidget *parent = 0);

    static void editBreakpoint(BreakpointId id, QWidget *parent);

private slots:
    void resizeColumnsToContents();
    void setAlwaysResizeColumnsToContents(bool on);

    void rowActivated(const QModelIndex &index);
    void setAlternatingRowColorsHelper(bool on) { setAlternatingRowColors(on); }
    void showAddressColumn(bool on);

private:
    void resizeEvent(QResizeEvent *ev);
    void contextMenuEvent(QContextMenuEvent *ev);
    void keyPressEvent(QKeyEvent *ev);
    void mouseDoubleClickEvent(QMouseEvent *ev);

    void deleteBreakpoints(const BreakpointIds &ids);
    void addBreakpoint();
    void editBreakpoints(const BreakpointIds &ids);
    void associateBreakpoint(const BreakpointIds &ids, int thread);
    void setBreakpointsEnabled(const BreakpointIds &ids, bool enabled);

    bool m_alwaysResizeColumnsToContents;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_BREAKWINDOW

