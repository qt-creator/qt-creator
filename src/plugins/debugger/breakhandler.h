/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_BREAKHANDLER_H
#define DEBUGGER_BREAKHANDLER_H

#include "breakpoint.h"

#include <QObject>
#include <QAbstractTableModel>

#include <QIcon>

//////////////////////////////////////////////////////////////////
//
// BreakHandler
//
//////////////////////////////////////////////////////////////////

namespace Debugger {

class DebuggerEngine;

namespace Internal {

class BreakpointMarker;

class BreakHandler : public QAbstractItemModel
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
    void handleAlienBreakpoint(const BreakpointResponse &response, DebuggerEngine *engine);
    void insertSubBreakpoint(BreakpointModelId id, const BreakpointResponse &data);
    void removeAlienBreakpoint(BreakpointModelId id);

    BreakpointModelIds allBreakpointIds() const;
    BreakpointModelIds engineBreakpointIds(DebuggerEngine *engine) const;
    BreakpointModelIds unclaimedBreakpointIds() const;
    int size() const { return m_storage.size(); }
    QStringList engineBreakpointPaths(DebuggerEngine *engine) const;

    // Find a breakpoint matching approximately the data in needle.
    BreakpointModelId findSimilarBreakpoint(const BreakpointResponse &needle) const;
    BreakpointModelId findBreakpointByResponseId(const BreakpointResponseId &resultId) const;
    BreakpointModelId findWatchpoint(const BreakpointParameters &data) const;
    BreakpointModelId findBreakpointByFunction(const QString &functionName) const;
    BreakpointModelId findBreakpointByIndex(const QModelIndex &index) const;
    BreakpointModelIds findBreakpointsByIndex(const QList<QModelIndex> &list) const;
    void updateMarkers();

    static QIcon breakpointIcon();
    static QIcon disabledBreakpointIcon();
    static QIcon pendingBreakpointIcon();
    static QIcon emptyIcon();
    static QIcon watchpointIcon();
    static QIcon tracepointIcon();

    BreakpointModelId findBreakpointByFileAndLine(const QString &fileName,
        int lineNumber, bool useMarkerPosition = true);
    BreakpointModelId findBreakpointByAddress(quint64 address) const;

    void breakByFunction(const QString &functionName);
    void removeBreakpoint(BreakpointModelId id);
    QIcon icon(BreakpointModelId id) const;
    void gotoLocation(BreakpointModelId id) const;

    // Getter retrieves property value.
    // Setter sets property value and triggers update if changed.
    BreakpointPathUsage pathUsage(BreakpointModelId id) const;
    void setPathUsage(BreakpointModelId, const BreakpointPathUsage &u);
    QByteArray condition(BreakpointModelId id) const;
    void setCondition(BreakpointModelId, const QByteArray &condition);
    int ignoreCount(BreakpointModelId id) const;
    void setIgnoreCount(BreakpointModelId, const int &count);
    int threadSpec(BreakpointModelId id) const;
    void setThreadSpec(BreakpointModelId, const int &spec);
    QString fileName(BreakpointModelId id) const;
    void setFileName(BreakpointModelId, const QString &fileName);
    QString functionName(BreakpointModelId id) const;
    void setFunctionName(BreakpointModelId, const QString &functionName);
    QString expression(BreakpointModelId id) const;
    void setExpression(BreakpointModelId, const QString &expression);
    QString message(BreakpointModelId id) const;
    void setMessage(BreakpointModelId, const QString &m);
    BreakpointType type(BreakpointModelId id) const;
    void setType(BreakpointModelId id, const BreakpointType &type);
    quint64 address(BreakpointModelId id) const;
    void setAddress(BreakpointModelId id, const quint64 &address);
    int lineNumber(BreakpointModelId id) const;
    void changeBreakpointData(BreakpointModelId id, const BreakpointParameters &data,
        BreakpointParts parts);
    const BreakpointParameters &breakpointData(BreakpointModelId id) const;
    BreakpointState state(BreakpointModelId id) const;
    bool isEnabled(BreakpointModelId id) const;
    void setEnabled(BreakpointModelId id, bool on);
    void updateFileNameFromMarker(BreakpointModelId id, const QString &fileName);
    void updateLineNumberFromMarker(BreakpointModelId id, int lineNumber);
    void changeLineNumberFromMarker(BreakpointModelId id, int lineNumber);
    void setMarkerFileAndLine(BreakpointModelId id,
        const QString &fileName, int lineNumber);
    bool isOneShot(BreakpointModelId id) const;
    bool isWatchpoint(BreakpointModelId id) const;
    bool isTracepoint(BreakpointModelId id) const;
    void setTracepoint(BreakpointModelId, bool on);
    DebuggerEngine *engine(BreakpointModelId id) const;
    void setEngine(BreakpointModelId id, DebuggerEngine *engine);
    const BreakpointResponse &response(BreakpointModelId id) const;
    void setResponse(BreakpointModelId id, const BreakpointResponse &data);
    bool needsChange(BreakpointModelId id) const;
    bool needsChildren(BreakpointModelId id) const;

    // State transitions.
    void notifyBreakpointChangeAfterInsertNeeded(BreakpointModelId id);
    void notifyBreakpointInsertProceeding(BreakpointModelId id);
    void notifyBreakpointInsertOk(BreakpointModelId id);
    void notifyBreakpointInsertFailed(BreakpointModelId id);
    void notifyBreakpointChangeOk(BreakpointModelId id);
    void notifyBreakpointChangeProceeding(BreakpointModelId id);
    void notifyBreakpointChangeFailed(BreakpointModelId id);
    void notifyBreakpointPending(BreakpointModelId id);
    void notifyBreakpointRemoveProceeding(BreakpointModelId id);
    void notifyBreakpointRemoveOk(BreakpointModelId id);
    void notifyBreakpointRemoveFailed(BreakpointModelId id);
    void notifyBreakpointReleased(BreakpointModelId id);
    void notifyBreakpointNeedsReinsertion(BreakpointModelId id);
    void notifyBreakpointAdjusted(BreakpointModelId id,
            const BreakpointParameters &data);

    static QString displayFromThreadSpec(int spec);
    static int threadSpecFromDisplay(const QString &str);

    // Convenience.
    void setWatchpointAtAddress(quint64 address, unsigned size);
    void setWatchpointAtExpression(const QString &exp);

private:
    // QAbstractItemModel implementation.
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QModelIndex index(int row, int col, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &parent) const;
    QModelIndex createIndex(int row, int column, quint32 id) const;
    QModelIndex createIndex(int row, int column, void *ptr) const;

    int indexOf(BreakpointModelId id) const;
    BreakpointModelId at(int index) const;
    bool isEngineRunning(BreakpointModelId id) const;
    void setState(BreakpointModelId id, BreakpointState state);
    void loadBreakpoints();
    void saveBreakpoints();
    void cleanupBreakpoint(BreakpointModelId id);
    Q_SLOT void changeLineNumberFromMarkerHelper(Debugger::Internal::BreakpointModelId id, int lineNumber);

    struct BreakpointItem
    {
        BreakpointItem();

        void destroyMarker();
        bool needsChange() const;
        bool isLocatedAt(const QString &fileName, int lineNumber,
            bool useMarkerPosition) const;
        void updateMarker(BreakpointModelId id);
        void updateMarkerIcon();
        QString toToolTip() const;
        QString markerFileName() const;
        int markerLineNumber() const;
        QIcon icon() const;

        BreakpointParameters data;
        BreakpointState state;   // Current state of breakpoint.
        DebuggerEngine *engine;  // Engine currently handling the breakpoint.
        BreakpointResponse response;
        BreakpointMarker *marker;
        QList<BreakpointResponse> subItems;
    };
    typedef QHash<BreakpointModelId, BreakpointItem> BreakpointStorage;
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
