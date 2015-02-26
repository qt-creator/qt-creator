/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_BREAKHANDLER_H
#define DEBUGGER_BREAKHANDLER_H

#include "breakpoint.h"
#include "debuggerprotocol.h"

#include <utils/treemodel.h>

#include <QCoreApplication>
#include <QPointer>

//////////////////////////////////////////////////////////////////
//
// BreakHandler
//
//////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

class BreakpointItem;
class BreakHandler;
class DebuggerCommand;
class DebuggerEngine;

// Non-owning "deletion-safe" wrapper around a BreakpointItem *
class Breakpoint
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::BreakHandler)

public:
    Breakpoint() {}

    bool isValid() const;
    operator const void *() const { return isValid() ? this : 0; }
    bool operator!() const { return !isValid(); }

    uint hash() const;

    const BreakpointParameters &parameters() const;
    void addToCommand(DebuggerCommand *cmd) const;

    BreakpointModelId id() const;
    bool isLocatedAt(const QString &fileName, int lineNumber,
                     bool useMarkerPosition) const;

    QIcon icon() const;
    BreakpointState state() const;
    void setEngine(DebuggerEngine *engine);

    // State transitions.
    void notifyBreakpointChangeAfterInsertNeeded();
    void notifyBreakpointInsertProceeding();
    void notifyBreakpointInsertOk();
    void notifyBreakpointInsertFailed();
    void notifyBreakpointChangeOk();
    void notifyBreakpointChangeProceeding();
    void notifyBreakpointChangeFailed();
    void notifyBreakpointPending();
    void notifyBreakpointRemoveProceeding();
    void notifyBreakpointRemoveOk();
    void notifyBreakpointRemoveFailed();
    void notifyBreakpointReleased();
    void notifyBreakpointNeedsReinsertion();
    void notifyBreakpointAdjusted(const BreakpointParameters &params);

    void update();

    void gotoLocation() const;

    // Getter retrieves property value.
    // Setter sets property value and triggers update if changed.
    // Only use setters when it is safe to assume that the breakpoint still
    // exist. That's not the case if the event loop could run after you
    // obtained the BreakpointItem pointer.
    BreakpointPathUsage pathUsage() const;
    void setPathUsage(const BreakpointPathUsage &u);
    QByteArray condition() const;
    void setCondition(const QByteArray &condition);
    int ignoreCount() const;
    void setIgnoreCount(const int &count);
    int threadSpec() const;
    void setThreadSpec(const int &spec);
    QString fileName() const;
    void setFileName(const QString &fileName);
    QString functionName() const;
    void setFunctionName(const QString &functionName);
    QString expression() const;
    void setExpression(const QString &expression);
    QString message() const;
    void setMessage(const QString &m);
    BreakpointType type() const;
    void setType(const BreakpointType &type);
    quint64 address() const;
    void setAddress(const quint64 &address);
    int lineNumber() const;
    void changeBreakpointData(const BreakpointParameters &data);
    bool isEnabled() const;
    void setEnabled(bool on) const;
    void updateFileNameFromMarker(const QString &fileName);
    void updateLineNumberFromMarker(int lineNumber);
    void changeLineNumberFromMarker(int lineNumber);
    void setMarkerFileAndLine(const QString &fileName, int lineNumber);
    bool isWatchpoint() const;
    bool isTracepoint() const;
    void setTracepoint(bool on);
    DebuggerEngine *engine() const;
    const BreakpointResponse &response() const;
    void setResponse(const BreakpointResponse &data);
    bool needsChange() const;
    bool needsChildren() const;

    bool isOneShot() const;
    void insertSubBreakpoint(const BreakpointResponse &data);
    void removeAlienBreakpoint();
    void removeBreakpoint() const;

    QString msgWatchpointByAddressTriggered(int number, quint64 address) const;
    QString msgWatchpointByAddressTriggered(
        int number, quint64 address, const QString &threadId) const;
    QString msgWatchpointByExpressionTriggered(int number, const QString &expr) const;
    QString msgWatchpointByExpressionTriggered(
        int number, const QString &expr, const QString &threadId) const;
    QString msgBreakpointTriggered(int number, const QString &threadId) const;

private:
    void gotoState(BreakpointState target, BreakpointState assumedCurrent);

    friend class BreakHandler;
    explicit Breakpoint(BreakpointItem *b);

    QPointer<BreakpointItem> b;
};

inline uint qHash(const Debugger::Internal::Breakpoint &b) { return b.hash(); }

typedef QList<Breakpoint> Breakpoints;

class BreakHandler : public Utils::TreeModel
{
    Q_OBJECT

public:
    BreakHandler();

    void loadSessionData();
    void saveSessionData();

    QAbstractItemModel *model() { return this; }

    // The only way to add a new breakpoint.
    void appendBreakpoint(const BreakpointParameters &data);
    void handleAlienBreakpoint(const BreakpointResponse &response, DebuggerEngine *engine);

    Breakpoints allBreakpoints() const;
    Breakpoints engineBreakpoints(DebuggerEngine *engine) const;
    Breakpoints unclaimedBreakpoints() const;
    QStringList engineBreakpointPaths(DebuggerEngine *engine) const;

    // Find a breakpoint matching approximately the data in needle.
    Breakpoint findSimilarBreakpoint(const BreakpointResponse &needle) const;
    Breakpoint findBreakpointByResponseId(const BreakpointResponseId &resultId) const;
    Breakpoint findWatchpoint(const BreakpointParameters &data) const;
    Breakpoint findBreakpointByFunction(const QString &functionName) const;
    Breakpoint findBreakpointByIndex(const QModelIndex &index) const;
    Breakpoints findBreakpointsByIndex(const QList<QModelIndex> &list) const;
    void updateMarkers();

    static QIcon breakpointIcon();
    static QIcon disabledBreakpointIcon();
    static QIcon pendingBreakpointIcon();
    static QIcon emptyIcon();
    static QIcon watchpointIcon();
    static QIcon tracepointIcon();

    Breakpoint findBreakpointByFileAndLine(const QString &fileName,
        int lineNumber, bool useMarkerPosition = true);
    Breakpoint findBreakpointByAddress(quint64 address) const;

    void breakByFunction(const QString &functionName);
    static QString displayFromThreadSpec(int spec);
    static int threadSpecFromDisplay(const QString &str);

    // Convenience.
    void setWatchpointAtAddress(quint64 address, unsigned size);
    void setWatchpointAtExpression(const QString &exp);

    Breakpoint breakpointById(BreakpointModelId id) const;

private:
    friend class BreakpointItem;
    friend class Breakpoint;

    void loadBreakpoints();
    void saveBreakpoints();

    void appendBreakpointInternal(const BreakpointParameters &data);

    Q_SLOT void changeLineNumberFromMarkerHelper(Debugger::Internal::BreakpointModelId id);
    Q_SLOT void deletionHelper(Debugger::Internal::BreakpointModelId id);

    void scheduleSynchronization();
    void timerEvent(QTimerEvent *event);
    int m_syncTimerId;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::Breakpoint)

#endif // DEBUGGER_BREAKHANDLER_H
