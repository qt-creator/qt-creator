/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef DEBUGGER_SNAPSHOTWINDOW_H
#define DEBUGGER_SNAPSHOTWINDOW_H

#include "basewindow.h"

namespace Debugger {
namespace Internal {

class SnapshotHandler;

class SnapshotTreeView : public BaseTreeView
{
    Q_OBJECT

public:
    explicit SnapshotTreeView(SnapshotHandler *handler);

private:
    void rowActivated(const QModelIndex &index);
    void removeSnapshot(int i);
    void keyPressEvent(QKeyEvent *ev);
    void contextMenuEvent(QContextMenuEvent *ev);

    SnapshotHandler *m_snapshotHandler;
};

class SnapshotWindow : public BaseWindow
{
    Q_OBJECT
public:
    explicit SnapshotWindow(SnapshotHandler *handler)
        : BaseWindow(new SnapshotTreeView(handler))
    {
        setWindowTitle(tr("Snapshots"));
    }
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_SNAPSHOTWINDOW_H

