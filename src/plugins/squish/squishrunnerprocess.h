// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "squishprocessbase.h"

#include <utils/link.h>

#include <optional>

namespace Squish::Internal {

class SquishRunnerProcess : public SquishProcessBase
{
    Q_OBJECT
public:
    enum RunnerCommand { Continue, Exit, Next, PrintVariables, Quit, Return, Step };
    enum RunnerMode { Run, StartAut, QueryServer };
    enum RunnerError { InvalidSocket, MappedAutMissing };

    explicit SquishRunnerProcess(QObject *parent = nullptr);
    ~SquishRunnerProcess() = default;

    void setupProcess(RunnerMode mode);
    void setTestCasePath(const Utils::FilePath &testCasePath) { m_currentTestCasePath = testCasePath; }

    void start(const Utils::CommandLine &cmdline, const Utils::Environment &env) override;

    inline qint64 processId() const { return m_process.processId(); }

    int autId() const { return m_autId; }

    void writeCommand(RunnerCommand cmd);
    void requestExpanded(const QString &variableName);
    Utils::Links setBreakpoints(const QString &scriptExtension);

signals:
    void queryDone(const QString &output, const QString &error);
    void runnerFinished();
    void interrupted(const QString &fileName, int line, int column);
    void localsUpdated(const QString &output);
    void runnerError(RunnerError error);

protected:
    void onDone() override;
    void onErrorOutput() override;

private:
    void onStdOutput(const QString &line);

    Utils::FilePath m_currentTestCasePath;
    int m_autId = 0;
    bool m_licenseIssues = false;
    std::optional<RunnerMode> m_mode;
};

} // namespace Squish::Internal
