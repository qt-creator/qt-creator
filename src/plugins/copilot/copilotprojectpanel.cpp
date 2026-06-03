// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "copilotprojectpanel.h"

#include "copilotconstants.h"
#include "copilotsettings.h"
#include "copilottr.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectsettingswidget.h>

#include <utils/layoutbuilder.h>

using namespace ProjectExplorer;

namespace Copilot::Internal {

static QWidget *createCopilotProjectPanel(Project *project)
{
    using namespace Layouting;

    auto widget = new QWidget;
    auto settings = new CopilotProjectSettings(project);
    settings->setParent(widget);

    widget->setEnabled(!settings->useGlobalSettings());

    // clang-format off
    Column {
        createGlobalOrProjectSelector(widget, settings->useGlobalSettings(),
            Constants::COPILOT_GENERAL_OPTIONS_ID,
            [widget, settings](bool useGlobal) {
                settings->setUseGlobalSettings(useGlobal);
                widget->setEnabled(!useGlobal);
            }),
        settings->enableCopilot,
        st,
    }.attachTo(widget);
    // clang-format on

    return widget;
}

class CopilotProjectPanelFactory final : public ProjectPanelFactory
{
public:
    CopilotProjectPanelFactory()
    {
        setPriority(1000);
        setDisplayName(Tr::tr("Copilot"));
        setCreateWidgetFunction(&createCopilotProjectPanel);
    }
};

void setupCopilotProjectPanel()
{
    static CopilotProjectPanelFactory theCopilotProjectPanelFactory;
}

} // namespace Copilot::Internal
