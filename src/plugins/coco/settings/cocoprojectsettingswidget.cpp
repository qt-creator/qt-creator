// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cocoprojectsettingswidget.h"

#include "cocobuild/cocoprojectwidget.h"
#include "cocopluginconstants.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <QDebug>

namespace Coco::Internal {

CocoProjectSettingsWidget::CocoProjectSettingsWidget(ProjectExplorer::Project *project)
    : m_layout{new QVBoxLayout}
{
    setUseGlobalSettingsCheckBoxVisible(false);
    setGlobalSettingsId(Constants::COCO_SETTINGS_PAGE_ID);

    if (auto *target = project->activeTarget()) {
        auto abc = target->activeBuildConfiguration();

        if (abc->id() == QmakeProjectManager::Constants::QMAKE_BC_ID
            || abc->id() == CMakeProjectManager::Constants::CMAKE_BUILDCONFIGURATION_ID)
            m_layout->addWidget(new CocoProjectWidget(project, *abc));
    }
    setLayout(m_layout);
}

CocoProjectSettingsWidget::~CocoProjectSettingsWidget()
{
    delete m_layout;
}

} // namespace Coco::Internal
