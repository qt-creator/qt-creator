/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#ifndef ABSTRACTPROCESSSTEP_H
#define ABSTRACTPROCESSSTEP_H

#include "buildstep.h"
#include "processparameters.h"

#include <QString>
#include <QProcess>

QT_BEGIN_NAMESPACE
class QEventLoop;
class QTimer;
QT_END_NAMESPACE

namespace Utils {
class QtcProcess;
}
namespace ProjectExplorer {

class IOutputParser;
// Documentation inside.
class PROJECTEXPLORER_EXPORT AbstractProcessStep : public BuildStep
{
    Q_OBJECT

public:
    virtual ~AbstractProcessStep();

    virtual bool init();
    virtual void run(QFutureInterface<bool> &);
    bool runInGuiThread() const { return true; }

    ProcessParameters *processParameters() { return &m_param; }

    bool ignoreReturnValue();
    void setIgnoreReturnValue(bool b);

    void setOutputParser(ProjectExplorer::IOutputParser *parser);
    void appendOutputParser(ProjectExplorer::IOutputParser *parser);
    ProjectExplorer::IOutputParser *outputParser() const;
protected:
    AbstractProcessStep(BuildStepList *bsl, const Core::Id id);
    AbstractProcessStep(BuildStepList *bsl, AbstractProcessStep *bs);

    virtual void processStarted();
    virtual void processFinished(int exitCode, QProcess::ExitStatus status);
    virtual void processStartupFailed();
    virtual bool processSucceeded(int exitCode, QProcess::ExitStatus status);
    virtual void stdOutput(const QString &line);
    virtual void stdError(const QString &line);

    QFutureInterface<bool> *futureInterface() const;

private slots:
    void processReadyReadStdOutput();
    void processReadyReadStdError();
    void slotProcessFinished(int, QProcess::ExitStatus);
    void checkForCancel();

    void cleanUp();

    void taskAdded(const ProjectExplorer::Task &task);

    void outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format);

private:
    QTimer *m_timer;
    QFutureInterface<bool> *m_futureInterface;
    ProcessParameters m_param;
    bool m_ignoreReturnValue;
    Utils::QtcProcess *m_process;
    QEventLoop *m_eventLoop;
    ProjectExplorer::IOutputParser *m_outputParserChain;
    bool m_killProcess;
    bool m_skipFlush;
};

} // namespace ProjectExplorer

#endif // ABSTRACTPROCESSSTEP_H
