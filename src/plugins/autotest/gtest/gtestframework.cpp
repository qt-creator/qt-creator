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
#include "gtestconstants.h"
#include "gtestsettings.h"
#include "gtestsettingspage.h"
#include "gtesttreeitem.h"
#include "gtestparser.h"

namespace Autotest {
namespace Internal {

ITestParser *GTestFramework::createTestParser() const
{
    return new GTestParser;
}

TestTreeItem *GTestFramework::createRootNode() const
{
    return new GTestTreeItem(
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

IFrameworkSettings *GTestFramework::createFrameworkSettings() const
{
    return new GTestSettings;
}

ITestSettingsPage *GTestFramework::createSettingsPage(QSharedPointer<IFrameworkSettings> settings) const
{
    return new GTestSettingsPage(settings, this);
}

bool GTestFramework::hasFrameworkSettings() const
{
    return true;
}

} // namespace Internal
} // namespace Autotest
