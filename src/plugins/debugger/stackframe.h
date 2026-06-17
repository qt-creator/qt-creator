// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <utils/filepath.h>

#include <QMetaType>

namespace Debugger { class DebuggerRunParameters; }

namespace Debugger::Internal {

class GdbMi;

class DEBUGGER_EXPORT StackFrame
{
public:
    StackFrame();
    void clear();
    bool isUsable() const;
    QString toToolTip() const;
    QString toString() const;
    static StackFrame parseFrame(const GdbMi &data, const DebuggerRunParameters &rp);
    static QList<StackFrame> parseFrames(const GdbMi &data, const DebuggerRunParameters &rp);
    void fixQrcFrame(const DebuggerRunParameters &rp);

public:
    DebuggerLanguage language = CppLanguage;
    QString level;
    QString function;
    Utils::FilePath file;// We try to put an absolute file name in there.
    QString module;      // Sometimes something like "/usr/lib/libstdc++.so.6"
    QString receiver;    // Used in ScriptEngine only.
    qint32 line = -1;
    quint64 address = 0;
    bool usable = false;
    bool machinery = false; // Executes debugger machinery rather than user code.
    bool machineryPlaceholder = false; // Stands in for a collapsed run of machinery frames.
    quint64 machineryRunId = 0; // Identifies the collapsed run (address of its first frame).
    QString context;  // Opaque value produced and consumed by the native backends.
    uint debuggerId = 0;
};

using StackFrames = QList<StackFrame>;

} // Debugger::Internal

Q_DECLARE_METATYPE(Debugger::Internal::StackFrame)
