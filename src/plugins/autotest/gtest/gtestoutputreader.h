// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../testoutputreader.h"

namespace Autotest {
namespace Internal {

class GTestOutputReader : public TestOutputReader
{
public:
    GTestOutputReader(Utils::Process *testApplication, const Utils::FilePath &buildDirectory,
                      const Utils::FilePath &projectFile);
protected:
    void processOutputLine(const QByteArray &outputLine) override;
    void processStdError(const QByteArray &outputLine) override;
    TestResult createDefaultResult() const override;

private:
    void onDone(int exitCode) override;
    void setCurrentTestCase(const QString &testCase);
    void setCurrentTestSuite(const QString &testSuite);
    void handleDescriptionAndReportResult(const TestResult &testResult);

    Utils::FilePath m_projectFile;
    QString m_currentTestSuite;
    QString m_currentTestCase;
    QString m_description;
    int m_iteration = 1;
    bool m_testSetStarted = false;
};

} // namespace Internal
} // namespace Autotest
