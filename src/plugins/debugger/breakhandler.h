/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <utils/qtcassert.h>

#include <QtCore/QObject>
#include <QtCore/QAbstractTableModel>

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
    // Making it copiable is tricky because of the markers.
    void operator=(const BreakpointData &);
    BreakpointData(const BreakpointData &);

    // Our owner
    BreakHandler *m_handler; // not owned.

public:
    bool enabled;            // should we talk to the debugger engine?
    bool pending;            // does the debugger engine know about us already?

    // this "user requested information". will get stored in the session
    QString fileName;        // short name of source file
    QByteArray condition;    // condition associated with breakpoint
    QByteArray ignoreCount;  // ignore count associated with breakpoint
    QByteArray lineNumber;   // line in source file
    QString funcName;        // name of containing function
    bool useFullPath;        // should we use the full path when setting the bp?

    // this is what gdb produced in response
    QByteArray bpNumber;     // breakpoint number assigned by the debugger engine
    QByteArray bpCondition;  // condition acknowledged by the debugger engine
    QByteArray bpIgnoreCount;// ignore count acknowledged by the debugger engine
    QString bpFileName;      // file name acknowledged by the debugger engine
    QByteArray bpLineNumber; // line number acknowledged by the debugger engine
    QString bpFuncName;      // function name acknowledged by the debugger engine
    QString bpAddress;       // address acknowledged by the debugger engine
    bool    bpMultiple;      // happens in constructors/gdb
    bool    bpEnabled;       // enable/disable command sent

    // taken from either user input or gdb responses
    QString markerFileName; // used to locate the marker
    int markerLineNumber;

    // our red blob in the editor
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

    BreakpointData *at(int index) const { QTC_ASSERT(index < size(), return 0); return m_bp.at(index); }
    int size() const { return m_bp.size(); }
    bool hasPendingBreakpoints() const;
    void append(BreakpointData *data);
    void removeAt(int index); // also deletes the marker
    void clear(); // also deletes all the marker
    int indexOf(BreakpointData *data) { return m_bp.indexOf(data); }
    int findBreakpoint(const QString &fileName, int lineNumber);
    int findBreakpoint(const BreakpointData &data); // returns index
    int findBreakpoint(int bpNumber); // returns index
    void updateMarkers();

    QList<BreakpointData *> insertedBreakpoints() const;
    void takeInsertedBreakPoint(BreakpointData *);
    QList<BreakpointData *> takeRemovedBreakpoints(); // owned
    QList<BreakpointData *> takeEnabledBreakpoints(); // not owned
    QList<BreakpointData *> takeDisabledBreakpoints(); // not owned

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

    DebuggerManager *m_manager; // not owned
    QList<BreakpointData *> m_bp;
    QList<BreakpointData *> m_inserted; // lately inserted breakpoints
    QList<BreakpointData *> m_removed; // lately removed breakpoints
    QList<BreakpointData *> m_enabled; // lately enabled breakpoints
    QList<BreakpointData *> m_disabled; // lately disabled breakpoints
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_BREAKHANDLER_H
