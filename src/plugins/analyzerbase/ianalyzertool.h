/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#ifndef IANALYZERTOOL_H
#define IANALYZERTOOL_H

#include "analyzerbase_global.h"
#include "analyzerconstants.h"

#include <coreplugin/id.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QObject>
#include <QAction>

#include <functional>

namespace ProjectExplorer { class RunConfiguration; }

namespace Analyzer {

class AnalyzerStartParameters;
class AnalyzerRunControl;

/**
 * The mode in which this tool should preferably be run
 *
 * The memcheck tool, for example, requires debug symbols, hence DebugMode
 * is preferred. On the other hand, callgrind should look at optimized code,
 * hence ReleaseMode.
 */
enum ToolMode {
    DebugMode,
    ReleaseMode,
    AnyMode
};

/**
 * This class represents an analyzation action, i.e. a tool that runs in a specific mode.
 *
*/

class ANALYZER_EXPORT AnalyzerAction : public QAction
{
    Q_OBJECT

public:
    explicit AnalyzerAction(QObject *parent = 0);

public:
    Core::Id menuGroup() const { return m_menuGroup; }
    void setMenuGroup(Core::Id menuGroup) { m_menuGroup = menuGroup; }

    Core::Id actionId() const { return m_actionId; }
    void setActionId(Core::Id id) { m_actionId = id; }

    Core::Id toolId() const { return m_toolId; }
    void setToolId(Core::Id id) { m_toolId = id; }
    void setToolMode(ToolMode mode) { m_toolMode = mode; }

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
    typedef std::function<AnalyzerRunControl *(const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration)> RunControlCreator;
    RunControlCreator runControlCreator() const { return m_runControlCreator; }
    void setRunControlCreator(const RunControlCreator &creator) { m_runControlCreator = creator; }

    typedef std::function<bool()> ToolPreparer;
    ToolPreparer toolPreparer() const { return m_toolPreparer; }
    void setToolPreparer(const ToolPreparer &toolPreparer) { m_toolPreparer = toolPreparer; }

    void startTool();

    /// This is only used for setups not using the startup project.
    typedef std::function<void()> ToolStarter;
    void setCustomToolStarter(const ToolStarter &toolStarter) { m_customToolStarter = toolStarter; }

protected:
    Core::Id m_menuGroup;
    Core::Id m_actionId;
    Core::Id m_toolId;
    ToolMode m_toolMode;
    Core::Id m_runMode;
    WidgetCreator m_widgetCreator;
    RunControlCreator m_runControlCreator;
    ToolStarter m_customToolStarter;
    ToolPreparer m_toolPreparer;
};

} // namespace Analyzer

#endif // IANALYZERTOOL_H
