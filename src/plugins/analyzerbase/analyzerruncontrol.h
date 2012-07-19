/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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

#ifndef ANALYZERRUNCONTROL_H
#define ANALYZERRUNCONTROL_H

#include "analyzerbase_global.h"

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/task.h>

namespace Analyzer {

class AnalyzerStartParameters;
class IAnalyzerTool;

class ANALYZER_EXPORT AnalyzerRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    AnalyzerRunControl(IAnalyzerTool *tool, const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration);
    ~AnalyzerRunControl();

    // ProjectExplorer::RunControl
    void start();
    StopResult stop();
    bool isRunning() const;
    QString displayName() const;
    QIcon icon() const;

private slots:
    void stopIt();
    void receiveOutput(const QString &, Utils::OutputFormat format);

    void addTask(ProjectExplorer::Task::TaskType type, const QString &description,
                 const QString &file, int line);

    void engineFinished();

private:
    class Private;
    Private *d;
};

} // namespace Analyzer

#endif // ANALYZERRUNCONTROL_H
