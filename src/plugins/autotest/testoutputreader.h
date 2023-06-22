// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "testresult.h"

#include <QObject>

namespace Utils { class Process; }

namespace Autotest {

class TestOutputReader : public QObject
{
    Q_OBJECT
public:
    TestOutputReader(Utils::Process *testApplication, const Utils::FilePath &buildDirectory);
    virtual ~TestOutputReader();
    void processStdOutput(const QByteArray &outputLine);
    virtual void processStdError(const QByteArray &outputLine);
    void reportCrash();
    void createAndReportResult(const QString &message, ResultType type);
    bool hadValidOutput() const { return m_hadValidOutput; }
    int disabledTests() const { return m_disabled; }
    bool hasSummary() const { return !m_summary.isEmpty(); }
    QHash<ResultType, int> summary() const { return m_summary; }
    void setId(const QString &id) { m_id = id; }
    QString id() const { return m_id; }

    virtual void onDone(int exitCode) { Q_UNUSED(exitCode) }

    void resetCommandlineColor();
signals:
    void newResult(const TestResult &result);
    void newOutputLineAvailable(const QByteArray &outputLine, OutputChannel channel);
protected:
    static Utils::FilePath constructSourceFilePath(const Utils::FilePath &base,
                                                   const QString &file);

    QString removeCommandlineColors(const QString &original);
    virtual void processOutputLine(const QByteArray &outputLine) = 0;
    virtual TestResult createDefaultResult() const = 0;
    void checkForSanitizerOutput(const QByteArray &line);
    void sendAndResetSanitizerResult();

    void reportResult(const TestResult &result);
    Utils::FilePath m_buildDir;
    QString m_id;
    QHash<ResultType, int> m_summary;
    int m_disabled = -1;
private:
    enum class SanitizerOutputMode { None, Asan, Ubsan};
    TestResult m_sanitizerResult;
    QStringList m_sanitizerLines;
    SanitizerOutputMode m_sanitizerOutputMode = SanitizerOutputMode::None;
    bool m_hadValidOutput = false;
};

} // namespace Autotest
