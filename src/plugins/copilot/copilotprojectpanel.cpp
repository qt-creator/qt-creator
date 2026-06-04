// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "copilotprojectpanel.h"

#include "copilotconstants.h"
#include "copilotsettings.h"
#include "copilottr.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>

#include <utils/layoutbuilder.h>

using namespace ProjectExplorer;

namespace Copilot::Internal {

class CopilotProjectWidget final : public QWidget
{
public:
    CopilotProjectWidget(Project *project)
        : m_settings(project)
    {
        m_settings.enableCopilot.setEnabled(!m_settings.useGlobalSettings());
        m_settings.useGlobalSettings.addOnChanged(this, [this] {
            m_settings.enableCopilot.setEnabled(!m_settings.useGlobalSettings());
        });

        // clang-format off
        using namespace Layouting;
        Column {
            Row {
                m_settings.useGlobalSettings,
                createUseGlobalSettingsLabel(Constants::COPILOT_GENERAL_OPTIONS_ID),
                st
            },
            createHr(),
            m_settings.enableCopilot,
            st,
        }.attachTo(this);
        // clang-format on
    }

    CopilotProjectSettings m_settings;
};

class CopilotProjectPanelFactory final : public ProjectPanelFactory
{
public:
    CopilotProjectPanelFactory()
    {
        setPriority(1000);
        setDisplayName(Tr::tr("Copilot"));
        setCreateWidgetFunction([](Project *project) {
            return new CopilotProjectWidget(project);
        });
    }
};

void setupCopilotProjectPanel()
{
    static CopilotProjectPanelFactory theCopilotProjectPanelFactory;
}

} // namespace Copilot::Internal
