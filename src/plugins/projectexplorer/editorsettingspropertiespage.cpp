// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "editorsettingspropertiespage.h"

#include "editorconfiguration.h"
#include "project.h"
#include "projectexplorertr.h"
#include "projectpanelfactory.h"

#include <texteditor/texteditorconstants.h>
#include <texteditor/tabsettings.h>

#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>

using namespace Utils;

namespace ProjectExplorer::Internal {

class EditorSettingsWidget : public QWidget
{
public:
    explicit EditorSettingsWidget(Project *project);

private:
    QCheckBox m_globalCheckBox;
    QPushButton m_restoreButton{Tr::tr("Restore Global")};
    QGroupBox m_displaySettings{Tr::tr("Display Settings")};
    QWidget m_behaviorSettings;
    TextEditor::TabSettings m_tabSettings;
};

EditorSettingsWidget::EditorSettingsWidget(Project *project)
{
    EditorConfiguration *config = project->editorConfiguration();
    const bool initial = config->useGlobalSettings();

    using namespace Layouting;

    Column {
        &m_tabSettings,
        &config->typingSettings,
        &config->storageSettings,
        &config->extraEncodingSettings,
        &config->behaviorSettings,
        st,
        noMargin,
    }.attachTo(&m_behaviorSettings);

    TextEditor::MarginSettings &m = config->marginSettings;

    Row {
        m.showMargin,
        m.tintMarginArea,
        m.marginColumn,
        m.useIndenter,
        st
    }.attachTo(&m_displaySettings);

    const auto updateEnabled = [this](bool useGlobal) {
        m_displaySettings.setEnabled(!useGlobal);
        m_behaviorSettings.setEnabled(!useGlobal);
        m_restoreButton.setEnabled(!useGlobal);
    };

    m_globalCheckBox.setChecked(initial);
    connect(&m_globalCheckBox, &QCheckBox::toggled, this, [config, updateEnabled](bool useGlobal) {
        updateEnabled(useGlobal);
        config->setUseGlobalSettings(useGlobal);
    });

    Column {
        Row {
            &m_globalCheckBox,
            createUseGlobalSettingsLabel(TextEditor::Constants::TEXT_EDITOR_BEHAVIOR_SETTINGS),
            st
        },
        createHr(),
        Row { &m_restoreButton, st },
        &m_displaySettings,
        &m_behaviorSettings,
        st,
        noMargin
    }.attachTo(this);

    m_tabSettings.setPreferences(config->codeStyle());
    updateEnabled(initial);

    connect(&m_restoreButton, &QAbstractButton::clicked,
            this, [this, config] {
        config->cloneGlobalSettings();
        m_tabSettings.setPreferences(config->codeStyle());
    });
}

class EditorSettingsProjectPanelFactory final : public ProjectPanelFactory
{
public:
    EditorSettingsProjectPanelFactory()
    {
        setPriority(30);
        setDisplayName(Tr::tr("Editor"));
        setCreateWidgetFunction([](Project *project) { return new EditorSettingsWidget(project); });
    }
};

void setupEditorSettingsProjectPanel()
{
    static EditorSettingsProjectPanelFactory theEditorSettingsProjectPanelFactory;
}

} // ProjectExplorer::Internal
