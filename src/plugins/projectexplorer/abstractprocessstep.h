/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "buildstep.h"
#include "processparameters.h"

#include <projectexplorer/ioutputparser.h>

#include <utils/qtcprocess.h>

#include <QString>
#include <QTimer>

#include <memory>

namespace Utils { class QtcProcess; }
namespace ProjectExplorer {

class IOutputParser;

// Documentation inside.
class PROJECTEXPLORER_EXPORT AbstractProcessStep : public BuildStep
{
    Q_OBJECT

public:
    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &) override;
    bool runInGuiThread() const final { return true; }

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

private:
    void processReadyReadStdOutput();
    void processReadyReadStdError();
    void slotProcessFinished(int, QProcess::ExitStatus);
    void checkForCancel();

    void cleanUp(QProcess *process);

    void taskAdded(const Task &task, int linkedOutputLines = 0, int skipLines = 0);

    void outputAdded(const QString &string, BuildStep::OutputFormat format);

    QTimer m_timer;
    QFutureInterface<bool> *m_futureInterface = nullptr;
    std::unique_ptr<Utils::QtcProcess> m_process;
    std::unique_ptr<IOutputParser> m_outputParserChain;
    ProcessParameters m_param;
    bool m_ignoreReturnValue = false;
    bool m_skipFlush = false;
};

} // namespace ProjectExplorer
