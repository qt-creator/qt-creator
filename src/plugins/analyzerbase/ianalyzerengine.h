/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef IANALYZERENGINE_H
#define IANALYZERENGINE_H

#include "analyzerbase_global.h"
#include "analyzerstartparameters.h"

#include <projectexplorer/task.h>
#include <utils/outputformat.h>

#include <QObject>
#include <QString>

namespace ProjectExplorer {
class RunConfiguration;
}

namespace Analyzer {

class IAnalyzerTool;

/**
 * An IAnalyzerEngine instance handles the launch of an analyzation tool.
 *
 * It gets created for each launch and deleted when the launch is stopped or ended.
 */
class ANALYZER_EXPORT IAnalyzerEngine : public QObject
{
    Q_OBJECT

public:
    IAnalyzerEngine(IAnalyzerTool *tool, const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration = 0);
    IAnalyzerEngine(IAnalyzerTool *tool,
        ProjectExplorer::RunConfiguration *runConfiguration);

    /// Start analyzation process.
    virtual bool start() = 0;
    /// Trigger async stop of the analyzation process.
    virtual void stop() = 0;

    /// Controller actions.
    virtual bool canPause() const { return false; }
    virtual void pause() {}
    virtual void unpause() {}

    /// The active run configuration for this engine, might be zero.
    ProjectExplorer::RunConfiguration *runConfiguration() const { return m_runConfig; }

    /// The start parameters for this engine.
    const AnalyzerStartParameters &startParameters() const { return m_sp; }

    /// The tool this engine is associated with.
    IAnalyzerTool *tool() const { return m_tool; }
    StartMode mode() const { return m_sp.startMode; }

signals:
    /// Should be emitted when the debuggee outputted something.
    void outputReceived(const QString &, Utils::OutputFormat format);
    /// Can be emitted when you want to show a task, e.g. to display an error.
    void taskToBeAdded(ProjectExplorer::Task::TaskType type, const QString &description,
                       const QString &file, int line);

    /// Must be emitted when the engine finished.
    void finished();
    /// Must be emitted when the engine is starting.
    void starting(const Analyzer::IAnalyzerEngine *);

private:
    ProjectExplorer::RunConfiguration *m_runConfig;
    AnalyzerStartParameters m_sp;
    IAnalyzerTool *m_tool;
};

} // namespace Analyzer

#endif // IANALYZERENGINE_H
