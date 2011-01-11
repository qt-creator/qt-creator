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
    void removeSessionData();

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

    static QIcon breakpointIcon();
    static QIcon disabledBreakpointIcon();
    static QIcon pendingBreakpointIcon();
    static QIcon emptyIcon();
    static QIcon watchpointIcon();
    static QIcon tracepointIcon();

    BreakpointId findBreakpointByFileAndLine(const QString &fileName,
        int lineNumber, bool useMarkerPosition = true);
    BreakpointId findBreakpointByAddress(quint64 address) const;

    void breakByFunction(const QString &functionName);
    void removeBreakpoint(BreakpointId id);
    QIcon icon(BreakpointId id) const;
    void gotoLocation(BreakpointId id) const;

    // Getter retrieves property value.
    // Setter sets property value and triggers update if changed.
    bool useFullPath(BreakpointId id) const;
    void setUseFullPath(BreakpointId, const bool &on);
    QByteArray condition(BreakpointId id) const;
    void setCondition(BreakpointId, const QByteArray &condition);
    int ignoreCount(BreakpointId id) const;
    void setIgnoreCount(BreakpointId, const int &count);
    int threadSpec(BreakpointId id) const;
    void setThreadSpec(BreakpointId, const int&spec);
    QString fileName(BreakpointId id) const;
    void setFileName(BreakpointId, const QString &fileName);
    QString functionName(BreakpointId id) const;
    void setFunctionName(BreakpointId, const QString &functionName);
    BreakpointType type(BreakpointId id) const;
    void setType(BreakpointId id, const BreakpointType &type);
    quint64 address(BreakpointId id) const;
    void setAddress(BreakpointId id, const quint64 &address);
    int lineNumber(BreakpointId id) const;
    void setBreakpointData(BreakpointId id, const BreakpointParameters &data);
    const BreakpointParameters &breakpointData(BreakpointId id) const;
    BreakpointState state(BreakpointId id) const;
    bool isEnabled(BreakpointId id) const;
    void setEnabled(BreakpointId id, bool on);
    void updateLineNumberFromMarker(BreakpointId id, int lineNumber);
    void setMarkerFileAndLine(BreakpointId id,
        const QString &fileName, int lineNumber);
    bool isTracepoint(BreakpointId id) const;
    void setTracepoint(BreakpointId, bool on);
    DebuggerEngine *engine(BreakpointId id) const;
    void setEngine(BreakpointId id, DebuggerEngine *engine);
    const BreakpointResponse &response(BreakpointId id) const;
    void setResponse(BreakpointId id, const BreakpointResponse &data);
    bool needsChange(BreakpointId id) const;

    // State transitions.
    void notifyBreakpointChangeAfterInsertNeeded(BreakpointId id);
    void notifyBreakpointInsertProceeding(BreakpointId id);
    void notifyBreakpointInsertOk(BreakpointId id);
    void notifyBreakpointInsertFailed(BreakpointId id);
    void notifyBreakpointChangeOk(BreakpointId id);
    void notifyBreakpointChangeProceeding(BreakpointId id);
    void notifyBreakpointChangeFailed(BreakpointId id);
    void notifyBreakpointPending(BreakpointId id);
    void notifyBreakpointRemoveProceeding(BreakpointId id);
    void notifyBreakpointRemoveOk(BreakpointId id);
    void notifyBreakpointRemoveFailed(BreakpointId id);
    void notifyBreakpointReleased(BreakpointId id);
    void notifyBreakpointNeedsReinsertion(BreakpointId id);
    void notifyBreakpointAdjusted(BreakpointId id,
            const BreakpointParameters &data);

    static QString displayFromThreadSpec(int spec);
    static int threadSpecFromDisplay(const QString &str);

private:
    // QAbstractItemModel implementation.
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    void setState(BreakpointId id, BreakpointState state);
    void loadBreakpoints();
    void saveBreakpoints();
    void updateMarker(BreakpointId id);
    void cleanupBreakpoint(BreakpointId id);

    struct BreakpointItem
    {
        BreakpointItem();

        void destroyMarker();
        bool needsChange() const;
        bool isLocatedAt(const QString &fileName, int lineNumber,
            bool useMarkerPosition) const;
        QString toToolTip() const;
        QString markerFileName() const;
        int markerLineNumber() const;
        QIcon icon() const;

        BreakpointParameters data;
        BreakpointState state;   // Current state of breakpoint.
        DebuggerEngine *engine;  // Engine currently handling the breakpoint.
        BreakpointResponse response;
        BreakpointMarker *marker;
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
