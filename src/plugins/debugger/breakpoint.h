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

#include <QtCore/QString>

namespace Debugger {
class DebuggerManager;
namespace Internal {

class BreakpointMarker;
class BreakHandler;

//////////////////////////////////////////////////////////////////
//
// BreakpointData
//
//////////////////////////////////////////////////////////////////

class BreakpointData
{
public:
    BreakpointData();
    ~BreakpointData();

    void removeMarker();
    void updateMarker();
    QString toToolTip() const;
    QString toString() const;
    BreakHandler *handler() { return m_handler; }

    bool isLocatedAt(const QString &fileName, int lineNumber) const;
    bool conditionsMatch() const;

private:
    // Intentionally unimplemented.
    // Making it copyable is tricky because of the markers.
    void operator=(const BreakpointData &);
    BreakpointData(const BreakpointData &);

    // Our owner
    BreakHandler *m_handler; // Not owned.
    friend class BreakHandler;

public:
    enum Type { BreakpointType, WatchpointType };

    bool enabled;            // Should we talk to the debugger engine?
    bool pending;            // Does the debugger engine know about us already?
    Type type;               // Type of breakpoint.

    // This "user requested information" will get stored in the session.
    QString fileName;        // Short name of source file.
    QByteArray condition;    // Condition associated with breakpoint.
    QByteArray ignoreCount;  // Ignore count associated with breakpoint.
    QByteArray lineNumber;   // Line in source file.
    QByteArray address;      // Address for watchpoints.
    QByteArray threadSpec;   // Thread specification.
    QString funcName;        // Name of containing function.
    bool useFullPath;        // Should we use the full path when setting the bp?

    // This is what gdb produced in response.
    QByteArray bpNumber;     // Breakpoint number assigned by the debugger engine.
    QByteArray bpCondition;  // Condition acknowledged by the debugger engine.
    QByteArray bpIgnoreCount;// Ignore count acknowledged by the debugger engine.
    QString bpFileName;      // File name acknowledged by the debugger engine.
    QString bpFullName;      // Full file name acknowledged by the debugger engine.
    QByteArray bpLineNumber; // Line number acknowledged by the debugger engine.
    QByteArray bpCorrectedLineNumber; // Acknowledged by the debugger engine.
    QByteArray bpThreadSpec; // Thread spec acknowledged by the debugger engine.
    QString bpFuncName;      // Function name acknowledged by the debugger engine.
    QByteArray bpAddress;    // Address acknowledged by the debugger engine.
    bool bpMultiple;         // Happens in constructors/gdb.
    bool bpEnabled;          // Enable/disable command sent.

    void setMarkerFileName(const QString &fileName);
    QString markerFileName() const { return m_markerFileName; }

    void setMarkerLineNumber(int lineNumber);
    int markerLineNumber() const { return m_markerLineNumber; }

private:
    // Taken from either user input or gdb responses.
    QString m_markerFileName; // Used to locate the marker.
    int m_markerLineNumber;

    // Our red blob in the editor.
    BreakpointMarker *marker;
};


} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_BREAKPOINT_H
