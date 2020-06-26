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

#include <utils/id.h>

#include <QSettings>

namespace Autotest {
namespace Internal {

static const char timeoutKey[]              = "Timeout";
static const char omitInternalKey[]         = "OmitInternal";
static const char omitRunConfigWarnKey[]    = "OmitRCWarnings";
static const char limitResultOutputKey[]    = "LimitResultOutput";
static const char autoScrollKey[]           = "AutoScrollResults";
static const char processArgsKey[]          = "ProcessArgs";
static const char displayApplicationKey[]   = "DisplayApp";
static const char popupOnStartKey[]         = "PopupOnStart";
static const char popupOnFinishKey[]        = "PopupOnFinish";
static const char popupOnFailKey[]          = "PopupOnFail";
static const char runAfterBuildKey[]        = "RunAfterBuild";
static const char groupSuffix[]             = ".group";

constexpr int defaultTimeout = 60000;

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
    s->setValue(displayApplicationKey, displayApplication);
    s->setValue(popupOnStartKey, popupOnStart);
    s->setValue(popupOnFinishKey, popupOnFinish);
    s->setValue(popupOnFailKey, popupOnFail);
    s->setValue(runAfterBuildKey, int(runAfterBuild));
    // store frameworks and their current active and grouping state
    for (const Utils::Id &id : frameworks.keys()) {
        s->setValue(id.toString(), frameworks.value(id));
        s->setValue(id.toString() + groupSuffix, frameworksGrouping.value(id));
    }
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
    displayApplication = s->value(displayApplicationKey, false).toBool();
    popupOnStart = s->value(popupOnStartKey, true).toBool();
    popupOnFinish = s->value(popupOnFinishKey, true).toBool();
    popupOnFail = s->value(popupOnFailKey, false).toBool();
    runAfterBuild = RunAfterBuildMode(s->value(runAfterBuildKey,
                                               int(RunAfterBuildMode::None)).toInt());
    // try to get settings for registered frameworks
    const TestFrameworks &registered = TestFrameworkManager::registeredFrameworks();
    frameworks.clear();
    frameworksGrouping.clear();
    for (const ITestFramework *framework : registered) {
        // get their active state
        const Utils::Id id = framework->id();
        const QString key = id.toString();
        frameworks.insert(id, s->value(key, framework->active()).toBool());
        // and whether grouping is enabled
        frameworksGrouping.insert(id, s->value(key + groupSuffix, framework->grouping()).toBool());
    }
    s->endGroup();
}

} // namespace Internal
} // namespace Autotest
