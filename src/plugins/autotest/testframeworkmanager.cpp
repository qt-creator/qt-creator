// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testframeworkmanager.h"

#include "testsettings.h"

#include <utils/aspects.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

using namespace Utils;

namespace Autotest {

static TestFrameworkManager *s_instance = nullptr;

TestFrameworkManager::TestFrameworkManager()
{
    s_instance = this;
}

TestFrameworkManager::~TestFrameworkManager()
{
    s_instance = nullptr;
}

void TestFrameworkManager::registerTestFramework(ITestFramework *framework)
{
    QTC_ASSERT(framework, return);
    QTC_ASSERT(!m_registeredFrameworks.contains(framework), return);
    // TODO check for unique priority before registering
    m_registeredFrameworks.append(framework);
    Utils::sort(m_registeredFrameworks, &ITestFramework::priority);
}

void TestFrameworkManager::registerTestTool(ITestTool *testTool)
{
    QTC_ASSERT(testTool, return);
    QTC_ASSERT(!m_registeredTestTools.contains(testTool), return);
    m_registeredTestTools.append(testTool);
}

void TestFrameworkManager::activateFrameworksAndToolsFromSettings()
{
    const Internal::TestSettings &settings = Internal::testSettings();
    for (ITestFramework *framework : std::as_const(s_instance->m_registeredFrameworks)) {
        framework->setActive(settings.frameworks.value(framework->id(), false));
        framework->setGrouping(settings.frameworksGrouping.value(framework->id(), false));
    }
    for (ITestTool *testTool : std::as_const(s_instance->m_registeredTestTools))
        testTool->setActive(settings.tools.value(testTool->id(), false));
}

const TestFrameworks TestFrameworkManager::registeredFrameworks()
{
    return s_instance->m_registeredFrameworks;
}

const TestTools TestFrameworkManager::registeredTestTools()
{
    return s_instance->m_registeredTestTools;
}

ITestFramework *TestFrameworkManager::frameworkForId(Id frameworkId)
{
    return Utils::findOrDefault(s_instance->m_registeredFrameworks,
            [frameworkId](ITestFramework *framework) {
                return framework->id() == frameworkId;
    });
}

ITestTool *TestFrameworkManager::testToolForId(Id testToolId)
{
    return Utils::findOrDefault(s_instance->m_registeredTestTools,
            [testToolId](ITestTool *testTool) {
                return testTool->id() == testToolId;
    });
}

ITestTool *TestFrameworkManager::testToolForBuildSystemId(Id buildSystemId)
{
    if (!buildSystemId.isValid())
        return nullptr;

    return Utils::findOrDefault(s_instance->m_registeredTestTools,
                                [&buildSystemId](ITestTool *testTool) {
        return testTool->buildSystemId() == buildSystemId;
    });
}

void TestFrameworkManager::synchronizeSettings()
{
    Internal::testSettings().fromSettings();
    for (ITestFramework *framework : std::as_const(m_registeredFrameworks))
        framework->readSettings();

    for (ITestTool *testTool : std::as_const(m_registeredTestTools))
        testTool->readSettings();
}

} // namespace Autotest
