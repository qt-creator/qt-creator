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

#ifndef ANALYZERMANAGER_H
#define ANALYZERMANAGER_H

#include "analyzerbase_global.h"
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

namespace ProjectExplorer { class RunConfiguration; }

namespace Analyzer {

class AnalyzerRunControl;

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

class ANALYZER_EXPORT ActionDescription
{
    Q_DECLARE_TR_FUNCTIONS(Analyzer::AnalyzerAction)

public:
    ActionDescription() {}

    Core::Id menuGroup() const { return m_menuGroup; }
    void setMenuGroup(Core::Id menuGroup) { m_menuGroup = menuGroup; }

    Core::Id actionId() const { return m_actionId; }
    void setActionId(Core::Id id) { m_actionId = id; }

    Core::Id perspectiveId() const { return m_perspective; }
    void setPerspectiveId(Core::Id id) { m_perspective = id; }
    void setToolMode(QFlags<ToolMode> mode) { m_toolMode = mode; }

    Core::Id runMode() const { return m_runMode; }
    void setRunMode(Core::Id mode) { m_runMode = mode; }
    bool isRunnable(QString *reason = 0) const;

    /// Creates all widgets used by the tool.
    /// Returns a control widget which will be shown in the status bar when
    /// this tool is selected.
    typedef std::function<QWidget *()> WidgetCreator;
    QWidget *createWidget() const { return m_widgetCreator(); }
    void setWidgetCreator(const WidgetCreator &creator) { m_widgetCreator = creator; }

    /// Returns a new engine for the given start parameters.
    /// Called each time the tool is launched.
    typedef std::function<AnalyzerRunControl *(ProjectExplorer::RunConfiguration *runConfiguration,
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

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

private:
    bool m_enabled = false;
    QString m_text;
    QString m_toolTip;
    Core::Id m_menuGroup;
    Core::Id m_actionId;
    Core::Id m_perspective;
    QFlags<ToolMode> m_toolMode = AnyMode;
    Core::Id m_runMode;
    WidgetCreator m_widgetCreator;
    RunControlCreator m_runControlCreator;
    ToolStarter m_customToolStarter;
    ToolPreparer m_toolPreparer;
};

// FIXME: Merge with AnalyzerPlugin.
class ANALYZER_EXPORT AnalyzerManager : public QObject
{
    Q_OBJECT

public:
    explicit AnalyzerManager(QObject *parent);
    ~AnalyzerManager();

    static void shutdown();

    // Register a tool for a given start mode.
    static void addAction(const ActionDescription &desc);
    static void addPerspective(const Perspective &perspective);

    // Dockwidgets are registered to the main window.
    static QDockWidget *createDockWidget(QWidget *widget, Core::Id dockId);

    static void enableMainWindow(bool on);

    static void showMode();
    static void selectAction(Core::Id actionId, bool alsoRunIt = false);
    static void stopTool();

    // Convenience functions.
    static void showStatusMessage(Core::Id perspective, const QString &message, int timeoutMS = 10000);
    static void showPermanentStatusMessage(Core::Id perspective, const QString &message);

    static void handleToolStarted();
    static void handleToolFinished();
    static QAction *stopAction();

    static AnalyzerRunControl *createRunControl(
        ProjectExplorer::RunConfiguration *runConfiguration, Core::Id runMode);
};

} // namespace Analyzer

#endif // ANALYZERMANAGER_H
