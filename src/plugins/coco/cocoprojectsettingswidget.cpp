// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cocoprojectsettingswidget.h"

#include "cocopluginconstants.h"
#include "cocoprojectwidget.h"
#include "cocotr.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectsettingswidget.h>
#include <projectexplorer/target.h>

#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <QVBoxLayout>
#include <QDebug>

using namespace ProjectExplorer;

namespace Coco::Internal {

class CocoProjectSettingsWidget final : public ProjectSettingsWidget
{
public:
    explicit CocoProjectSettingsWidget(Project *project)
    {
        setUseGlobalSettingsCheckBoxVisible(false);
        setGlobalSettingsId(Constants::COCO_SETTINGS_PAGE_ID);

        auto layout = new QVBoxLayout;
        if (auto *target = project->activeTarget()) {
            auto abc = target->activeBuildConfiguration();

            if (abc->id() == QmakeProjectManager::Constants::QMAKE_BC_ID
                    || abc->id() == CMakeProjectManager::Constants::CMAKE_BUILDCONFIGURATION_ID)
                layout->addWidget(new CocoProjectWidget(project, abc));
        }
        setLayout(layout);
    }
};

class CocoProjectPanelFactory final : public ProjectPanelFactory
{
public:
    CocoProjectPanelFactory()
    {
        setPriority(50);
        setDisplayName(Tr::tr("Coco Code Coverage"));
        setSupportsFunction([](Project *project) {
            if (Target *target = project->activeTarget()) {
                if (BuildConfiguration *abc = target->activeBuildConfiguration())
                    return BuildSettings::supportsBuildConfig(*abc);
            }
            return false;
        });
        setCreateWidgetFunction(
                    [](Project *project) { return new CocoProjectSettingsWidget(project); });
    }
};

void setupCocoProjectPanel()
{
    static CocoProjectPanelFactory theCocoProjectPanelFactory;
}

} // namespace Coco::Internal
