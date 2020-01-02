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

#include <QMetaType>
#include <QString>

#include <utils/fileutils.h>

namespace Debugger {
namespace Internal {

class GdbMi;

//////////////////////////////////////////////////////////////////
//
// BreakpointData
//
//////////////////////////////////////////////////////////////////

//! \enum Debugger::Internal::BreakpointType

// Note: Keep synchronized with similar definitions in dumper.py
enum BreakpointType
{
    UnknownBreakpointType,
    BreakpointByFileAndLine,
    BreakpointByFunction,
    BreakpointByAddress,
    BreakpointAtThrow,
    BreakpointAtCatch,
    BreakpointAtMain,
    BreakpointAtFork,
    BreakpointAtExec,
    BreakpointAtSysCall,
    WatchpointAtAddress,
    WatchpointAtExpression,
    BreakpointOnQmlSignalEmit,
    BreakpointAtJavaScriptThrow,
    LastBreakpointType
};

//! \enum Debugger::Internal::BreakpointState
enum BreakpointState
{
    BreakpointNew,
    BreakpointInsertionRequested,  //!< Inferior was told about bp, not ack'ed.
    BreakpointInsertionProceeding,
    BreakpointInserted,
    BreakpointUpdateRequested,
    BreakpointUpdateProceeding,
    BreakpointRemoveRequested,
    BreakpointRemoveProceeding,
    BreakpointDead
};

//! \enum Debugger::Internal::BreakpointPathUsage
enum BreakpointPathUsage
{
    BreakpointPathUsageEngineDefault, //!< Default value that suits the engine.
    BreakpointUseFullPath,            //!< Use full path to avoid ambiguities. Slow with gdb.
    BreakpointUseShortPath            //!< Use filename only, in case source files are relocated.
};

enum BreakpointColumns
{
    BreakpointNumberColumn,
    BreakpointFunctionColumn,
    BreakpointFileColumn,
    BreakpointLineColumn,
    BreakpointAddressColumn,
    BreakpointConditionColumn,
    BreakpointIgnoreColumn,
    BreakpointThreadsColumn
};

enum BreakpointParts
{
    NoParts         = 0,
    FileAndLinePart = (1 <<  0),
    FunctionPart    = (1 <<  1),
    AddressPart     = (1 <<  2),
    ExpressionPart  = (1 <<  3),
    ConditionPart   = (1 <<  4),
    IgnoreCountPart = (1 <<  5),
    ThreadSpecPart  = (1 <<  6),
    ModulePart      = (1 <<  7),
    TracePointPart  = (1 <<  8),

    EnabledPart     = (1 <<  9),
    TypePart        = (1 << 10),
    PathUsagePart   = (1 << 11),
    CommandPart     = (1 << 12),
    MessagePart     = (1 << 13),
    OneShotPart     = (1 << 14),

    AllConditionParts = ConditionPart|IgnoreCountPart|ThreadSpecPart
               |OneShotPart,

    AllParts = FileAndLinePart|FunctionPart
               |ExpressionPart|AddressPart|ConditionPart
               |IgnoreCountPart|ThreadSpecPart|ModulePart|TracePointPart
               |EnabledPart|TypePart|PathUsagePart|CommandPart|MessagePart
               |OneShotPart
};

inline void operator|=(BreakpointParts &p, BreakpointParts r)
{
    p = BreakpointParts(int(p) | int(r));
}

class BreakpointParameters
{
public:
    explicit BreakpointParameters(BreakpointType = UnknownBreakpointType);
    BreakpointParts differencesTo(const BreakpointParameters &rhs) const;
    bool isValid() const;
    bool equals(const BreakpointParameters &rhs) const;
    bool conditionsMatch(const QString &other) const;
    bool isWatchpoint() const
        { return type == WatchpointAtAddress || type == WatchpointAtExpression; }
    bool isLocatedAt(const Utils::FilePath &file, int line, const Utils::FilePath &markerFile) const;
    // Enough for now.
    bool isBreakpoint() const { return !isWatchpoint() && !isTracepoint(); }
    bool isTracepoint() const { return tracepoint; }
    bool isCppBreakpoint() const;
    bool isQmlFileAndLineBreakpoint() const;
    QString toString() const;
    void updateLocation(const QString &location); // file.cpp:42
    void updateFromGdbOutput(const GdbMi &bkpt);

    bool operator==(const BreakpointParameters &p) const { return equals(p); }
    bool operator!=(const BreakpointParameters &p) const { return !equals(p); }

    BreakpointType type;     //!< Type of breakpoint.
    bool enabled;            //!< Should we talk to the debugger engine?
    BreakpointPathUsage pathUsage;  //!< Should we use the full path when setting the bp?
    Utils::FilePath fileName;//!< Short name of source file.
    QString condition;       //!< Condition associated with breakpoint.
    int ignoreCount;         //!< Ignore count associated with breakpoint.
    int lineNumber;          //!< Line in source file.
    quint64 address;         //!< Address for address based data breakpoints.
    QString expression;      //!< Expression for expression based data breakpoints.
    uint size;               //!< Size of watched area for data breakpoints.
    uint bitpos;             //!< Location of watched bitfield within watched area.
    uint bitsize;            //!< Size of watched bitfield within watched area.
    int threadSpec;          //!< Thread specification.
    QString functionName;
    QString module;          //!< module for file name
    QString command;         //!< command to execute
    QString message;         //!< message
    bool tracepoint;
    bool oneShot;            //!< Should this breakpoint trigger only once?

    bool pending = true;     //!< Breakpoint not fully resolved.
    int hitCount = 0;        //!< Number of times this has been hit.
};

} // namespace Internal
} // namespace Debugger
