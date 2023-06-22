// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "breakpoint.h"
#include "debugger_global.h"
#include "debuggerprotocol.h"

#include <utils/filepath.h>
#include <utils/treemodel.h>

#include <QPointer>

namespace Utils { class ItemViewEvent; }

namespace Debugger::Internal {

class BreakHandler;
class BreakpointItem;
class BreakpointManager;
class BreakpointMarker;
class DebuggerCommand;
class DebuggerEngine;
class GlobalBreakpointMarker;
class Location;

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
    void setParameters(const BreakpointParameters &params);

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
public:
    explicit BreakpointItem(const GlobalBreakpoint &gbp);
    ~BreakpointItem() final;

    QVariant data(int column, int role) const final;

    QIcon icon(bool withLocationMarker = false) const;

    void setMarkerFileAndPosition(const Utils::FilePath &fileName,
                                  const Utils::Text::Position &textPosition);
    bool needsChange() const;

    SubBreakpoint findOrCreateSubBreakpoint(const QString &responseId);
    Utils::FilePath markerFileName() const;
    int markerLineNumber() const;

    const BreakpointParameters &requestedParameters() const;
    void addToCommand(DebuggerCommand *cmd,
                      BreakpointPathUsage defaultPathUsage
                      = BreakpointPathUsage::BreakpointUseFullPath) const;
    void updateFromGdbOutput(const GdbMi &bkpt, const Utils::FilePath &fileRoot);

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
    Utils::Text::Position textPosition() const { return m_parameters.textPosition; }
    bool isEnabled() const { return m_parameters.enabled; }
    bool isWatchpoint() const { return m_parameters.isWatchpoint(); }
    bool isTracepoint() const { return m_parameters.isTracepoint(); }
    bool isOneShot() const { return m_parameters.oneShot; }
    bool isPending() const { return m_parameters.pending; }

    void setTextPosition(const Utils::Text::Position pos) { m_parameters.textPosition = pos; }
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
    void setMessage(const QString& message) { m_parameters.message = message; }

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

    void setNeedsLocationMarker(bool needsLocationMarker);

private:
    void destroyMarker();
    void updateMarker();
    void setState(BreakpointState state);

    const GlobalBreakpoint m_globalBreakpoint; // Origin, or null for aliens.
    BreakpointParameters m_parameters;         // Parameters acknowledged by engine.
    BreakpointParameters m_alienParameters;    // Requested parameters in case of no associated global bp.
    BreakpointState m_state = BreakpointNew;   // Current state of breakpoint.
    BreakpointMarker *m_marker = nullptr;
    QString m_responseId;      //!< Breakpoint number or id assigned by or used in the debugger backend.
    QString m_displayName;
    bool m_needsLocationMarker = false;
};

using Breakpoint = QPointer<BreakpointItem>;
using Breakpoints = const QList<Breakpoint>;
using SubBreakpoints = const QList<SubBreakpoint>;

using BreakHandlerModel = Utils::TreeModel<Utils::TypedTreeItem<BreakpointItem>, BreakpointItem, SubBreakpointItem>;
using BreakpointManagerModel = Utils::TreeModel<Utils::TypedTreeItem<GlobalBreakpointItem>, GlobalBreakpointItem>;

inline auto qHash(const Debugger::Internal::SubBreakpoint &b) { return qHash(b.data()); }
inline auto qHash(const Debugger::Internal::Breakpoint &b) { return qHash(b.data()); }
inline auto qHash(const Debugger::Internal::GlobalBreakpoint &b) { return qHash(b.data()); }

class BreakHandler : public BreakHandlerModel
{
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

    void setLocation(const Location &loc);
    void resetLocation();

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

class DEBUGGER_EXPORT BreakpointManager : public BreakpointManagerModel
{
public:
    BreakpointManager();

    static QAbstractItemModel *model();

    static const GlobalBreakpoints globalBreakpoints();

    static GlobalBreakpoint createBreakpoint(const BreakpointParameters &data);
    static GlobalBreakpoint findBreakpointFromContext(const ContextData &location);

    static void claimBreakpointsForEngine(DebuggerEngine *engine);
    static void setOrRemoveBreakpoint(const ContextData &location, const QString &tracePointMessage = {});
    static void enableOrDisableBreakpoint(const ContextData &location);
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

} // Debugger::Internal
