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

#include "breakpoint.h"

#include <QtCore/QObject>
#include <QtCore/QAbstractTableModel>

#include <QtGui/QIcon>

//////////////////////////////////////////////////////////////////
//
// BreakHandler
//
//////////////////////////////////////////////////////////////////

namespace Debugger {

class DebuggerEngine;

namespace Internal {

class BreakpointMarker;

class BreakHandler : public QAbstractTableModel
{
    Q_OBJECT

public:
    BreakHandler();
    ~BreakHandler();

    void loadSessionData();
    void saveSessionData();

    QAbstractItemModel *model() { return this; }

    // The only way to add a new breakpoint.
    void appendBreakpoint(const BreakpointParameters &data);

    BreakpointIds allBreakpointIds() const;
    BreakpointIds engineBreakpointIds(DebuggerEngine *engine) const;
    BreakpointIds unclaimedBreakpointIds() const;
    int size() const { return m_storage.size(); }

    // Find a breakpoint matching approximately the data in needle.
    BreakpointId findSimilarBreakpoint(const BreakpointResponse &needle) const;
    BreakpointId findBreakpointByNumber(int bpNumber) const;
    BreakpointId findWatchpointByAddress(quint64 address) const;
    BreakpointId findBreakpointByFunction(const QString &functionName) const;
    BreakpointId findBreakpointByIndex(const QModelIndex &index) const;
    BreakpointIds findBreakpointsByIndex(const QList<QModelIndex> &list) const;
    void setWatchpointByAddress(quint64 address);
    bool hasWatchpointAt(quint64 address) const;
    void updateMarkers();

    QIcon breakpointIcon() const { return m_breakpointIcon; }
    QIcon disabledBreakpointIcon() const { return m_disabledBreakpointIcon; }
    QIcon pendingBreakPointIcon() const { return m_pendingBreakPointIcon; }
    QIcon emptyIcon() const { return m_emptyIcon; }

    void toggleBreakpoint(const QString &fileName, int lineNumber, quint64 address = 0);
    BreakpointId findBreakpointByFileAndLine(const QString &fileName,
        int lineNumber, bool useMarkerPosition = true);
    BreakpointId findBreakpointByAddress(quint64 address) const;

    void breakByFunction(const QString &functionName);
    void removeBreakpoint(BreakpointId id);
    QIcon icon(BreakpointId id) const;
    void gotoLocation(BreakpointId id) const;

    // Getter retrieves property value.
    // Setter sets property value and triggers update if changed.
    #define PROPERTY(type, getter, setter) \
        type getter(BreakpointId id) const; \
        void setter(BreakpointId id, const type &value);

    PROPERTY(bool, useFullPath, setUseFullPath)
    PROPERTY(QByteArray, condition, setCondition)
    PROPERTY(int, ignoreCount, setIgnoreCount)
    PROPERTY(QByteArray, threadSpec, setThreadSpec)
    PROPERTY(QString, fileName, setFileName)
    PROPERTY(QString, functionName, setFunctionName)
    PROPERTY(BreakpointType, type, setType);
    PROPERTY(quint64, address, setAddress);
    PROPERTY(int, lineNumber, setLineNumber);
    #undef PROPERTY
    void setBreakpointData(BreakpointId id, const BreakpointParameters &data);
    const BreakpointParameters &breakpointData(BreakpointId id) const;
    BreakpointState state(BreakpointId id) const;
    bool isEnabled(BreakpointId id) const;
    void setEnabled(BreakpointId id, bool on);
    void updateLineNumberFromMarker(BreakpointId id, int lineNumber);
    void setMarkerFileAndLine(BreakpointId id,
        const QString &fileName, int lineNumber);
    DebuggerEngine *engine(BreakpointId id) const;
    void setEngine(BreakpointId id, DebuggerEngine *engine);
    const BreakpointResponse &response(BreakpointId id) const;
    void setResponse(BreakpointId id, const BreakpointResponse &data);

    // Incorporate debugger feedback. No synchronization request needed.
    void ackCondition(BreakpointId id);
    void ackIgnoreCount(BreakpointId id);
    void ackEnabled(BreakpointId id);

    // State transitions.
    void notifyBreakpointInsertProceeding(BreakpointId id);
    void notifyBreakpointInsertOk(BreakpointId id);
    void notifyBreakpointInsertFailed(BreakpointId id);
    void notifyBreakpointChangeOk(BreakpointId id);
    void notifyBreakpointChangeFailed(BreakpointId id);
    void notifyBreakpointPending(BreakpointId id);
    void notifyBreakpointRemoveProceeding(BreakpointId id);
    void notifyBreakpointRemoveOk(BreakpointId id);
    void notifyBreakpointRemoveFailed(BreakpointId id);
    void notifyBreakpointReleased(BreakpointId id);
    void notifyBreakpointAdjusted(BreakpointId id,
            const BreakpointParameters &data);

private:
public:
    // FIXME: Make private.
    void setState(BreakpointId id, BreakpointState state);

private:
    friend class BreakpointMarker;

    // QAbstractItemModel implementation.
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    void markerUpdated(BreakpointMarker *marker, int lineNumber);
    void loadBreakpoints();
    void saveBreakpoints();
    void updateMarker(BreakpointId id);
    void cleanupBreakpoint(BreakpointId id);

    const QIcon m_breakpointIcon;
    const QIcon m_disabledBreakpointIcon;
    const QIcon m_pendingBreakPointIcon;
    const QIcon m_emptyIcon;
    const QIcon m_watchpointIcon;

    struct BreakpointItem
    {
        BreakpointItem();

        void destroyMarker();
        bool isPending() const { return response.pending; }
        bool needsChange() const;
        bool isLocatedAt(const QString &fileName, int lineNumber,
            bool useMarkerPosition) const;
        QString toToolTip() const;

        BreakpointParameters data;
        BreakpointState state;   // Current state of breakpoint.
        DebuggerEngine *engine;  // Engine currently handling the breakpoint.
        BreakpointResponse response;
        BreakpointMarker *marker;
        QString markerFileName; // Used to locate the marker.
        int markerLineNumber;
    };
    typedef QHash<BreakpointId, BreakpointItem> BreakpointStorage;
    typedef BreakpointStorage::ConstIterator ConstIterator;
    typedef BreakpointStorage::Iterator Iterator;
    BreakpointStorage m_storage;

    void scheduleSynchronization();
    void timerEvent(QTimerEvent *event);
    int m_syncTimerId;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_BREAKHANDLER_H
