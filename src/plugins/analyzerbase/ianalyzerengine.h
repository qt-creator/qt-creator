/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef IANALYZERENGINE_H
#define IANALYZERENGINE_H

#include "analyzerbase_global.h"
#include "analyzerstartparameters.h"

#include <projectexplorer/task.h>
#include <utils/ssh/sshconnection.h>

#include <QtCore/QObject>
#include <QtCore/QString>

namespace ProjectExplorer {
class RunConfiguration;
}

namespace Analyzer {

/**
 * An IAnalyzerEngine instance handles the launch of an analyzation tool.
 *
 * It gets created for each launch and deleted when the launch is stopped or ended.
 */
class ANALYZER_EXPORT IAnalyzerEngine : public QObject
{
    Q_OBJECT

public:
    explicit IAnalyzerEngine(const AnalyzerStartParameters &sp,
                             ProjectExplorer::RunConfiguration *runConfiguration = 0);
    virtual ~IAnalyzerEngine();

    /// start analyzation process
    virtual void start() = 0;
    /// trigger async stop of the analyzation process
    virtual void stop() = 0;

    /// the active run configuration for this engine, might be zero
    ProjectExplorer::RunConfiguration *runConfiguration() const;

    /// the start parameters for this engine
    AnalyzerStartParameters startParameters() const;

signals:
    /// should be emitted when the debuggee outputted something
    void standardOutputReceived(const QString &);
    /// should be emitted when the debuggee outputted an error
    void standardErrorReceived(const QString &);
    /// can be emitted when you want to show a task, e.g. to display an error
    void taskToBeAdded(ProjectExplorer::Task::TaskType type, const QString &description,
                       const QString &file, int line);

    /// must be emitted when the engine finished
    void finished();
    /// must be emitted when the engine is starting
    void starting(const Analyzer::IAnalyzerEngine *);

private:
    ProjectExplorer::RunConfiguration *m_runConfig;
    AnalyzerStartParameters m_sp;
};

} // namespace Analyzer

#endif // IANALYZERENGINE_H
