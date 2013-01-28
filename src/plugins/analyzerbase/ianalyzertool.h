/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef IANALYZERTOOL_H
#define IANALYZERTOOL_H

#include "analyzerbase_global.h"
#include "analyzerconstants.h"
#include "analyzerstartparameters.h"

#include <coreplugin/id.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QObject>

namespace ProjectExplorer {
class RunConfiguration;
}

namespace Analyzer {

class IAnalyzerOutputPaneAdapter;
class IAnalyzerEngine;
class AbstractAnalyzerSubConfig;


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

    /// Returns a unique ID for this tool.
    virtual Core::Id id() const = 0;
    /// Returns the run mode for this tool.
    virtual ProjectExplorer::RunMode runMode() const = 0;
    /// Returns a short user readable display name for this tool.
    virtual QString displayName() const = 0;
    /// Returns a user readable description name for this tool.
    virtual QString description() const = 0;
    /// Returns an id for the start action.
    virtual Core::Id actionId(StartMode mode) const
        { return defaultActionId(this, mode); }
    /// Returns the menu group the start action should go to.
    virtual Core::Id menuGroup(StartMode mode) const
        { return defaultMenuGroup(mode); }
    /// Returns a short user readable action name for this tool.
    virtual QString actionName(StartMode mode) const
        { return defaultActionName(this, mode); }

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

    /// Convenience implementation.
    static Core::Id defaultMenuGroup(StartMode mode);
    static Core::Id defaultActionId(const IAnalyzerTool *tool, StartMode mode);
    static QString defaultActionName(const IAnalyzerTool *tool, StartMode mode);

    /// This gets called after all analyzation tools where initialized.
    virtual void extensionsInitialized() = 0;

    /// Creates all widgets used by the tool.
    /// Returns a control widget which will be shown in the status bar when
    /// this tool is selected. Must be non-zero.
    virtual QWidget *createWidgets() = 0;

    /// Returns a new engine for the given start parameters.
    /// Called each time the tool is launched.
    virtual IAnalyzerEngine *createEngine(const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration = 0) = 0;

    /// Returns true if the tool can be run
    virtual bool canRun(ProjectExplorer::RunConfiguration *runConfiguration,
                        ProjectExplorer::RunMode mode) const = 0;

    /// Create the start parameters for the run control factory
    virtual AnalyzerStartParameters createStartParameters(
            ProjectExplorer::RunConfiguration *runConfiguration,
            ProjectExplorer::RunMode mode) const = 0;

    virtual void startTool(StartMode mode) = 0;

    /// Called when tools gets selected.
    virtual void toolSelected() const {}

    /// Called when tools gets deselected.
    virtual void toolDeselected() const {}

    /// Factory method to create the global tool setting
    virtual AbstractAnalyzerSubConfig *createGlobalSettings();

    /// Factory method to create the project tool setting
    virtual AbstractAnalyzerSubConfig *createProjectSettings();
};

} // namespace Analyzer

#endif // IANALYZERTOOL_H
