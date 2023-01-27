// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "boosttestframework.h"

#include "boosttestconstants.h"
#include "boosttesttreeitem.h"
#include "boosttestparser.h"
#include "../autotesttr.h"

namespace Autotest {
namespace Internal {

ITestParser *BoostTestFramework::createTestParser()
{
    return new BoostTestParser(this);
}

ITestTreeItem *BoostTestFramework::createRootNode()
{
    return new BoostTestTreeItem(this, displayName(), {}, ITestTreeItem::Root);
}

const char *BoostTestFramework::name() const
{
    return BoostTest::Constants::FRAMEWORK_NAME;
}

QString BoostTestFramework::displayName() const
{
    return Tr::tr(BoostTest::Constants::FRAMEWORK_SETTINGS_CATEGORY);
}

unsigned BoostTestFramework::priority() const
{
    return BoostTest::Constants::FRAMEWORK_PRIORITY;
}

} // namespace Internal
} // namespace Autotest
