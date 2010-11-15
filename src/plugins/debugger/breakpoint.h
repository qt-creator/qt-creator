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

typedef quint64 BreakpointId; // FIXME: make Internal.

namespace Internal {

class BreakWindow;
class BreakpointDialog;
class BreakHandler;
class BreakpointData;

QDataStream &operator>>(QDataStream& stream, BreakpointData &data);

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
    BreakpointPending,
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

    BreakpointType type;     // Type of breakpoint.
    bool enabled;            // Should we talk to the debugger engine?
    bool useFullPath;        // Should we use the full path when setting the bp?
    QString fileName;        // Short name of source file.
    QByteArray condition;    // Condition associated with breakpoint.
    int ignoreCount;         // Ignore count associated with breakpoint.
    int lineNumber;          // Line in source file.
    quint64 address;         // Address for watchpoints.
    QByteArray threadSpec;   // Thread specification.
    QString functionName;
};

inline bool operator==(const BreakpointParameters &p1, const BreakpointParameters &p2)
{ return p1.equals(p2); }
inline bool operator!=(const BreakpointParameters &p1, const BreakpointParameters &p2)
{ return !p1.equals(p2); }

class BreakpointData
{
private:
    friend class BreakHandler; // This should be the only class manipulating data.
    friend class BreakWindow; // FIXME: Remove.
    friend class BreakpointDialog; // FIXME: Remove.
    friend QDataStream &operator>>(QDataStream& stream, BreakpointData &data);

public:
    explicit BreakpointData(BreakpointType type = UnknownType);

    BreakpointType type() const { return m_parameters.type; }
    quint64 address() const { return m_parameters.address; }
    bool useFullPath() const { return m_parameters.useFullPath; }
    QString toString() const;

    bool conditionsMatch(const QByteArray &other) const;
    QString functionName() const { return m_parameters.functionName; }
    QString fileName() const { return m_parameters.fileName; }
    int lineNumber() const { return m_parameters.lineNumber; }
    int ignoreCount() const { return m_parameters.ignoreCount; }
    bool isEnabled() const { return m_parameters.enabled; }
    QByteArray threadSpec() const { return m_parameters.threadSpec; }
    QByteArray condition() const { return m_parameters.condition; }
    const BreakpointParameters &parameters() const { return m_parameters; }

    bool isWatchpoint() const { return type() == Watchpoint; }
    bool isBreakpoint() const { return type() != Watchpoint; } // Enough for now.

private:
    // All setters return true on change.
    bool setUseFullPath(bool on);
    bool setMarkerFileName(const QString &file);
    bool setMarkerLineNumber(int line);
    bool setFileName(const QString &file);
    bool setEnabled(bool on);
    bool setIgnoreCount(int count);
    bool setFunctionName(const QString &name);
    bool setLineNumber(int line);
    bool setAddress(quint64 address);
    bool setThreadSpec(const QByteArray &spec);
    bool setType(BreakpointType type);
    bool setCondition(const QByteArray &cond);

private:
    // This "user requested information" will get stored in the session.
    BreakpointParameters m_parameters;
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
    QString fullName;       // Full file name acknowledged by the debugger engine.
    bool multiple;          // Happens in constructors/gdb.
    QByteArray state;       // gdb: <PENDING>, <MULTIPLE>
};

typedef QList<BreakpointId> BreakpointIds;

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_BREAKPOINT_H
