/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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

#include "analyzerconstants.h"

#include "../debuggermainwindow.h"

#include <coreplugin/id.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>

#include <QCoreApplication>

#include <functional>

QT_BEGIN_NAMESPACE
class QDockWidget;
class QAction;
QT_END_NAMESPACE

namespace Debugger {

/**
 * The mode in which this tool should preferably be run
 *
 * Debugging tools which try to show stack traces as close as possible to what the source code
 * looks like will prefer SymbolsMode. Profiling tools which need optimized code for realistic
 * performance, but still want to show analytical output that depends on debug symbols, will prefer
 * ProfileMode.
 */
enum ToolMode {
    DebugMode     = 0x1,
    ProfileMode   = 0x2,
    ReleaseMode   = 0x4,
    SymbolsMode   = DebugMode   | ProfileMode,
    OptimizedMode = ProfileMode | ReleaseMode,
    AnyMode       = DebugMode   | ProfileMode | ReleaseMode
};
Q_DECLARE_FLAGS(ToolModes, ToolMode)

/**
 * This class represents an analyzation action, i.e. a tool that runs in a specific mode.
 *
*/

class DEBUGGER_EXPORT ActionDescription
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::AnalyzerAction)

public:
    ActionDescription() {}

    Core::Id menuGroup() const { return m_menuGroup; }
    void setMenuGroup(Core::Id menuGroup) { m_menuGroup = menuGroup; }

    QByteArray perspectiveId() const { return m_perspectiveId; }
    void setPerspectiveId(const QByteArray &id) { m_perspectiveId = id; }
    void setToolMode(QFlags<ToolMode> mode) { m_toolMode = mode; }

    Core::Id runMode() const { return m_runMode; }
    void setRunMode(Core::Id mode) { m_runMode = mode; }

    /// Returns a new engine for the given start parameters.
    /// Called each time the tool is launched.
    typedef std::function<ProjectExplorer::RunControl *(ProjectExplorer::RunConfiguration *runConfiguration,
                                                        Core::Id runMode)> RunControlCreator;
    RunControlCreator runControlCreator() const { return m_runControlCreator; }
    void setRunControlCreator(const RunControlCreator &creator) { m_runControlCreator = creator; }

    typedef std::function<bool()> ToolPreparer;
    ToolPreparer toolPreparer() const { return m_toolPreparer; }
    void setToolPreparer(const ToolPreparer &toolPreparer) { m_toolPreparer = toolPreparer; }

    void startTool() const;

    /// This is only used for setups not using the startup project.
    typedef std::function<void(ProjectExplorer::RunConfiguration *runConfiguration)> ToolStarter;
    void setCustomToolStarter(const ToolStarter &toolStarter) { m_customToolStarter = toolStarter; }

    QString toolTip() const { return m_toolTip; }
    void setToolTip(const QString &toolTip) { m_toolTip = toolTip; }

    QString text() const { return m_text; }
    void setText(const QString &text) { m_text = text; }

private:
    QString m_text;
    QString m_toolTip;
    Core::Id m_menuGroup;
    QByteArray m_perspectiveId;
    QFlags<ToolMode> m_toolMode = AnyMode;
    Core::Id m_runMode;
    RunControlCreator m_runControlCreator;
    ToolStarter m_customToolStarter;
    ToolPreparer m_toolPreparer;
};

// FIXME: Merge with something sensible.

// Register a tool for a given start mode.
DEBUGGER_EXPORT void registerAction(Core::Id actionId, const ActionDescription &desc, QAction *startAction = 0);
DEBUGGER_EXPORT void registerPerspective(const QByteArray &perspectiveId, const Utils::Perspective *perspective);
DEBUGGER_EXPORT void registerToolbar(const QByteArray &perspectiveId, const Utils::ToolbarDescription &desc);

DEBUGGER_EXPORT void enableMainWindow(bool on);
DEBUGGER_EXPORT QWidget *mainWindow();

DEBUGGER_EXPORT void selectPerspective(const QByteArray &perspectiveId);
DEBUGGER_EXPORT QByteArray currentPerspective();

// Convenience functions.
DEBUGGER_EXPORT void showStatusMessage(const QString &message, int timeoutMS = 10000);
DEBUGGER_EXPORT void showPermanentStatusMessage(const QString &message);

DEBUGGER_EXPORT QAction *createStartAction();
DEBUGGER_EXPORT QAction *createStopAction();

DEBUGGER_EXPORT ProjectExplorer::RunControl *createAnalyzerRunControl(
    ProjectExplorer::RunConfiguration *runConfiguration, Core::Id runMode);

} // namespace Debugger
