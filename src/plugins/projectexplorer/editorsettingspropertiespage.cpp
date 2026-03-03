// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "editorsettingspropertiespage.h"

#include "editorconfiguration.h"
#include "project.h"
#include "projectexplorertr.h"
#include "projectpanelfactory.h"
#include "projectsettingswidget.h"

#include <texteditor/texteditorconstants.h>
#include <texteditor/behaviorsettings.h>
#include <texteditor/behaviorsettingswidget.h>
#include <texteditor/extraencodingsettings.h>
#include <texteditor/marginsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/typingsettings.h>

#include <utils/layoutbuilder.h>
#include <utils/textcodec.h>

#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QSpinBox>

using namespace Utils;

namespace ProjectExplorer::Internal {

class EditorSettingsWidget : public ProjectSettingsWidget
{
public:
    explicit EditorSettingsWidget(Project *project);

private:
    void globalSettingsActivated(bool useGlobal);
    void restoreDefaultValues();

    void settingsToUi(const EditorConfiguration *config);

    Project *m_project;

    QPushButton *m_restoreButton;
    QGroupBox *m_displaySettings;
    TextEditor::BehaviorSettingsWidget *m_behaviorSettings;
};

EditorSettingsWidget::EditorSettingsWidget(Project *project) : m_project(project)
{
    setGlobalSettingsId(TextEditor::Constants::TEXT_EDITOR_BEHAVIOR_SETTINGS);

    m_restoreButton = new QPushButton(Tr::tr("Restore Global"));

    m_displaySettings = new QGroupBox(Tr::tr("Display Settings"));
    m_displaySettings->setEnabled(false);

    EditorConfiguration *config = m_project->editorConfiguration();
    m_behaviorSettings = new TextEditor::BehaviorSettingsWidget(&config->typingSettings,
                                                                &config->storageSettings,
                                                                &config->behaviorSettings,
                                                                &config->extraEncodingSettings,
                                                                this);

    using namespace Layouting;

    TextEditor::MarginSettings &m = config->marginSettings;

    Row {
        m.showMargin,
        m.tintMarginArea,
        m.marginColumn,
        m.useIndenter,
        st
    }.attachTo(m_displaySettings);

    Column {
        Row { m_restoreButton, st },
        m_displaySettings,
        m_behaviorSettings,
        st,
        noMargin
    }.attachTo(this);

    settingsToUi(config);

    globalSettingsActivated(config->useGlobalSettings());
    setUseGlobalSettings(config->useGlobalSettings());

    connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged,
            this, &EditorSettingsWidget::globalSettingsActivated);

    connect(m_restoreButton, &QAbstractButton::clicked,
            this, &EditorSettingsWidget::restoreDefaultValues);

    connect(&config->extraEncodingSettings.defaultEncoding, &BaseAspect::changed, config, [config] {
            config->setTextEncoding(config->extraEncodingSettings.defaultEncoding());
    });
}

void EditorSettingsWidget::settingsToUi(const EditorConfiguration *config)
{
    m_behaviorSettings->setCodeStyle(config->codeStyle());
}

void EditorSettingsWidget::globalSettingsActivated(bool useGlobal)
{
    m_displaySettings->setEnabled(!useGlobal);
    m_behaviorSettings->setActive(!useGlobal);
    m_restoreButton->setEnabled(!useGlobal);
    EditorConfiguration *config = m_project->editorConfiguration();
    config->setUseGlobalSettings(useGlobal);
}

void EditorSettingsWidget::restoreDefaultValues()
{
    EditorConfiguration *config = m_project->editorConfiguration();
    config->cloneGlobalSettings();
    settingsToUi(config);
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
