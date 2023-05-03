// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctestconfiguration.h"
#include "ctestoutputreader.h"

namespace Autotest {
namespace Internal {

CTestConfiguration::CTestConfiguration(ITestBase *testBase)
    : TestToolConfiguration(testBase)
{
    setDisplayName("CTest");
}

TestOutputReader *CTestConfiguration::createOutputReader(Utils::Process *app) const
{
    return new CTestOutputReader(app, workingDirectory());
}

} // namespace Internal
} // namespace Autotest
