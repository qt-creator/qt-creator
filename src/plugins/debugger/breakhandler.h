/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "breakpoint.h"
#include "debuggerprotocol.h"

#include <utils/fileutils.h>
#include <utils/treemodel.h>

#include <QCoreApplication>
#include <QPointer>

namespace Utils { class ItemViewEvent; }

namespace Debugger {
namespace Internal {

class BreakpointItem;
class BreakpointMarker;
class BreakHandler;
class DebuggerCommand;
class DebuggerEngine;
class BreakpointManager;
class GlobalBreakpointMarker;

class SubBreakpointItem : public QObject, public Utils::TypedTreeItem<Utils::TreeItem, BreakpointItem>
{
public:
    QVariant data(int column, int role) const final;

    BreakpointItem *breakpoint() const { return Utils::TypedTreeItem<Utils::TreeItem, BreakpointItem>::parent(); }
    void setParameters(const BreakpointParameters &pars) { params = pars; }
    BreakpointParameters params;
    QString responseId;      //!< Breakpoint number assigned by the debugger engine.
    QString displayName;      //!< Breakpoint number assigned by the debugger engine.
};

using SubBreakpoint = QPointer<SubBreakpointItem>;

class GlobalBreakpointItem : public QObject, public Utils::TreeItem
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::BreakHandler)

public:
    explicit GlobalBreakpointItem();
    ~GlobalBreakpointItem() override;

    QVariant data(int column, int role) const override;
    QIcon icon() const;

    void deleteBreakpoint();
    void removeBreakpointFromModel();

    void updateLineNumber(int lineNumber);
    void updateFileName(const Utils::FilePath &fileName);

    QString displayName() const;
    Utils::FilePath markerFileName() const;
    QString toolTip() const;
    int markerLineNumber() const;
    int modelId() const;

    bool isEnabled() const { return m_params.enabled; }
    void setEnabled(bool enabled, bool descend = true);

    const BreakpointParameters &requestedParameters() const { return m_params; }

private:
    friend class BreakHandler;
    friend class BreakpointManager;
    friend class BreakpointMarker;
    friend class DebuggerEngine;

    void updateMarker();
    void updateMarkerIcon();
    void destroyMarker();
//    void scheduleSynchronization();
    QPointer<DebuggerEngine> usingEngine() const;

    bool isEngineRunning() const;

    const int m_modelId;
    BreakpointParameters m_params;
    GlobalBreakpointMarker *m_marker = nullptr; // The primary marker set by the user.
};

using GlobalBreakpoint = QPointer<GlobalBreakpointItem>;
using GlobalBreakpoints = QList<GlobalBreakpoint>;

class BreakpointItem final : public QObject, public Utils::TypedTreeItem<SubBreakpointItem>
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::BreakHandler)

public:
    explicit BreakpointItem(const GlobalBreakpoint &gbp);
    ~BreakpointItem() final;

    QVariant data(int column, int role) const final;

    QIcon icon() const;

    void setMarkerFileAndLine(const Utils::FilePath &fileName, int lineNumber);
    bool needsChange() const;

    SubBreakpoint findOrCreateSubBreakpoint(const QString &responseId);
    Utils::FilePath markerFileName() const;
    int markerLineNumber() const;

    const BreakpointParameters &requestedParameters() const;
    void addToCommand(DebuggerCommand *cmd) const;
    void updateFromGdbOutput(const GdbMi &bkpt);

    int modelId() const;
    QString responseId() const { return m_responseId; }
    QString displayName() const { return m_displayName; }
    QString toolTip() const;
    QString shortToolTip() const;
    BreakpointState state() const { return m_state; }
    BreakpointType type() const { return m_parameters.type; }
    BreakpointPathUsage pathUsage() const;
    const BreakpointParameters parameters() const { return m_parameters; }

    QString condition() const { return m_parameters.condition; }
    int ignoreCount() const { return m_parameters.ignoreCount; }
    int threadSpec() const { return m_parameters.threadSpec; }
    Utils::FilePath fileName() const { return m_parameters.fileName; }
    QString functionName() const { return m_parameters.functionName; }
    QString expression() const { return m_parameters.expression; }
    QString message() const { return m_parameters.message; }
    QString command() const { return m_parameters.command; }
    quint64 address() const { return m_parameters.address; }
    int lineNumber() const { return m_parameters.lineNumber; }
    bool isEnabled() const { return m_parameters.enabled; }
    bool isWatchpoint() const { return m_parameters.isWatchpoint(); }
    bool isTracepoint() const { return m_parameters.isTracepoint(); }
    bool isOneShot() const { return m_parameters.oneShot; }
    bool isPending() const { return m_parameters.pending; }

    void setLineNumber(int lineNumber) { m_parameters.lineNumber = lineNumber; }
    void setFileName(const Utils::FilePath &fileName) { m_parameters.fileName = fileName; }
    void setFunctionName(const QString &functionName) { m_parameters.functionName = functionName; }
    void setPending(bool pending);
    void setResponseId(const QString &str) { m_responseId = str; }
    void setDisplayName(const QString &name) { m_displayName = name; }
    void setParameters(const BreakpointParameters &value);
    void setAddress(quint64 address) { m_parameters.address = address; }
    void setEnabled(bool on);
    void setHitCount(int hitCount) { m_parameters.hitCount = hitCount; }
    void setThreadSpec(int threadSpec) { m_parameters.threadSpec = threadSpec; }
    void setIgnoreCount(int count) { m_parameters.ignoreCount = count; }
    void setCommand(const QString &command) { m_parameters.command = command; }
    void setCondition(const QString &condition) { m_parameters.condition = condition; }

    QString msgWatchpointByAddressTriggered(quint64 address) const;
    QString msgWatchpointByAddressTriggered(quint64 address, const QString &threadId) const;
    QString msgWatchpointByExpressionTriggered(const QString &expr) const;
    QString msgWatchpointByExpressionTriggered(const QString &expr, const QString &threadId) const;
    QString msgBreakpointTriggered(const QString &threadId) const;

    friend class BreakpointManager;
    friend class BreakHandler;
    friend class DebuggerEngine;

    void adjustMarker();

    void deleteBreakpoint();
    void deleteGlobalOrThisBreakpoint();

    void updateLineNumber(int lineNumber);
    void updateFileName(const Utils::FilePath &fileName);

    const GlobalBreakpoint globalBreakpoint() const;
    void gotoState(BreakpointState target, BreakpointState assumedCurrent);

private:
    void destroyMarker();
    void updateMarker();
    void updateMarkerIcon();
    void setState(BreakpointState state);

    const GlobalBreakpoint m_globalBreakpoint; // Origin, or null for aliens.
    BreakpointParameters m_parameters;         // Parameters acknowledged by engine.
    BreakpointParameters m_alienParameters;    // Requested parameters in case of no associated global bp.
    BreakpointState m_state = BreakpointNew;   // Current state of breakpoint.
    BreakpointMarker *m_marker = nullptr;
    QString m_responseId;      //!< Breakpoint number or id assigned by or used in the debugger backend.
    QString m_displayName;
};

using Breakpoint = QPointer<BreakpointItem>;
using Breakpoints = const QList<Breakpoint>;
using SubBreakpoints = const QList<SubBreakpoint>;

using BreakHandlerModel = Utils::TreeModel<Utils::TypedTreeItem<BreakpointItem>, BreakpointItem, SubBreakpointItem>;
using BreakpointManagerModel = Utils::TreeModel<Utils::TypedTreeItem<GlobalBreakpointItem>, GlobalBreakpointItem>;

inline uint qHash(const Debugger::Internal::SubBreakpoint &b) { return qHash(b.data()); }
inline uint qHash(const Debugger::Internal::Breakpoint &b) { return qHash(b.data()); }
inline uint qHash(const Debugger::Internal::GlobalBreakpoint &b) { return qHash(b.data()); }

class BreakHandler : public BreakHandlerModel
{
    Q_OBJECT

public:
    explicit BreakHandler(DebuggerEngine *engine);

    QAbstractItemModel *model() { return this; }
    const Breakpoints breakpoints() const;

    bool tryClaimBreakpoint(const GlobalBreakpoint &gbp);
    void releaseAllBreakpoints();

    void handleAlienBreakpoint(const QString &responseId, const BreakpointParameters &response);
    void removeAlienBreakpoint(const QString &responseId);
    void requestBreakpointInsertion(const Breakpoint &bp);
    void requestBreakpointUpdate(const Breakpoint &bp);
    void requestBreakpointRemoval(const Breakpoint &bp);
    void requestBreakpointEnabling(const Breakpoint &bp, bool enabled);
    void requestSubBreakpointEnabling(const SubBreakpoint &sbp, bool enabled);

    void removeBreakpoint(const Breakpoint &bp);
    void editBreakpoint(const Breakpoint &bp, QWidget *parent);

    Breakpoint findBreakpointByResponseId(const QString &responseId) const;
    SubBreakpoint findSubBreakpointByResponseId(const QString &responseId) const;
    Breakpoint findWatchpoint(const BreakpointParameters &data) const;
    Breakpoint findBreakpointByModelId(int modelId) const;

    static QString displayFromThreadSpec(int spec);
    static int threadSpecFromDisplay(const QString &str);

    // Convenience.
    void setWatchpointAtAddress(quint64 address, unsigned size);
    void setWatchpointAtExpression(const QString &exp);

    void updateDisassemblerMarker(const Breakpoint &bp);
    void removeDisassemblerMarker(const Breakpoint &bp);

private:
    Breakpoint findBreakpointByIndex(const QModelIndex &index) const;
    Breakpoints findBreakpointsByIndex(const QList<QModelIndex> &list) const;
    SubBreakpoint findSubBreakpointByIndex(const QModelIndex &index) const;
    SubBreakpoints findSubBreakpointsByIndex(const QList<QModelIndex> &list) const;
    void editBreakpoints(const Breakpoints &bps, QWidget *parent);

    void gotoState(Breakpoint bp, BreakpointState target, BreakpointState assumedCurrent);
    void gotoLocation(const Breakpoint &bp) const;

    QVariant data(const QModelIndex &idx, int role) const final;
    bool setData(const QModelIndex &idx, const QVariant &value, int role) final;

    bool contextMenuEvent(const Utils::ItemViewEvent &ev);

    friend class BreakpointItem;

    DebuggerEngine * const m_engine;
};

class BreakpointManager : public BreakpointManagerModel
{
    Q_OBJECT

public:
    BreakpointManager();

    static QAbstractItemModel *model();

    static const GlobalBreakpoints globalBreakpoints();

    static GlobalBreakpoint createBreakpoint(const BreakpointParameters &data);
    static GlobalBreakpoint findBreakpointFromContext(const ContextData &location);

    static void claimBreakpointsForEngine(DebuggerEngine *engine);
    static void toggleBreakpoint(const ContextData &location, const QString &tracePointMessage = QString());
    static void createBreakpointForEngine(const BreakpointParameters &data, DebuggerEngine *engine);
    static void editBreakpoint(const GlobalBreakpoint &gbp, QWidget *parent);

    static void executeAddBreakpointDialog();
    static void executeDeleteAllBreakpointsDialog();

private:
    static GlobalBreakpoint createBreakpointHelper(const BreakpointParameters &data);
    static GlobalBreakpoint findBreakpointByIndex(const QModelIndex &index);
    static GlobalBreakpoints findBreakpointsByIndex(const QList<QModelIndex> &list);

    QVariant data(const QModelIndex &idx, int role) const final;
    bool setData(const QModelIndex &idx, const QVariant &value, int role) final;

    void loadSessionData();
    void saveSessionData();

    bool contextMenuEvent(const Utils::ItemViewEvent &ev);
    void gotoLocation(const GlobalBreakpoint &gbp) const;
    void editBreakpoints(const GlobalBreakpoints &gbps, QWidget *parent);
};

} // namespace Internal
} // namespace Debugger

