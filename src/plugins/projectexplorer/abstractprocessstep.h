/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
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

namespace Utils { class QtcProcess; }
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

    void setOutputParser(IOutputParser *parser);
    void appendOutputParser(IOutputParser *parser);
    IOutputParser *outputParser() const;

    void emitFaultyConfigurationMessage();

protected:
    AbstractProcessStep(BuildStepList *bsl, Core::Id id);
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

    void taskAdded(const Task &task, int linkedOutputLines = 0, int skipLines = 0);

    void outputAdded(const QString &string, BuildStep::OutputFormat format);

private:
    QTimer *m_timer;
    QFutureInterface<bool> *m_futureInterface;
    ProcessParameters m_param;
    bool m_ignoreReturnValue;
    Utils::QtcProcess *m_process;
    IOutputParser *m_outputParserChain;
    bool m_killProcess;
    bool m_skipFlush;
};

} // namespace ProjectExplorer

#endif // ABSTRACTPROCESSSTEP_H
