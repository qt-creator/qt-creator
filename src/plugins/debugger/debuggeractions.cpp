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

#include "debuggeractions.h"

#include <QtGui/QAction>
#include <QtGui/QApplication>

namespace Debugger {
namespace Internal {


//////////////////////////////////////////////////////////////////////////
//
// Debugger specific stuff
//
//////////////////////////////////////////////////////////////////////////

struct ActionData
{
    ActionCode code;
    const char *text;
    bool checkable;
};

static ActionData data[] =
{
    //
    //  General
    // 
    { AdjustColumnWidths,
        QT_TR_NOOP("Adjust column widths to contents"), false },
    { AlwaysAdjustColumnWidths,
        QT_TR_NOOP("Adjust column widths to contents"), true },

    //
    // Locals & Watchers
    //
    { WatchExpression,
        QT_TR_NOOP("Watch expression '%1'"), false },
    { WatchExpression,
        QT_TR_NOOP("Watch expression '%1'"), false },
    { SettingsDialog,
        QT_TR_NOOP("Debugger properties..."), false },
    { UseDumpers,
        QT_TR_NOOP("Use custom dumpers"), true },
    { DebugDumpers,
        QT_TR_NOOP("Debug custom dumpers"), true },
    { RecheckDumpers,
        QT_TR_NOOP("Recheck custom dumper availability"), true },

    //
    // Breakpoints
    //
    { SynchronizeBreakpoints,
        QT_TR_NOOP("Syncronize breakpoints"), false },
};

QAction *action(ActionCode code)
{
    static QHash<ActionCode, QAction *> actions;

    if (actions.isEmpty()) {
        for (int i = 0; i != sizeof(data)/sizeof(data[0]); ++i) {
            const ActionData &d = data[i];
            QAction *act = new QAction(QObject::tr(d.text), 0);
            act->setCheckable(d.checkable);
            actions[d.code] = act;
        }
    }

    return actions.value(code);
}

} // namespace Internal
} // namespace Debugger

