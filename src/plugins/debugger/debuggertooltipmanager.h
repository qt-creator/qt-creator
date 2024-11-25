// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QPoint>

namespace Debugger::Internal {

class DebuggerEngine;
class StackFrame;

class DebuggerToolTipContext
{
public:
    bool isValid() const { return !expression.isEmpty(); }
    bool matchesFrame(const StackFrame &frame) const;
    bool isSame(const DebuggerToolTipContext &other) const;
    QString toolTip() const;

    Utils::FilePath fileName;
    int position = 0;
    int line = 0;
    int column = 0;
    int scopeFromLine = 0;
    int scopeToLine = 0;
    QString function; //!< Optional, informational only.
    QString engineType;

    QPoint mousePosition;
    QString expression;
    QString iname;
    bool isCppEditor = true;
};

using DebuggerToolTipContexts = QList<DebuggerToolTipContext>;

class DebuggerToolTipManager
{
    Q_DISABLE_COPY_MOVE(DebuggerToolTipManager)

public:
    explicit DebuggerToolTipManager(DebuggerEngine *engine);
    ~DebuggerToolTipManager();

    void deregisterEngine();
    void updateToolTips();
    bool hasToolTips() const;

    DebuggerToolTipContexts pendingTooltips() const;

    void closeAllToolTips();
    void resetLocation();

private:
    class DebuggerToolTipManagerPrivate *d;
};

} // Debugger::Internal
