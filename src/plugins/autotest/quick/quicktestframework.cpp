// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "quicktestframework.h"
#include "quicktestparser.h"
#include "quicktesttreeitem.h"

#include "../autotesttr.h"

namespace Autotest {
namespace Internal {

QuickTestFramework &theQuickTestFramework()
{
    static QuickTestFramework framework;
    return framework;
}

QuickTestFramework::QuickTestFramework()
{
    setId(QuickTest::Constants::FRAMEWORK_ID);
    setDisplayName(Tr::tr("Quick Test"));
    setPriority(5);
}

ITestParser *QuickTestFramework::createTestParser()
{
    return new QuickTestParser(this);
}

ITestTreeItem *QuickTestFramework::createRootNode()
{
    return new QuickTestTreeItem(this, displayName(), {}, ITestTreeItem::Root);
}

} // namespace Internal
} // namespace Autotest
