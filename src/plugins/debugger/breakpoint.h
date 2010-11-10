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

#include <QtCore/QMetaType>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QCoreApplication>

namespace Debugger {

class DebuggerEngine;

typedef quint64 BreakpointId; // FIXME: make Internal.

namespace Internal {

class BreakpointMarker;
class BreakHandler;

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
    Watchpoint,
};

enum BreakpointState
{
    BreakpointNew,
    BreakpointInsertRequested,  // Inferior was told about bp, not ack'ed.
    BreakpointInsertProceeding,
    BreakpointChangeRequested,
    BreakpointChangeProceeding,
    BreakpointPending,
    BreakpointInserted,
    BreakpointRemoveRequested,
    BreakpointRemoveProceeding,
    BreakpointDead,
};

class BreakpointData
{
private:

    // Intentionally unimplemented.
    // Making it copyable is tricky because of the markers.
    BreakpointData(const BreakpointData &);
    void operator=(const BreakpointData &);

    friend class BreakHandler;

public:
    BreakpointData();

    bool isPending() const { return m_state == BreakpointPending
        || m_state == BreakpointNew; }
    BreakpointType type() const { return m_type; }
    quint64 address() const { return m_address; }
    bool useFullPath() const { return m_useFullPath; }
    QString toToolTip() const;
    QString toString() const;

    bool isLocatedAt(const QString &fileName, int lineNumber,
        bool useMarkerPosition) const;
    bool conditionsMatch(const QString &other) const;
    BreakpointState state() const { return m_state; }
    QString functionName() const { return m_functionName; }
    QString markerFileName() const { return m_markerFileName; }
    QString fileName() const { return m_fileName; }
    int markerLineNumber() const { return m_markerLineNumber; }
    int lineNumber() const { return m_lineNumber; }
    int ignoreCount() const { return m_ignoreCount; }
    bool isEnabled() const { return m_enabled; }
    QByteArray threadSpec() const { return m_threadSpec; }
    QByteArray condition() const { return m_condition; }
    DebuggerEngine *engine() const { return m_engine; }

    bool isWatchpoint() const { return m_type == Watchpoint; }
    bool isBreakpoint() const { return m_type != Watchpoint; } // Enough for now.
    // Generic name for function to break on 'throw'
    static const char *throwFunction;
    static const char *catchFunction;

//private:
     // All setters return true on change.
    bool setUseFullPath(bool on);
    bool setMarkerFileName(const QString &file);
    bool setMarkerLineNumber(int line);
    bool setFileName(const QString &file);
    bool setEnabled(bool on);
    bool setIgnoreCount(bool count);
    bool setFunctionName(const QString &name);
    bool setLineNumber(int line);
    bool setAddress(quint64 address);
    bool setThreadSpec(const QByteArray &spec);
    bool setType(BreakpointType type);
    bool setCondition(const QByteArray &cond);
    bool setState(BreakpointState state);
    bool setEngine(DebuggerEngine *engine);

private:
    DebuggerEngine *m_engine;
    BreakpointType m_type;     // Type of breakpoint.
    BreakpointState m_state;   // Current state of breakpoint.
    bool m_enabled;            // Should we talk to the debugger engine?
    bool m_pending;            // Does the debugger engine know about us already?
    bool m_useFullPath;        // Should we use the full path when setting the bp?
    // This "user requested information" will get stored in the session.
    QString m_fileName;        // Short name of source file.
    QByteArray m_condition;    // Condition associated with breakpoint.
    int m_ignoreCount;         // Ignore count associated with breakpoint.
    int m_lineNumber;          // Line in source file.
    quint64 m_address;         // Address for watchpoints.
    QByteArray m_threadSpec;   // Thread specification.
    // Name of containing function, special values:
    // BreakpointData::throwFunction, BreakpointData::catchFunction
    QString m_functionName;
    QString m_markerFileName; // Used to locate the marker.
    int m_markerLineNumber;

public:
    Q_DECLARE_TR_FUNCTIONS(BreakHandler)
};

// This is what debuggers produced in response to the attempt to
// insert a breakpoint. The data might differ from the requested bits.
class BreakpointResponse
{
public:
    BreakpointResponse();
    QString toString() const;

public:
    int bpNumber;             // Breakpoint number assigned by the debugger engine.
    BreakpointType bpType;    // Breakpoint type used by debugger engine.
    QByteArray bpCondition;   // Condition acknowledged by the debugger engine.
    int bpIgnoreCount;        // Ignore count acknowledged by the debugger engine.
    QString bpFileName;       // File name acknowledged by the debugger engine.
    QString bpFullName;       // Full file name acknowledged by the debugger engine.
    int bpLineNumber;         // Line number acknowledged by the debugger engine.
    //int bpCorrectedLineNumber; // Acknowledged by the debugger engine.
    QByteArray bpThreadSpec;  // Thread spec acknowledged by the debugger engine.
    QString bpFuncName;       // Function name acknowledged by the debugger engine.
    quint64 bpAddress;        // Address acknowledged by the debugger engine.
    bool bpMultiple;          // Happens in constructors/gdb.
    bool bpEnabled;           // Enable/disable command sent.
    QByteArray bpState;       // gdb: <PENDING>, <MULTIPLE>
};

typedef QList<BreakpointId> BreakpointIds;

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_BREAKPOINT_H
