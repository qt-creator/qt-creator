// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "editorsettingspropertiespage.h"

#include "editorconfiguration.h"
#include "project.h"
#include "projectexplorertr.h"
#include "projectpanelfactory.h"
#include "projectsettingswidget.h"

#include <texteditor/texteditorconstants.h>
#include <texteditor/tabsettings.h>

#include <utils/layoutbuilder.h>

#include <QGroupBox>
#include <QPushButton>

using namespace Utils;

namespace ProjectExplorer::Internal {

class EditorSettingsWidget : public ProjectSettingsWidget
{
public:
    explicit EditorSettingsWidget(Project *project);

private:
    QPushButton m_restoreButton{Tr::tr("Restore Global")};
    QGroupBox m_displaySettings{Tr::tr("Display Settings")};
    QWidget m_behaviorSettings;
    TextEditor::TabSettings m_tabSettings;
};

EditorSettingsWidget::EditorSettingsWidget(Project *project)
{
    setGlobalSettingsId(TextEditor::Constants::TEXT_EDITOR_BEHAVIOR_SETTINGS);

    EditorConfiguration *config = project->editorConfiguration();

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

    Column {
        createGlobalOrProjectSelector(),
        Row { &m_restoreButton, st },
        &m_displaySettings,
        &m_behaviorSettings,
        st,
        noMargin
    }.attachTo(this);

    m_tabSettings.setPreferences(config->codeStyle());

    const auto updateEnabled = [this](bool useGlobal) {
        m_displaySettings.setEnabled(!useGlobal);
        m_behaviorSettings.setEnabled(!useGlobal);
        m_restoreButton.setEnabled(!useGlobal);
    };
    updateEnabled(config->useGlobalSettings());
    setUseGlobalSettings(config->useGlobalSettings());

    connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged,
            this, [config, updateEnabled](bool useGlobal) {
        updateEnabled(useGlobal);
        config->setUseGlobalSettings(useGlobal);
    });
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
