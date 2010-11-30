/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGGER_BREAKPOINT_H
#define DEBUGGER_BREAKPOINT_H

#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QString>

namespace Debugger {
namespace Internal {

typedef quint64 BreakpointId;

//////////////////////////////////////////////////////////////////
//
// BreakpointData
//
//////////////////////////////////////////////////////////////////

enum BreakpointType
{
    UnknownType,
    BreakpointByFileAndLine,
    BreakpointByFunction,
    BreakpointByAddress,
    BreakpointAtThrow,
    BreakpointAtCatch,
    BreakpointAtMain,
    Watchpoint,
};

enum BreakpointState
{
    BreakpointNew,
    BreakpointInsertRequested,  // Inferior was told about bp, not ack'ed.
    BreakpointInsertProceeding,
    BreakpointChangeRequested,
    BreakpointChangeProceeding,
    BreakpointInserted,
    BreakpointRemoveRequested,
    BreakpointRemoveProceeding,
    BreakpointDead,
};

class BreakpointParameters
{
public:
    explicit BreakpointParameters(BreakpointType = UnknownType);
    bool equals(const BreakpointParameters &rhs) const;
    bool conditionsMatch(const QByteArray &other) const;
    bool isWatchpoint() const { return type == Watchpoint; }
    bool isBreakpoint() const { return type != Watchpoint; } // Enough for now.
    QString toString() const;

    bool operator==(const BreakpointParameters &p) const { return equals(p); }
    bool operator!=(const BreakpointParameters &p) const { return !equals(p); }

    BreakpointType type;     // Type of breakpoint.
    bool enabled;            // Should we talk to the debugger engine?
    bool useFullPath;        // Should we use the full path when setting the bp?
    QString fileName;        // Short name of source file.
    QByteArray condition;    // Condition associated with breakpoint.
    int ignoreCount;         // Ignore count associated with breakpoint.
    int lineNumber;          // Line in source file.
    quint64 address;         // Address for watchpoints.
    int threadSpec;          // Thread specification.
    QString functionName;
};


// This is what debuggers produced in response to the attempt to
// insert a breakpoint. The data might differ from the requested bits.
class BreakpointResponse : public BreakpointParameters
{
public:
    BreakpointResponse();
    QString toString() const;

public:
    void fromParameters(const BreakpointParameters &p);

    int number;             // Breakpoint number assigned by the debugger engine.
    bool pending;           // Breakpoint not fully resolved.
    QString fullName;       // Full file name acknowledged by the debugger engine.
    bool multiple;          // Happens in constructors/gdb.
    QByteArray extra;       // gdb: <PENDING>, <MULTIPLE>
    int correctedLineNumber;
};

typedef QList<BreakpointId> BreakpointIds;

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_BREAKPOINT_H
