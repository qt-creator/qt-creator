// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../testoutputreader.h"

namespace Utils { class QtcProcess; }

namespace Autotest {
namespace Internal {

class CTestOutputReader final : public Autotest::TestOutputReader
{
public:
    CTestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                      Utils::QtcProcess *testApplication, const Utils::FilePath &buildDirectory);

protected:
    void processOutputLine(const QByteArray &outputLineWithNewLine) final;
    TestResultPtr createDefaultResult() const final;
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
