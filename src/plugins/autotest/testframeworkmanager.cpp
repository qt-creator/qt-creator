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
#include "iframeworksettings.h"
#include "itestparser.h"
#include "testsettings.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QLoggingCategory>
#include <QSettings>

static Q_LOGGING_CATEGORY(LOG, "qtc.autotest.frameworkmanager", QtWarningMsg)

using namespace Core;

namespace Autotest {

static TestFrameworkManager *s_instance = nullptr;

TestFrameworkManager::TestFrameworkManager()
{
    s_instance = this;
}

TestFrameworkManager *TestFrameworkManager::instance()
{
    return s_instance;
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
    return true;
}

void TestFrameworkManager::activateFrameworksFromSettings(const Internal::TestSettings *settings)
{
    for (ITestFramework *framework : qAsConst(m_registeredFrameworks)) {
        framework->setActive(settings->frameworks.value(framework->id(), false));
        framework->setGrouping(settings->frameworksGrouping.value(framework->id(), false));
    }
}

TestFrameworks TestFrameworkManager::registeredFrameworks() const
{
    return m_registeredFrameworks;
}

TestFrameworks TestFrameworkManager::sortedRegisteredFrameworks() const
{
    TestFrameworks registered = m_registeredFrameworks;
    Utils::sort(registered, &ITestFramework::priority);
    qCDebug(LOG) << "Registered frameworks sorted by priority" << registered;
    return registered;
}

TestFrameworks TestFrameworkManager::activeFrameworks() const
{
    TestFrameworks active;
    for (ITestFramework *framework : qAsConst(m_registeredFrameworks)) {
        if (framework->active())
            active.append(framework);
    }
    return active;
}

TestFrameworks TestFrameworkManager::sortedActiveFrameworks() const
{
    TestFrameworks active = activeFrameworks();
    Utils::sort(active, &ITestFramework::priority);
    qCDebug(LOG) << "Active frameworks sorted by priority" << active;
    return active;
}

ITestFramework *TestFrameworkManager::frameworkForId(Id frameworkId)
{
    return Utils::findOrDefault(s_instance->m_registeredFrameworks,
            [frameworkId](ITestFramework *framework) {
                return framework->id() == frameworkId;
            });
}

void TestFrameworkManager::synchronizeSettings(QSettings *s)
{
    Internal::AutotestPlugin::settings()->fromSettings(s);
    for (ITestFramework *framework : qAsConst(m_registeredFrameworks)) {
        if (IFrameworkSettings *fSettings = framework->frameworkSettings())
            fSettings->fromSettings(s);
    }
}

bool TestFrameworkManager::hasActiveFrameworks() const
{
    return Utils::anyOf(m_registeredFrameworks, &ITestFramework::active);
}

Id ITestFramework::settingsId() const
{
    return Core::Id(Constants::SETTINGSPAGE_PREFIX)
            .withSuffix(QString("%1.%2").arg(priority()).arg(QLatin1String(name())));
}

} // namespace Autotest
