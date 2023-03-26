// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "quicktestframework.h"
#include "quicktestparser.h"
#include "quicktesttreeitem.h"

#include "../autotestconstants.h"
#include "../autotesttr.h"
#include "../testframeworkmanager.h"
#include "../qtest/qttestconstants.h"

namespace Autotest {
namespace Internal {

ITestParser *QuickTestFramework::createTestParser()
{
    return new QuickTestParser(this);
}

ITestTreeItem *QuickTestFramework::createRootNode()
{
    return new QuickTestTreeItem(this, displayName(), {}, ITestTreeItem::Root);
}

const char *QuickTestFramework::name() const
{
    return QuickTest::Constants::FRAMEWORK_NAME;
}

QString QuickTestFramework::displayName() const
{
    return Tr::tr("Quick Test");
}

unsigned QuickTestFramework::priority() const
{
    return 5;
}

ITestSettings *QuickTestFramework::testSettings()
{
    static const Utils::Id id
            = Utils::Id(Constants::FRAMEWORK_PREFIX).withSuffix(QtTest::Constants::FRAMEWORK_NAME);
    ITestFramework *qtTestFramework = TestFrameworkManager::frameworkForId(id);
    return qtTestFramework->testSettings();
}

} // namespace Internal
} // namespace Autotest
