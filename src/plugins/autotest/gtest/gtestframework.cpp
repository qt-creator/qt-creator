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

#include "gtestframework.h"
#include "gtesttreeitem.h"
#include "gtestparser.h"
#include "../testframeworkmanager.h"

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

TestTreeItem *GTestFramework::createRootNode()
{
    return new GTestTreeItem(
                this,
                QCoreApplication::translate("GTestFramework",
                                            GTest::Constants::FRAMEWORK_SETTINGS_CATEGORY),
                QString(), TestTreeItem::Root);
}

const char *GTestFramework::name() const
{
    return GTest::Constants::FRAMEWORK_NAME;
}

unsigned GTestFramework::priority() const
{
    return GTest::Constants::FRAMEWORK_PRIORITY;
}

QString GTestFramework::currentGTestFilter()
{
    return g_settings->gtestFilter;
}

QString GTestFramework::groupingToolTip() const
{
    return QCoreApplication::translate("GTestFramework",
                                       "Enable or disable grouping of test cases by folder or "
                                       "GTest filter.\nSee also Google Test settings.");
}

GTest::Constants::GroupMode GTestFramework::groupMode()
{
    return g_settings->groupMode;
}

} // namespace Internal
} // namespace Autotest
