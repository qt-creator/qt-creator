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

#ifndef DEBUGGER_BREAKHANDLER_H
#define DEBUGGER_BREAKHANDLER_H

#include <QtCore/QObject>
#include <QtCore/QAbstractTableModel>
#include <QtGui/QIcon>

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
    explicit BreakpointData(BreakHandler *handler);
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

public:
    bool enabled;            // Should we talk to the debugger engine?
    bool pending;            // Does the debugger engine know about us already?

    // This "user requested information" will get stored in the session.
    QString fileName;        // Short name of source file.
    QByteArray condition;    // Condition associated with breakpoint.
    QByteArray ignoreCount;  // Ignore count associated with breakpoint.
    QByteArray lineNumber;   // Line in source file.
    QString funcName;        // Name of containing function.
    bool useFullPath;        // Should we use the full path when setting the bp?

    // This is what gdb produced in response.
    QByteArray bpNumber;     // Breakpoint number assigned by the debugger engine.
    QByteArray bpCondition;  // Condition acknowledged by the debugger engine.
    QByteArray bpIgnoreCount;// Ignore count acknowledged by the debugger engine.
    QString bpFileName;      // File name acknowledged by the debugger engine.
    QByteArray bpLineNumber; // Line number acknowledged by the debugger engine.
    QByteArray bpCorrectedLineNumber; // Acknowledged by the debugger engine.
    QString bpFuncName;      // Function name acknowledged by the debugger engine.
    QByteArray bpAddress;    // Address acknowledged by the debugger engine.
    bool    bpMultiple;      // Happens in constructors/gdb.
    bool    bpEnabled;       // Enable/disable command sent.

    // Taken from either user input or gdb responses.
    QString markerFileName; // Used to locate the marker.
    int markerLineNumber;

    // Our red blob in the editor.
    BreakpointMarker *marker;
};


//////////////////////////////////////////////////////////////////
//
// BreakHandler
//
//////////////////////////////////////////////////////////////////

class BreakHandler : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit BreakHandler(DebuggerManager *manager, QObject *parent = 0);
    ~BreakHandler();

    void removeAllBreakpoints();
    void setAllPending();
    void loadSessionData();
    void saveSessionData();

    QAbstractItemModel *model() { return this; }

    BreakpointData *at(int index) const;
    int size() const { return m_bp.size(); }
    bool hasPendingBreakpoints() const;
    void append(BreakpointData *data);
    void removeAt(int index); // This also deletes the marker.
    void clear(); // This also deletes all the marker.
    int indexOf(BreakpointData *data) { return m_bp.indexOf(data); }
    int findBreakpoint(const QString &fileName, int lineNumber);
    int findBreakpoint(const BreakpointData &data); // Returns index.
    int findBreakpoint(int bpNumber); // Returns index.
    void updateMarkers();

    QList<BreakpointData *> insertedBreakpoints() const;
    void takeInsertedBreakPoint(BreakpointData *);
    QList<BreakpointData *> takeRemovedBreakpoints(); // Owned.
    QList<BreakpointData *> takeEnabledBreakpoints(); // Not owned.
    QList<BreakpointData *> takeDisabledBreakpoints(); // Not owned.

    QIcon breakpointIcon() const         { return m_breakpointIcon; }
    QIcon disabledBreakpointIcon() const { return m_disabledBreakpointIcon; }
    QIcon pendingBreakPointIcon() const  { return m_pendingBreakPointIcon; }

public slots:
    void setBreakpoint(const QString &fileName, int lineNumber);
    void toggleBreakpointEnabled(BreakpointData *data);
    void toggleBreakpointEnabled(const QString &fileName, int lineNumber);
    void breakByFunction(const QString &functionName);
    void activateBreakpoint(int index);
    void removeBreakpoint(int index);

private:
    friend class BreakpointMarker;

    // QAbstractItemModel
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    void markerUpdated(BreakpointMarker *, int lineNumber);
    void loadBreakpoints();
    void saveBreakpoints();
    void resetBreakpoints();
    void removeBreakpointHelper(int index);

    const QIcon m_breakpointIcon;
    const QIcon m_disabledBreakpointIcon;
    const QIcon m_pendingBreakPointIcon;

    DebuggerManager *m_manager; // Not owned.
    QList<BreakpointData *> m_bp;
    QList<BreakpointData *> m_inserted; // Lately inserted breakpoints.
    QList<BreakpointData *> m_removed; // Lately removed breakpoints.
    QList<BreakpointData *> m_enabled; // Lately enabled breakpoints.
    QList<BreakpointData *> m_disabled; // Lately disabled breakpoints.
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_BREAKHANDLER_H
