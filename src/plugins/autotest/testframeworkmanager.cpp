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
#include "testrunner.h"
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
    m_testRunner = Internal::TestRunner::instance();
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
    for (ITestFramework *framework : m_registeredFrameworks.values())
        delete framework;
}

bool TestFrameworkManager::registerTestFramework(ITestFramework *framework)
{
    QTC_ASSERT(framework, return false);
    Id id = framework->id();
    QTC_ASSERT(!m_registeredFrameworks.contains(id), delete framework; return false);
    // TODO check for unique priority before registering
    qCDebug(LOG) << "Registering" << id;
    m_registeredFrameworks.insert(id, framework);

    if (IFrameworkSettings *frameworkSettings = framework->frameworkSettings())
        m_frameworkSettings.insert(id, frameworkSettings);

    return true;
}

void TestFrameworkManager::activateFrameworksFromSettings(const Internal::TestSettings *settings)
{
    FrameworkIterator it = m_registeredFrameworks.begin();
    FrameworkIterator end = m_registeredFrameworks.end();
    for ( ; it != end; ++it) {
        it.value()->setActive(settings->frameworks.value(it.key(), false));
        it.value()->setGrouping(settings->frameworksGrouping.value(it.key(), false));
    }
}

TestFrameworks TestFrameworkManager::registeredFrameworks() const
{
    return m_registeredFrameworks.values();
}

TestFrameworks TestFrameworkManager::sortedRegisteredFrameworks() const
{
    TestFrameworks registered = m_registeredFrameworks.values();
    Utils::sort(registered, &ITestFramework::priority);
    qCDebug(LOG) << "Registered frameworks sorted by priority" << registered;
    return registered;
}

TestFrameworks TestFrameworkManager::activeFrameworks() const
{
    TestFrameworks active;
    for (ITestFramework *framework : m_registeredFrameworks) {
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
    return instance()->m_registeredFrameworks.value(frameworkId, nullptr);
}

IFrameworkSettings *TestFrameworkManager::settingsForTestFramework(
            const Id &frameworkId) const
{
    return m_frameworkSettings.value(frameworkId, nullptr);
}

void TestFrameworkManager::synchronizeSettings(QSettings *s)
{
    Internal::AutotestPlugin::settings()->fromSettings(s);
    for (const Id &id : m_frameworkSettings.keys()) {
        if (IFrameworkSettings *fSettings = settingsForTestFramework(id))
            fSettings->fromSettings(s);
    }
}

bool TestFrameworkManager::hasActiveFrameworks() const
{
    for (ITestFramework *framework : m_registeredFrameworks.values()) {
        if (framework->active())
            return true;
    }
    return false;
}

Id ITestFramework::settingsId() const
{
    return Core::Id(Constants::SETTINGSPAGE_PREFIX)
            .withSuffix(QString("%1.%2").arg(priority()).arg(QLatin1String(name())));
}

} // namespace Autotest
