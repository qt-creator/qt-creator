/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "testprojectsettings.h"
#include "autotestconstants.h"
#include "testframeworkmanager.h"

#include <projectexplorer/session.h>
#include <utils/algorithm.h>

namespace Autotest {
namespace Internal {

static const char SK_ACTIVE_FRAMEWORKS[]        = "AutoTest.ActiveFrameworks";
static const char SK_RUN_AFTER_BUILD[]          = "AutoTest.RunAfterBuild";

TestProjectSettings::TestProjectSettings(ProjectExplorer::Project *project)
    : m_project(project)
{
    load();
    connect(project, &ProjectExplorer::Project::settingsLoaded,
            this, &TestProjectSettings::load);
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings,
            this, &TestProjectSettings::save);
}

TestProjectSettings::~TestProjectSettings()
{
    save();
}

void TestProjectSettings::setUseGlobalSettings(bool useGlobal)
{
    if (m_useGlobalSettings == useGlobal)
        return;
     m_useGlobalSettings = useGlobal;
}

void TestProjectSettings::activateFramework(const Core::Id &id, bool activate)
{
    if (m_activeTestFrameworks.value(id) != activate) {
        m_activeTestFrameworks[id] = activate;
    }
}

void TestProjectSettings::load()
{
    const QVariant useGlobal = m_project->namedSettings(Constants::SK_USE_GLOBAL);
    m_useGlobalSettings = useGlobal.isValid() ? useGlobal.toBool() : true;

    TestFrameworkManager *frameworkManager = TestFrameworkManager::instance();
    const QList<Core::Id> registered = frameworkManager->sortedRegisteredFrameworkIds();
    const QVariant activeFrameworks = m_project->namedSettings(SK_ACTIVE_FRAMEWORKS);

    m_activeTestFrameworks.clear();
    if (activeFrameworks.isValid()) {
        const QMap<QString, QVariant> frameworksMap = activeFrameworks.toMap();
        for (const Core::Id &id : registered) {
            const QString idStr = id.toString();
            bool active = frameworksMap.value(idStr, frameworkManager->isActive(id)).toBool();
            m_activeTestFrameworks.insert(id, active);
        }
    } else {
        for (const Core::Id &id : registered)
            m_activeTestFrameworks.insert(id, frameworkManager->isActive(id));
    }

    const QVariant runAfterBuild = m_project->namedSettings(SK_RUN_AFTER_BUILD);
    m_runAfterBuild = runAfterBuild.isValid() ? RunAfterBuildMode(runAfterBuild.toInt())
                                              : RunAfterBuildMode::None;
}

void TestProjectSettings::save()
{
    m_project->setNamedSettings(Constants::SK_USE_GLOBAL, m_useGlobalSettings);
    QVariantMap activeFrameworks;
    auto end = m_activeTestFrameworks.cend();
    for (auto it = m_activeTestFrameworks.cbegin(); it != end; ++it)
        activeFrameworks.insert(it.key().toString(), it.value());
    m_project->setNamedSettings(SK_ACTIVE_FRAMEWORKS, activeFrameworks);
    m_project->setNamedSettings(SK_RUN_AFTER_BUILD, int(m_runAfterBuild));
}

} // namespace Internal
} // namespace Autotest
