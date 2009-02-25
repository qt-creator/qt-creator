/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_BREAKHANDLER_H
#define DEBUGGER_BREAKHANDLER_H

#include <QtCore/QObject>
#include <QtCore/QAbstractItemModel>

namespace Debugger {
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
    BreakHandler *handler() { return m_handler; }

    bool isLocatedAt(const QString &fileName, int lineNumber) const;
    bool conditionsMatch() const;

private:
    // Intentionally unimplemented.
    // Making it copiable is tricky because of the markers.
    void operator=(const BreakpointData &);
    BreakpointData(const BreakpointData &);

    // Our owner
    BreakHandler *m_handler;  // not owned.

public:
    bool pending;           // does the debugger engine know about us already?

    // this "user requested information". will get stored in the session
    QString fileName;       // short name of source file
    QString condition;      // condition associated with breakpoint
    QString ignoreCount;    // ignore count associated with breakpoint
    QString lineNumber;     // line in source file
    QString funcName;       // name of containing function

    // this is what gdb produced in response
    QString bpNumber;       // breakpoint number assigned by the debugger engine
    QString bpCondition;    // condition acknowledged by the debugger engine
    QString bpIgnoreCount;  // ignore count acknowledged by the debugger engine
    QString bpFileName;     // file name acknowledged by the debugger engine
    QString bpLineNumber;   // line number acknowledged by the debugger engine
    QString bpFuncName;     // function name acknowledged by the debugger engine
    QString bpAddress;      // address acknowledged by the debugger engine
    bool    bpMultiple;     // happens in constructors/gdb

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

class BreakHandler : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit BreakHandler(QObject *parent = 0);
    ~BreakHandler();

    void removeAllBreakpoints();
    void setAllPending();
    void loadSessionData();
    void saveSessionData();

    QAbstractItemModel *model() { return this; }

    BreakpointData *at(int index) const { return index < size() ? m_bp.at(index) : 0; }
    int size() const { return m_bp.size(); }
    void append(BreakpointData *data) { m_bp.append(data); }
    void removeAt(int index); // also deletes the marker
    void clear(); // also deletes all the marker
    int indexOf(BreakpointData *data) { return m_bp.indexOf(data); }
    int indexOf(const QString &fileName, int lineNumber);
    int findBreakpoint(const BreakpointData &data); // returns index
    int findBreakpoint(int bpNumber); // returns index
    void updateMarkers();

    QList<BreakpointData *> takeRemovedBreakpoints();

public slots:
    void setBreakpoint(const QString &fileName, int lineNumber);
    void breakByFunction(const QString &functionName);
    void activateBreakPoint(int index);
    void removeBreakpoint(int index);

signals:
    void gotoLocation(const QString &fileName, int lineNumber, bool setMarker);

    void sessionValueRequested(const QString &name, QVariant *value);
    void setSessionValueRequested(const QString &name, const QVariant &value);

private:
    friend class BreakpointMarker;

    // QAbstractItemModel
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &, int role);
    QModelIndex parent(const QModelIndex &) const { return QModelIndex(); }
    QModelIndex index(int row, int column, const QModelIndex &) const
        { return createIndex(row, column); }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    void markerUpdated(BreakpointMarker *, int lineNumber);
    void loadBreakpoints();
    void saveBreakpoints();
    void resetBreakpoints();
    void removeBreakpointHelper(int index);

    QList<BreakpointData *> m_bp;
    QList<BreakpointData *> m_removed;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_BREAKHANDLER_H
