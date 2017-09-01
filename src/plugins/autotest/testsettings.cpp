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
#include "autotestconstants.h"
#include "testframeworkmanager.h"

#include <coreplugin/id.h>

#include <QSettings>

namespace Autotest {
namespace Internal {

static const char timeoutKey[]              = "Timeout";
static const char omitInternalKey[]         = "OmitInternal";
static const char omitRunConfigWarnKey[]    = "OmitRCWarnings";
static const char limitResultOutputKey[]    = "LimitResultOutput";
static const char autoScrollKey[]           = "AutoScrollResults";
static const char filterScanKey[]           = "FilterScan";
static const char filtersKey[]              = "WhiteListFilters";
static const char processArgsKey[]          = "ProcessArgs";

static const int defaultTimeout = 60000;

TestSettings::TestSettings()
    : timeout(defaultTimeout)
{
}

void TestSettings::toSettings(QSettings *s) const
{
    s->beginGroup(Constants::SETTINGSGROUP);
    s->setValue(timeoutKey, timeout);
    s->setValue(omitInternalKey, omitInternalMssg);
    s->setValue(omitRunConfigWarnKey, omitRunConfigWarn);
    s->setValue(limitResultOutputKey, limitResultOutput);
    s->setValue(autoScrollKey, autoScroll);
    s->setValue(processArgsKey, processArgs);
    s->setValue(filterScanKey, filterScan);
    s->setValue(filtersKey, whiteListFilters);
    // store frameworks and their current active state
    for (const Core::Id &id : frameworks.keys())
        s->setValue(QLatin1String(id.name()), frameworks.value(id));
    s->endGroup();
}

void TestSettings::fromSettings(QSettings *s)
{
    s->beginGroup(Constants::SETTINGSGROUP);
    timeout = s->value(timeoutKey, defaultTimeout).toInt();
    omitInternalMssg = s->value(omitInternalKey, true).toBool();
    omitRunConfigWarn = s->value(omitRunConfigWarnKey, false).toBool();
    limitResultOutput = s->value(limitResultOutputKey, true).toBool();
    autoScroll = s->value(autoScrollKey, true).toBool();
    processArgs = s->value(processArgsKey, false).toBool();
    filterScan = s->value(filterScanKey, false).toBool();
    whiteListFilters = s->value(filtersKey, QStringList()).toStringList();
    // try to get settings for registered frameworks
    TestFrameworkManager *frameworkManager = TestFrameworkManager::instance();
    const QList<Core::Id> &registered = frameworkManager->registeredFrameworkIds();
    frameworks.clear();
    for (const Core::Id &id : registered) {
        frameworks.insert(id, s->value(QLatin1String(id.name()),
                                       frameworkManager->isActive(id)).toBool());
    }
    s->endGroup();
}

} // namespace Internal
} // namespace Autotest
