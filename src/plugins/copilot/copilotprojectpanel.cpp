// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "copilotprojectpanel.h"
#include "copilotconstants.h"
#include "copilotsettings.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectsettingswidget.h>

#include <utils/layoutbuilder.h>

#include <QWidget>

using namespace ProjectExplorer;

namespace Copilot::Internal {

class CopilotProjectSettingsWidget : public ProjectExplorer::ProjectSettingsWidget
{
public:
    CopilotProjectSettingsWidget()
    {
        setGlobalSettingsId(Constants::COPILOT_GENERAL_OPTIONS_ID);
        setUseGlobalSettingsCheckBoxVisible(true);
    }
};

ProjectSettingsWidget *createCopilotProjectPanel(Project *project)
{
    using namespace Layouting;
    using namespace ProjectExplorer;

    auto widget = new CopilotProjectSettingsWidget;
    auto settings = new CopilotProjectSettings(project);
    settings->setParent(widget);

    QObject::connect(widget,
                     &ProjectSettingsWidget::useGlobalSettingsChanged,
                     settings,
                     &CopilotProjectSettings::setUseGlobalSettings);

    widget->setUseGlobalSettings(settings->useGlobalSettings());
    widget->setEnabled(!settings->useGlobalSettings());

    QObject::connect(widget,
                     &ProjectSettingsWidget::useGlobalSettingsChanged,
                     widget,
                     [widget](bool useGlobal) { widget->setEnabled(!useGlobal); });

    // clang-format off
    Column {
        settings->enableCopilot,
    }.attachTo(widget);
    // clang-format on

    return widget;
}

} // namespace Copilot::Internal
