/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "testframeworkmanager.h"

#include "autotestconstants.h"
#include "autotestplugin.h"
#include "testsettings.h"

#include <utils/aspects.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QSettings>

using namespace Utils;

namespace Autotest {

static TestFrameworkManager *s_instance = nullptr;

TestFrameworkManager::TestFrameworkManager()
{
    s_instance = this;
}

TestFrameworkManager::~TestFrameworkManager()
{
    qDeleteAll(m_registeredFrameworks);
    s_instance = nullptr;
}

bool TestFrameworkManager::registerTestFramework(ITestFramework *framework)
{
    QTC_ASSERT(framework, return false);
    QTC_ASSERT(!m_registeredFrameworks.contains(framework), return false);
    // TODO check for unique priority before registering
    m_registeredFrameworks.append(framework);
    Utils::sort(m_registeredFrameworks, &ITestFramework::priority);
    return true;
}

bool TestFrameworkManager::registerTestTool(ITestTool *testTool)
{
    QTC_ASSERT(testTool, return false);
    QTC_ASSERT(!m_registeredTestTools.contains(testTool), return false);
    m_registeredTestTools.append(testTool);
    return true;
}

void TestFrameworkManager::activateFrameworksAndToolsFromSettings(
        const Internal::TestSettings *settings)
{
    for (ITestFramework *framework : qAsConst(s_instance->m_registeredFrameworks)) {
        framework->setActive(settings->frameworks.value(framework->id(), false));
        framework->setGrouping(settings->frameworksGrouping.value(framework->id(), false));
    }
    for (ITestTool *testTool : qAsConst(s_instance->m_registeredTestTools))
        testTool->setActive(settings->tools.value(testTool->id(), false));
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

void TestFrameworkManager::synchronizeSettings(QSettings *s)
{
    Internal::AutotestPlugin::settings()->fromSettings(s);
    for (ITestFramework *framework : qAsConst(m_registeredFrameworks)) {
        if (ITestSettings *fSettings = framework->testSettings())
            fSettings->readSettings(s);
    }
    for (ITestTool *testTool : qAsConst(m_registeredTestTools)) {
        if (ITestSettings *tSettings = testTool->testSettings())
            tSettings->readSettings(s);
    }
}

} // namespace Autotest
