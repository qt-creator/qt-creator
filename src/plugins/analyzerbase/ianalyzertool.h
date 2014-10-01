/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef IANALYZERTOOL_H
#define IANALYZERTOOL_H

#include "analyzerbase_global.h"
#include "analyzerstartparameters.h"

#include <coreplugin/id.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QObject>
#include <QAction>

namespace ProjectExplorer { class RunConfiguration; }

namespace Analyzer {

class AnalyzerRunControl;


/**
 * This class represents an analyzation tool, e.g. "Valgrind Memcheck".
 *
 * Each tool can run in different run modes. The modes are specific to the mode.
 *
 * @code
 * bool YourPlugin::initialize(const QStringList &arguments, QString *errorString)
 * {
 *    AnalyzerManager::addTool(new MemcheckTool(this));
 *    return true;
 * }
 * @endcode
 */
class ANALYZER_EXPORT IAnalyzerTool : public QObject
{
    Q_OBJECT

public:
    explicit IAnalyzerTool(QObject *parent = 0);

    /// Returns the run mode for this tool.
    virtual ProjectExplorer::RunMode runMode() const = 0;

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
    virtual ToolMode toolMode() const = 0;

    /// Creates all widgets used by the tool.
    /// Returns a control widget which will be shown in the status bar when
    /// this tool is selected. Must be non-zero.
    virtual QWidget *createWidgets() = 0;

    /// Returns a new engine for the given start parameters.
    /// Called each time the tool is launched.
    virtual AnalyzerRunControl *createRunControl(const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration) = 0;

    virtual void startTool(StartMode mode) = 0;
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
    IAnalyzerTool *tool() const { return m_tool; }
    void setTool(IAnalyzerTool *tool) { m_tool = tool; }

    StartMode startMode() const { return m_startMode; }
    void setStartMode(StartMode startMode) { m_startMode = startMode; }

    Core::Id menuGroup() const { return m_menuGroup; }
    void setMenuGroup(Core::Id menuGroup) { m_menuGroup = menuGroup; }

    Core::Id id() const { return m_id; }
    void setId(Core::Id id) { m_id = id; }

protected:
    IAnalyzerTool *m_tool;
    StartMode m_startMode;
    Core::Id m_menuGroup;
    Core::Id m_id;
};


} // namespace Analyzer

#endif // IANALYZERTOOL_H
