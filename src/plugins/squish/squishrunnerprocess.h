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
    enum RunnerCommand { Continue, EndRecord, Exit, Next, Pick, PrintVariables, Return, Step };
    enum RunnerMode { Run, StartAut, QueryServer, Record, Inspect };
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
    void requestListObject(const QString &value);
    void requestListProperties(const QString &value);
    Utils::Links setBreakpoints(const QString &scriptExtension);

    bool lastRunHadLicenseIssues() const { return m_licenseIssues; }

signals:
    void queryDone(const QString &output, const QString &error);
    void recorderDone();
    void runnerFinished();
    void interrupted(const QString &fileName, int line, int column);
    void localsUpdated(const QString &output);
    void propertiesFetched(const QStringList &properties);
    void objectPicked(const QString &output);
    void updateChildren(const QString &name, const QStringList &children);
    void runnerError(RunnerError error);
    void autIdRetrieved();

protected:
    void onDone() override;
    void onErrorOutput() override;

private:
    enum OutputMode { SingleLine, MultiLineChildren, MultiLineProperties };

    void onStdOutput(const QString &line);
    void handleMultiLineOutput(OutputMode mode);
    void onInspectorOutput(const QString &line);

    Utils::FilePath m_currentTestCasePath;
    QStringList m_multiLineContent;
    QString m_context;
    OutputMode m_outputMode = SingleLine;
    int m_autId = 0;
    bool m_licenseIssues = false;
    std::optional<RunnerMode> m_mode;
};

} // namespace Squish::Internal
