// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/breakhandler.h>

#include <QPair>

QT_BEGIN_NAMESPACE
class QVariant;
class QDebug;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {
class BreakpointParameters;
struct ThreadData;
class Register;
class GdbMi;
class DisassemblerLines;

// Perform mapping on parts of the source tree as reported by/passed to debugger
// in case the user has specified such mappings in the global settings.
enum SourcePathMode { DebuggerToSource, SourceToDebugger };
QString cdbSourcePathMapping(QString fileName,
                             const QList<QPair<QString, QString> > &sourcePathMapping,
                             SourcePathMode mode);

// Ensure unique 'namespace' for breakpoints of the breakhandler.
enum { cdbBreakPointStartId = 100000,
       cdbBreakPointIdMinorPart = 100};

QString breakPointCdbId(const Breakpoint &bp);

// Convert breakpoint in CDB syntax (applying source path mappings using native paths).
QString cdbAddBreakpointCommand(const BreakpointParameters &d,
                                const QList<QPair<QString, QString> > &sourcePathMapping,
                                const QString &responseId = QString());
QString cdbClearBreakpointCommand(const Breakpoint &bp);

// Write memory (f ...).
QString cdbWriteMemoryCommand(quint64 addr, const QByteArray &data);

QString debugByteArray(const QByteArray &a);

DisassemblerLines parseCdbDisassembler(const QString &a);

// Model EXCEPTION_RECORD + firstchance
struct WinException
{
    WinException();
    void fromGdbMI(const GdbMi &);
    QString toString(bool includeLocation = false) const;

    friend  QDebug operator<<(QDebug s, const WinException &e);

    unsigned exceptionCode = 0;
    unsigned exceptionFlags = 0;
    quint64 exceptionAddress = 0;
    quint64 info1 = 0;
    quint64 info2 = 0;
    bool firstChance = false;
    QString file;
    int lineNumber = 0;
    QString function;
};

} // namespace Internal
} // namespace Debugger
