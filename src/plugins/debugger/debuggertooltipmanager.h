/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_DEBUGGERTOOLTIPMANAGER_H
#define DEBUGGER_DEBUGGERTOOLTIPMANAGER_H

#include "debuggerconstants.h"

#include <QCoreApplication>
#include <QDate>
#include <QPoint>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

namespace Debugger {

class DebuggerEngine;

namespace Internal {

class StackFrame;

class DebuggerToolTipContext
{
public:
    DebuggerToolTipContext();
    bool isValid() const { return !expression.isEmpty(); }
    bool matchesFrame(const StackFrame &frame) const;
    bool isSame(const DebuggerToolTipContext &other) const;

    QString fileName;
    int position;
    int line;
    int column;
    int scopeFromLine;
    int scopeToLine;
    QString function; //!< Optional, informational only.
    QString engineType;
    QDate creationDate;

    QPoint mousePosition;
    QString expression;
    QByteArray iname;
};

typedef QList<DebuggerToolTipContext> DebuggerToolTipContexts;

class DebuggerToolTipManager : public QObject
{
    Q_OBJECT

public:
    DebuggerToolTipManager();
    ~DebuggerToolTipManager();

    static void registerEngine(DebuggerEngine *engine);
    static void deregisterEngine(DebuggerEngine *engine);
    static void updateEngine(DebuggerEngine *engine);
    static bool hasToolTips();

    static DebuggerToolTipContexts pendingTooltips(DebuggerEngine *engine);

    virtual bool eventFilter(QObject *, QEvent *);

    static QString treeModelClipboardContents(const QAbstractItemModel *model);

    void debugModeEntered();
    void leavingDebugMode();
    void sessionAboutToChange();
    static void loadSessionData();
    static void saveSessionData();
    static void closeAllToolTips();
    static void resetLocation();

public slots:
    static void slotUpdateVisibleToolTips();
    void slotItemIsExpanded(const QModelIndex &idx);
    void slotColumnAdjustmentRequested();
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_INTERNAL_DEBUGGERTOOLTIPMANAGER_H
