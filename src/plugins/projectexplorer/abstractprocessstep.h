// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "buildstep.h"

#include <QProcess>

namespace Utils {
class CommandLine;
}

namespace ProjectExplorer {
class ProcessParameters;

// Documentation inside.
class PROJECTEXPLORER_EXPORT AbstractProcessStep : public BuildStep
{
    Q_OBJECT

public:
    ProcessParameters *processParameters();
    bool setupProcessParameters(ProcessParameters *params) const;

    bool ignoreReturnValue() const;
    void setIgnoreReturnValue(bool b);

    void setCommandLineProvider(const std::function<Utils::CommandLine()> &provider);
    void setWorkingDirectoryProvider(const std::function<Utils::FilePath()> &provider);
    void setEnvironmentModifier(const std::function<void(Utils::Environment &)> &modifier);
    void setUseEnglishOutput();

    void emitFaultyConfigurationMessage();

protected:
    AbstractProcessStep(BuildStepList *bsl, Utils::Id id);
    ~AbstractProcessStep() override;

    bool init() override;
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    void doRun() override;
    void doCancel() override;
    void setLowPriority();
    void setDisplayedParameters(ProcessParameters *params);

    virtual void finish(bool success);
    virtual void processStartupFailed();
    virtual void stdOutput(const QString &output);
    virtual void stdError(const QString &output);

private:
    ProcessParameters *displayedParameters() const;
    virtual void processFinished(bool success);
    void handleProcessDone();

    class Private;
    Private *d;
};

} // namespace ProjectExplorer
