// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gtestframework.h"

#include "../autotesttr.h"
#include "gtesttreeitem.h"
#include "gtestparser.h"

namespace Autotest {
namespace Internal {

static GTestSettings *g_settings;

GTestFramework::GTestFramework()
    : ITestFramework(true)
{
    g_settings = &m_settings;
}

ITestParser *GTestFramework::createTestParser()
{
    return new GTestParser(this);
}

ITestTreeItem *GTestFramework::createRootNode()
{
    return new GTestTreeItem(this, displayName(), {}, ITestTreeItem::Root);
}

const char *GTestFramework::name() const
{
    return GTest::Constants::FRAMEWORK_NAME;
}

QString GTestFramework:: displayName() const
{
    return Tr::tr(GTest::Constants::FRAMEWORK_SETTINGS_CATEGORY);
}

unsigned GTestFramework::priority() const
{
    return GTest::Constants::FRAMEWORK_PRIORITY;
}

QString GTestFramework::currentGTestFilter()
{
    return g_settings->gtestFilter.value();
}

QString GTestFramework::groupingToolTip() const
{
    return Tr::tr("Enable or disable grouping of test cases by folder or "
                  "GTest filter.\nSee also Google Test settings.");
}

GTest::Constants::GroupMode GTestFramework::groupMode()
{
    return GTest::Constants::GroupMode(g_settings->groupMode.itemValue().toInt());
}

} // namespace Internal
} // namespace Autotest
