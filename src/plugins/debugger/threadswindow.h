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

#ifndef DEBUGGER_THREADWINDOW_H
#define DEBUGGER_THREADWINDOW_H

#include <QtGui/QTreeView>

namespace Debugger {
namespace Internal {
    
class ThreadsWindow : public QTreeView
{
    Q_OBJECT

public:
    ThreadsWindow(QWidget *parent = 0);

signals:
    void threadSelected(int);

public slots:
    void resizeColumnsToContents();
    void setAlwaysResizeColumnsToContents(bool on);

private slots:
    void rowActivated(const QModelIndex &index);

private:
    void resizeEvent(QResizeEvent *ev);
    void contextMenuEvent(QContextMenuEvent *ev);

    bool m_alwaysResizeColumnsToContents;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_THREADWINDOW_H
