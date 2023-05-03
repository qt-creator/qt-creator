// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../testoutputreader.h"

namespace Utils { class Process; }

namespace Autotest {
namespace Internal {

class CTestOutputReader final : public Autotest::TestOutputReader
{
public:
    CTestOutputReader(Utils::Process *testApplication, const Utils::FilePath &buildDirectory);

protected:
    void processOutputLine(const QByteArray &outputLineWithNewLine) final;
    TestResult createDefaultResult() const final;
private:
    void sendCompleteInformation();
    int m_currentTestNo = -1;
    QString m_project;
    QString m_testName;
    QString m_description;
    ResultType m_result = ResultType::Invalid;
    bool m_expectExceptionFromCrash = false;
};

} // namespace Internal
} // namespace Autotest
