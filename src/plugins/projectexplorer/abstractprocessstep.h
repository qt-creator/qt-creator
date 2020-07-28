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

#include <QProcess>

namespace ProjectExplorer {
class ProcessParameters;

// Documentation inside.
class PROJECTEXPLORER_EXPORT AbstractProcessStep : public BuildStep
{
    Q_OBJECT

public:
    ProcessParameters *processParameters();

    bool ignoreReturnValue() const;
    void setIgnoreReturnValue(bool b);

    void emitFaultyConfigurationMessage();

protected:
    AbstractProcessStep(BuildStepList *bsl, Utils::Id id);
    ~AbstractProcessStep() override;

    bool init() override;
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    void doRun() override;
    void doCancel() override;
    void setLowPriority();

    virtual void finish(bool success);
    virtual void processStarted();
    virtual void processFinished(int exitCode, QProcess::ExitStatus status);
    virtual void processStartupFailed();
    virtual bool processSucceeded(int exitCode, QProcess::ExitStatus status);
    virtual void stdOutput(const QString &output);
    virtual void stdError(const QString &output);

private:
    void processReadyReadStdOutput();
    void processReadyReadStdError();
    void slotProcessFinished(int, QProcess::ExitStatus);

    class Private;
    Private *d;
};

} // namespace ProjectExplorer
