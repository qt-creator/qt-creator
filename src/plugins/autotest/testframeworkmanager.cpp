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
#include "itestframework.h"
#include "itestparser.h"
#include "testrunner.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(LOG, "qtc.autotest.frameworkmanager")

namespace Autotest {
namespace Internal {

TestFrameworkManager *s_instance = 0;

TestFrameworkManager::TestFrameworkManager()
{
    m_testTreeModel = TestTreeModel::instance();
    m_testRunner = TestRunner::instance();
    s_instance = this;
}

TestFrameworkManager *TestFrameworkManager::instance()
{
    if (!s_instance)
        return new TestFrameworkManager;
    return s_instance;
}

TestFrameworkManager::~TestFrameworkManager()
{
    delete m_testRunner;
    delete m_testTreeModel;
}

bool TestFrameworkManager::registerTestFramework(ITestFramework *framework)
{
    QTC_ASSERT(framework, return false);
    Core::Id id = Core::Id(Constants::FRAMEWORK_PREFIX).withSuffix(framework->name());
    QTC_ASSERT(!m_registeredFrameworks.contains(id), return false);
    // check for unique priority?
    m_registeredFrameworks.insert(id, framework);
    return true;
}

QList<Core::Id> TestFrameworkManager::registeredFrameworkIds() const
{
    return m_registeredFrameworks.keys();
}

QList<Core::Id> TestFrameworkManager::sortedFrameworkIds() const
{
    QList<Core::Id> sorted = registeredFrameworkIds();
    qCDebug(LOG) << "Registered frameworks" << sorted;

    Utils::sort(sorted, [this] (const Core::Id &lhs, const Core::Id &rhs) {
        return m_registeredFrameworks[lhs]->priority() < m_registeredFrameworks[rhs]->priority();
    });
    qCDebug(LOG) << "Sorted by priority" << sorted;
    return sorted;
}

TestTreeItem *TestFrameworkManager::rootNodeForTestFramework(const Core::Id &frameworkId) const
{
    ITestFramework *framework = m_registeredFrameworks.value(frameworkId, 0);
    return framework ? framework->rootNode() : 0;
}

ITestParser *TestFrameworkManager::testParserForTestFramework(const Core::Id &frameworkId) const
{
    ITestFramework *framework = m_registeredFrameworks.value(frameworkId, 0);
    if (!framework)
        return 0;
    ITestParser *testParser = framework->testParser();
    qCDebug(LOG) << "Setting" << frameworkId << "as Id for test parser";
    testParser->setId(frameworkId);
    return testParser;
}

} // namespace Internal
} // namespace Autotest
