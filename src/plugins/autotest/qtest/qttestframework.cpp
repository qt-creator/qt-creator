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

#include "qttestframework.h"
#include "qttestconstants.h"
#include "qttestparser.h"
#include "qttestsettings.h"
#include "qttestsettingspage.h"
#include "qttesttreeitem.h"

namespace Autotest {
namespace Internal {

ITestParser *QtTestFramework::createTestParser() const
{
    return new QtTestParser;
}

TestTreeItem *QtTestFramework::createRootNode() const
{
    return new QtTestTreeItem(
                QCoreApplication::translate("QtTestFramework",
                                            QtTest::Constants::FRAMEWORK_SETTINGS_CATEGORY),
                QString(), TestTreeItem::Root);
}

IFrameworkSettings *QtTestFramework::createFrameworkSettings() const
{
    return new QtTestSettings;
}

ITestSettingsPage *QtTestFramework::createSettingsPage(QSharedPointer<IFrameworkSettings> settings) const
{
    return new QtTestSettingsPage(settings, this);
}

bool QtTestFramework::hasFrameworkSettings() const
{
    return true;
}

const char *QtTestFramework::name() const
{
    return QtTest::Constants::FRAMEWORK_NAME;
}

unsigned QtTestFramework::priority() const
{
    return QtTest::Constants::FRAMEWORK_PRIORITY;
}

} // namespace Internal
} // namespace Autotest
