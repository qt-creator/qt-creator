/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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

//! \enum Debugger::Internal::BreakpointType
enum BreakpointType
{
    UnknownType,
    BreakpointByFileAndLine,
    BreakpointByFunction,
    BreakpointByAddress,
    BreakpointAtThrow,
    BreakpointAtCatch,
    BreakpointAtMain,
    Watchpoint
};

//! \enum Debugger::Internal::BreakpointState
enum BreakpointState
{
    BreakpointNew,
    BreakpointInsertRequested,  //!< Inferior was told about bp, not ack'ed.
    BreakpointInsertProceeding,
    BreakpointChangeRequested,
    BreakpointChangeProceeding,
    BreakpointInserted,
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

class BreakpointParameters
{
public:
    explicit BreakpointParameters(BreakpointType = UnknownType);
    bool equals(const BreakpointParameters &rhs) const;
    bool conditionsMatch(const QByteArray &other) const;
    bool isWatchpoint() const { return type == Watchpoint; }
    // Enough for now.
    bool isBreakpoint() const { return type != Watchpoint && !tracepoint; }
    bool isTracepoint() const { return tracepoint; }
    QString toString() const;

    bool operator==(const BreakpointParameters &p) const { return equals(p); }
    bool operator!=(const BreakpointParameters &p) const { return !equals(p); }

    BreakpointType type;     //!< Type of breakpoint.
    bool enabled;            //!< Should we talk to the debugger engine?
    BreakpointPathUsage pathUsage;  //!< Should we use the full path when setting the bp?
    QString fileName;        //!< Short name of source file.
    QByteArray condition;    //!< Condition associated with breakpoint.
    int ignoreCount;         //!< Ignore count associated with breakpoint.
    int lineNumber;          //!< Line in source file.
    quint64 address;         //!< Address for watchpoints.
    uint size;               //!< Size of watched area for watchpoints.
    uint bitpos;             //!< Location of watched bitfield within watched area.
    uint bitsize;            //!< Size of watched bitfield within watched area.
    int threadSpec;          //!< Thread specification.
    QString functionName;
    QString module;          //!< module for file name
    QString command;         //!< command to execute
    bool tracepoint;
};

class BreakpointResponse : public BreakpointParameters
{
public:
    BreakpointResponse();
    QString toString() const;

public:
    void fromParameters(const BreakpointParameters &p);

    int number;             //!< Breakpoint number assigned by the debugger engine.
    bool pending;           //!< Breakpoint not fully resolved.
    QString fullName;       //!< Full file name acknowledged by the debugger engine.
    bool multiple;          //!< Happens in constructors/gdb.
    QByteArray extra;       //!< gdb: <PENDING>, <MULTIPLE>
    QList<quint64> addresses;//!< Extra addresses for templated code.
    int correctedLineNumber; //!< Line number as seen by gdb.
};

typedef QList<BreakpointId> BreakpointIds;

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_BREAKPOINT_H
