// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "copilotprojectpanel.h"

#include "copilotconstants.h"
#include "copilotsettings.h"
#include "copilottr.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>

#include <utils/layoutbuilder.h>

#include <QCheckBox>

using namespace ProjectExplorer;

namespace Copilot::Internal {

class CopilotProjectWidget final : public QWidget
{
public:
    CopilotProjectWidget(Project *project)
    {
        auto settings = new CopilotProjectSettings(project);
        settings->setParent(this);
        const bool initial = settings->useGlobalSettings();
        setEnabled(!initial);
        m_globalCheckBox.setChecked(initial);
        connect(&m_globalCheckBox, &QCheckBox::toggled, this, [this, settings](bool useGlobal) {
            settings->setUseGlobalSettings(useGlobal);
            setEnabled(!useGlobal);
        });

        // clang-format off
        using namespace Layouting;
        Column {
            Row {
                &m_globalCheckBox,
                createUseGlobalSettingsLabel(Constants::COPILOT_GENERAL_OPTIONS_ID),
                st
            },
            createHr(),
            settings->enableCopilot,
            st,
        }.attachTo(this);
        // clang-format on
    }

    QCheckBox m_globalCheckBox;
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
