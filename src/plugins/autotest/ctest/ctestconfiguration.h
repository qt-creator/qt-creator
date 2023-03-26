// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../testconfiguration.h"

namespace Autotest {
namespace Internal {

class CTestConfiguration final : public Autotest::TestToolConfiguration
{
public:
    explicit CTestConfiguration(ITestBase *testBase);

    TestOutputReader *createOutputReader(const QFutureInterface<TestResult> &fi,
                                         Utils::QtcProcess *app) const final;
};

} // namespace Internal
} // namespace Autotest
