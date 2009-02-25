/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
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
QT_END_NAMESPACE

namespace ProjectExplorer {

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
    AbstractProcessStep(Project *pro);
    // reimplemented from BuildStep::init()
    // You need to call this from YourBuildStep::init()
    virtual bool init(const QString & name);
    // reimplemented from BuildStep::init()
    // You need to call this from YourBuildStep::run()
    virtual void run(QFutureInterface<bool> &);

    // pure virtual functions inheritated from BuildStep
    virtual QString name() = 0;
    virtual QString displayName() = 0;
    virtual BuildStepConfigWidget *createConfigWidget() = 0;
    virtual bool immutable() const = 0;

    // setCommand() sets the executable to run in the \p buildConfiguration
    void setCommand(const QString &buildConfiguration, const QString &cmd);
    // returns the executable that is run for the \p buildConfiguration
    QString command(const QString &buildConfiguration) const;

    // sets the workingDirectory for the process for a buildConfiguration
    // if no workingDirectory is set, it falls back to the projects workingDirectory TODO remove that magic, thats bad
    void setWorkingDirectory(const QString &buildConfiguration, const QString &workingDirectory);
    //returns the workingDirectory for a \p buildConfiguration
    QString workingDirectory(const QString &buildConfiguration) const;

    // sets the command line arguments used by the process for a \p buildConfiguration
    void setArguments(const QString &buildConfiguration, const QStringList &arguments);
    // returns the arguments used in the \p buildCOnfiguration
    QStringList arguments(const QString &buildConfiguration) const;

    // enables or disables a BuildStep
    // Disabled BuildSteps immediately return true from their run method
    void setEnabled(const QString &buildConfiguration, bool b);
    // returns wheter the BuildStep is disabled
    bool enabled(const QString &buildConfiguration) const;

    void setEnvironment(const QString &buildConfiguration, Environment env);
    Environment environment(const QString &buildConfiguration) const;

protected:
    // Called after the process is started
    // the default implementation adds a process started message to the output message
    virtual void processStarted();
    // Called after the process Finished
    // the default implementation adds a line to the output window
    virtual bool processFinished(int exitCode, QProcess::ExitStatus status);
    // Called if the process could not be started,
    // by default adds a message to the output window
    virtual void processStartupFailed();
    virtual void stdOut(const QString &line);
    virtual void stdError(const QString &line);
private slots:
    void processReadyReadStdOutput();
    void processReadyReadStdError();
    void slotProcessFinished(int, QProcess::ExitStatus);
    void checkForCancel();
private:

    QTimer *m_timer;
    QFutureInterface<bool> *m_futureInterface;
    QString m_workingDirectory;
    QString m_command;
    QStringList m_arguments;
    bool m_enabled;
    QProcess *m_process;
    QEventLoop *m_eventLoop;
    ProjectExplorer::Environment m_environment;
};

} // namespace ProjectExplorer

#endif // ABSTRACTPROCESSSTEP_H
