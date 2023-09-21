// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debuggerconstants.h"

#include <utils/filepath.h>

#include <QMetaType>

namespace Debugger::Internal {

class DebuggerRunParameters;
class GdbMi;

class StackFrame
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
    QString context;  // Opaque value produced and consumed by the native backends.
    uint debuggerId = 0;
};

using StackFrames = QList<StackFrame>;

} // Debugger::Internal

Q_DECLARE_METATYPE(Debugger::Internal::StackFrame)
