// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "buildstep.h"

#include <QProcess>

namespace Utils {
class CommandLine;
enum class ProcessResult;
class QtcProcess;
namespace Tasking { class Group; }
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
    bool isSuccess(Utils::ProcessResult result) const;

    virtual void finish(Utils::ProcessResult result);

    bool checkWorkingDirectory();
    void setupProcess(Utils::QtcProcess *process);
    void runTaskTree(const Utils::Tasking::Group &recipe);
    ProcessParameters *displayedParameters() const;

private:
    void setupStreams();
    void processStartupFailed();
    void handleProcessDone();

    class Private;
    Private *d;
};

} // namespace ProjectExplorer
