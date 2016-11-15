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

#pragma once

#include "debuggerconstants.h"

#include <QCoreApplication>
#include <QDate>
#include <QPoint>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class DebuggerEngine;
class DebuggerCommand;
class StackFrame;

class DebuggerToolTipContext
{
public:
    DebuggerToolTipContext();
    bool isValid() const { return !expression.isEmpty(); }
    bool matchesFrame(const StackFrame &frame) const;
    bool isSame(const DebuggerToolTipContext &other) const;
    QString toolTip() const;

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
    QString iname;
    bool isCppEditor;
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

    void debugModeEntered();
    void leavingDebugMode();
    void sessionAboutToChange();
    static void loadSessionData();
    static void saveSessionData();
    static void closeAllToolTips();
    static void resetLocation();
    static void updateVisibleToolTips();
};

} // namespace Internal
} // namespace Debugger
