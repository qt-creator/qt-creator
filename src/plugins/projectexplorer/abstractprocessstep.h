/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef ABSTRACTPROCESSSTEP_H
#define ABSTRACTPROCESSSTEP_H

#include "buildstep.h"
#include "environment.h"

#include <QtCore/QString>
#include <QtCore/QProcess>

QT_BEGIN_NAMESPACE
class QEventLoop;
class QTimer;
QT_END_NAMESPACE

namespace ProjectExplorer {

class IOutputParser;

/*!
  AbstractProcessStep is a convenience class, which can be used as a base class instead of BuildStep.
  It should be used as a base class if your buildstep just needs to run a process.

  Usage:
    Use setCommand(), setArguments(), setWorkingDirectory() to specify the process you want to run
    (you need to do that before calling AbstractProcessStep::init()).
    Inside YourBuildStep::init() call AbstractProcessStep::init().
    Inside YourBuildStep::run() call AbstractProcessStep::run(), which automatically starts the proces
    and by default adds the output on stdOut and stdErr to the OutputWindow.
    If you need to process the process output override stdOut() and/or stdErr.
    The two functions processStarted() and processFinished() are called after starting/finishing the process.
    By default they add a message to the output window.

    Use setEnabled() to control wheter the BuildStep needs to run. (A disabled BuildStep immediately returns true,
    from the run function.)

*/

class PROJECTEXPLORER_EXPORT AbstractProcessStep : public BuildStep
{
    Q_OBJECT
public:
    AbstractProcessStep(BuildConfiguration *bc);
    AbstractProcessStep(AbstractProcessStep *bs, BuildConfiguration *bc);
    virtual ~AbstractProcessStep();

    /// reimplemented from BuildStep::init()
    /// You need to call this from YourBuildStep::init()
    virtual bool init();
    /// reimplemented from BuildStep::init()
    /// You need to call this from YourBuildStep::run()
    virtual void run(QFutureInterface<bool> &);

    // pure virtual functions inheritated from BuildStep
    virtual QString name() = 0;
    virtual QString displayName() = 0;
    virtual BuildStepConfigWidget *createConfigWidget() = 0;
    virtual bool immutable() const = 0;

    /// setCommand() sets the executable to run in the \p buildConfiguration
    /// should be called from init()
    void setCommand(const QString &cmd);

    /// sets the workingDirectory for the process for a buildConfiguration
    /// should be called from init()
    void setWorkingDirectory(const QString &workingDirectory);

    /// sets the command line arguments used by the process for a \p buildConfiguration
    /// should be called from init()
    void setArguments(const QStringList &arguments);

    /// enables or disables a BuildStep
    /// Disabled BuildSteps immediately return true from their run method
    /// should be called from init()
    void setEnabled(bool b);

    /// If ignoreReturnValue is set to true, then the abstractprocess step will
    /// return sucess even if the return value indicates otherwise
    /// should be called from init
    void setIgnoreReturnValue(bool b);
    /// Set the Environment for running the command
    /// should be called from init()
    void setEnvironment(Environment env);

    QString workingDirectory() const;

    // derived classes needs to call this function
    /// Delete all existing output parsers and start a new chain with the
    /// given parser.
    void setOutputParser(ProjectExplorer::IOutputParser *parser);
    /// Append the given output parser to the existing chain of parsers.
    void appendOutputParser(ProjectExplorer::IOutputParser *parser);
    ProjectExplorer::IOutputParser *outputParser() const;

protected:
    /// Called after the process is started
    /// the default implementation adds a process started message to the output message
    virtual void processStarted();
    /// Called after the process Finished
    /// the default implementation adds a line to the output window
    virtual bool processFinished(int exitCode, QProcess::ExitStatus status);
    /// Called if the process could not be started,
    /// by default adds a message to the output window
    virtual void processStartupFailed();
    /// Called for each line of output on stdOut()
    /// the default implementation adds the line to the
    /// application output window
    virtual void stdOutput(const QString &line);
    /// Called for each line of output on StdErrror()
    /// the default implementation adds the line to the
    /// application output window
    virtual void stdError(const QString &line);

private slots:
    void processReadyReadStdOutput();
    void processReadyReadStdError();
    void slotProcessFinished(int, QProcess::ExitStatus);
    void checkForCancel();

    void taskAdded(const ProjectExplorer::TaskWindow::Task &task);
    void outputAdded(const QString &string);

private:
    QTimer *m_timer;
    QFutureInterface<bool> *m_futureInterface;
    QString m_workingDirectory;
    QString m_command;
    QStringList m_arguments;
    bool m_enabled;
    bool m_ignoreReturnValue;
    QProcess *m_process;
    QEventLoop *m_eventLoop;
    ProjectExplorer::Environment m_environment;
    ProjectExplorer::IOutputParser *m_outputParserChain;
};

} // namespace ProjectExplorer

#endif // ABSTRACTPROCESSSTEP_H
