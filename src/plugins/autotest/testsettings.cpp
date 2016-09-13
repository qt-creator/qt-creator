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

#include "testsettings.h"
#include "testframeworkmanager.h"

#include <coreplugin/id.h>

#include <QSettings>

namespace Autotest {
namespace Internal {

static const char group[]                   = "Autotest";
static const char timeoutKey[]              = "Timeout";
static const char omitInternalKey[]         = "OmitInternal";
static const char omitRunConfigWarnKey[]    = "OmitRCWarnings";
static const char limitResultOutputKey[]    = "LimitResultOutput";
static const char autoScrollKey[]           = "AutoScrollResults";
static const char alwaysParseKey[]          = "AlwaysParse";

static const int defaultTimeout = 60000;

TestSettings::TestSettings()
    : timeout(defaultTimeout)
{
}

void TestSettings::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(group));
    s->setValue(QLatin1String(timeoutKey), timeout);
    s->setValue(QLatin1String(omitInternalKey), omitInternalMssg);
    s->setValue(QLatin1String(omitRunConfigWarnKey), omitRunConfigWarn);
    s->setValue(QLatin1String(limitResultOutputKey), limitResultOutput);
    s->setValue(QLatin1String(autoScrollKey), autoScroll);
    s->setValue(QLatin1String(alwaysParseKey), alwaysParse);
    // store frameworks and their current active state
    for (const Core::Id &id : frameworks.keys())
        s->setValue(QLatin1String(id.name()), frameworks.value(id));

    s->beginGroup("QtTest");
    qtTestSettings.toSettings(s);
    s->endGroup();
    s->beginGroup("GTest");
    gTestSettings.toSettings(s);
    s->endGroup();

    s->endGroup();
}

void TestSettings::fromSettings(QSettings *s)
{
    s->beginGroup(group);
    timeout = s->value(QLatin1String(timeoutKey), defaultTimeout).toInt();
    omitInternalMssg = s->value(QLatin1String(omitInternalKey), true).toBool();
    omitRunConfigWarn = s->value(QLatin1String(omitRunConfigWarnKey), false).toBool();
    limitResultOutput = s->value(QLatin1String(limitResultOutputKey), true).toBool();
    autoScroll = s->value(QLatin1String(autoScrollKey), true).toBool();
    alwaysParse = s->value(QLatin1String(alwaysParseKey), true).toBool();
    // try to get settings for registered frameworks
    TestFrameworkManager *frameworkManager = TestFrameworkManager::instance();
    const QList<Core::Id> &registered = frameworkManager->registeredFrameworkIds();
    frameworks.clear();
    for (const Core::Id &id : registered) {
        frameworks.insert(id, s->value(QLatin1String(id.name()),
                                       frameworkManager->isActive(id)).toBool());
    }

    s->beginGroup("QtTest");
    qtTestSettings.fromSettings(s);
    s->endGroup();
    s->beginGroup("GTest");
    gTestSettings.fromSettings(s);
    s->endGroup();

    s->endGroup();
}

} // namespace Internal
} // namespace Autotest
