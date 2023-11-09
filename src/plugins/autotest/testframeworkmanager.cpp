// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testframeworkmanager.h"

#include "testsettings.h"

#include <utils/aspects.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

using namespace Utils;

namespace Autotest::TestFrameworkManager {

TestFrameworks &testFrameworks()
{
    static TestFrameworks theFrameworks;
    return theFrameworks;
}

TestTools &testTools()
{
    static TestTools theTools;
    return theTools;
}

void registerTestFramework(ITestFramework *framework)
{
    QTC_ASSERT(framework, return);
    QTC_ASSERT(!testFrameworks().contains(framework), return);
    // TODO check for unique priority before registering
    testFrameworks().append(framework);
    Utils::sort(testFrameworks(), &ITestFramework::priority);
}

void registerTestTool(ITestTool *testTool)
{
    QTC_ASSERT(testTool, return);
    QTC_ASSERT(!testTools().contains(testTool), return);
    testTools().append(testTool);
}

void activateFrameworksAndToolsFromSettings()
{
    const Internal::TestSettings &settings = Internal::testSettings();
    for (ITestFramework *framework : std::as_const(testFrameworks())) {
        framework->setActive(settings.frameworks.value(framework->id(), false));
        framework->setGrouping(settings.frameworksGrouping.value(framework->id(), false));
    }
    for (ITestTool *testTool : std::as_const(testTools()))
        testTool->setActive(settings.tools.value(testTool->id(), false));
}

const TestFrameworks registeredFrameworks()
{
    return testFrameworks();
}

const TestTools registeredTestTools()
{
    return testTools();
}

ITestFramework *frameworkForId(Id frameworkId)
{
    return Utils::findOrDefault(testFrameworks(), [frameworkId](ITestFramework *framework) {
                return framework->id() == frameworkId;
    });
}

ITestTool *testToolForId(Id testToolId)
{
    return Utils::findOrDefault(testTools(), [testToolId](ITestTool *testTool) {
                return testTool->id() == testToolId;
    });
}

ITestTool *testToolForBuildSystemId(Id buildSystemId)
{
    if (!buildSystemId.isValid())
        return nullptr;

    return Utils::findOrDefault(testTools(), [&buildSystemId](ITestTool *testTool) {
        return testTool->buildSystemId() == buildSystemId;
    });
}

} // Autotest::TestframeworkManager
