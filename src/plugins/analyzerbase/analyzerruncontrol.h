/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef ANALYZERRUNCONTROL_H
#define ANALYZERRUNCONTROL_H

#include "analyzerconstants.h"

#include <valgrind/xmlprotocol/parser.h>

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/task.h>

#include <QtCore/QScopedPointer>

namespace Analyzer {

class IAnalyzerEngine;

namespace Internal {

class AnalyzerRunControl;

class AnalyzerRunControlFactory: public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT

public:
    AnalyzerRunControlFactory(QObject *parent = 0);

    typedef ProjectExplorer::RunConfiguration RunConfiguration;
    typedef ProjectExplorer::RunControl RunControl;

    // virtuals from IRunControlFactory
    bool canRun(RunConfiguration *runConfiguration, const QString &mode) const;
    RunControl *create(RunConfiguration *runConfiguration, const QString &mode);
    QString displayName() const;

    ProjectExplorer::IRunConfigurationAspect *createRunConfigurationAspect();
    ProjectExplorer::RunConfigWidget *createConfigurationWidget(RunConfiguration *runConfiguration);

signals:
    void runControlCreated(Analyzer::Internal::AnalyzerRunControl *);
};


class AnalyzerRunControl: public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    typedef ProjectExplorer::RunConfiguration RunConfiguration;
    // the constructor is likely to gain more arguments later
    explicit AnalyzerRunControl(RunConfiguration *runConfiguration);
    ~AnalyzerRunControl();

    // pure virtuals from ProjectExplorer::RunControl
    void start();
    StopResult stop();
    bool isRunning() const;
    QString displayName() const;

private slots:
    void receiveStandardOutput(const QString &);
    void receiveStandardError(const QString &);

    void addTask(ProjectExplorer::Task::TaskType type, const QString &description,
                 const QString &file, int line);

    void engineFinished();

private:
    bool m_isRunning;
    IAnalyzerEngine *m_engine;
};


} // namespace Internal
} // namespace Analyzer

#endif // ANALYZERRUNCONTROL_H
