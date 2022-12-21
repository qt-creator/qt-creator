// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "itestframework.h"

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Autotest {
namespace Internal {
struct TestSettings;
}

class TestFrameworkManager final
{

public:
    TestFrameworkManager();
    ~TestFrameworkManager();

    bool registerTestFramework(ITestFramework *framework);
    bool registerTestTool(ITestTool *testTool);
    void synchronizeSettings(QSettings *s);

    static ITestFramework *frameworkForId(Utils::Id frameworkId);
    static ITestTool *testToolForId(Utils::Id testToolId);
    static ITestTool *testToolForBuildSystemId(Utils::Id buildSystemId);
    static void activateFrameworksAndToolsFromSettings(const Internal::TestSettings *settings);
    static const TestFrameworks registeredFrameworks();
    static const TestTools registeredTestTools();

private:
    TestFrameworks m_registeredFrameworks;
    TestTools m_registeredTestTools;
};

} // namespace Autotest
