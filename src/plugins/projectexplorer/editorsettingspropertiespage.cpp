// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "editorsettingspropertiespage.h"

#include "editorconfiguration.h"
#include "project.h"
#include "projectexplorertr.h"

#include <texteditor/texteditorconstants.h>
#include <texteditor/behaviorsettings.h>
#include <texteditor/behaviorsettingswidget.h>
#include <texteditor/extraencodingsettings.h>
#include <texteditor/marginsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/typingsettings.h>

#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTextCodec>

namespace ProjectExplorer::Internal {

EditorSettingsWidget::EditorSettingsWidget(Project *project) : m_project(project)
{
    setGlobalSettingsId(TextEditor::Constants::TEXT_EDITOR_BEHAVIOR_SETTINGS);

    m_restoreButton = new QPushButton(Tr::tr("Restore Global"));

    m_displaySettings = new QGroupBox(Tr::tr("Display Settings"));
    m_displaySettings->setEnabled(false);

    m_showWrapColumn = new QCheckBox(Tr::tr("Display right &margin at column:"));

    m_tintMarginArea = new QCheckBox("Colorize right margin area");

    m_wrapColumn = new QSpinBox(m_displaySettings);
    m_wrapColumn->setEnabled(false);
    m_wrapColumn->setMaximum(999);

    m_useIndenter = new QCheckBox(Tr::tr("Use context-specific margin"));
    m_useIndenter->setToolTip(Tr::tr("If available, use a different margin. For example, "
                                 "the ColumnLimit from the ClangFormat plugin."));

    m_behaviorSettings = new TextEditor::BehaviorSettingsWidget(this);

    using namespace Layouting;

    Row {
        m_showWrapColumn,
        m_tintMarginArea,
        m_wrapColumn,
        m_useIndenter,
        st
    }.attachTo(m_displaySettings);

    Column {
        Row { m_restoreButton, st },
        m_displaySettings,
        m_behaviorSettings,
        st,
        noMargin
    }.attachTo(this);

    const EditorConfiguration *config = m_project->editorConfiguration();
    settingsToUi(config);

    globalSettingsActivated(config->useGlobalSettings());
    setUseGlobalSettings(config->useGlobalSettings());

    connect(m_showWrapColumn, &QCheckBox::toggled,
            m_wrapColumn, &QSpinBox::setEnabled);

    connect(m_tintMarginArea, &QCheckBox::toggled,
            m_tintMarginArea, &QSpinBox::setEnabled);

    connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged,
            this, &EditorSettingsWidget::globalSettingsActivated);

    connect(m_restoreButton, &QAbstractButton::clicked,
            this, &EditorSettingsWidget::restoreDefaultValues);

    connect(m_showWrapColumn, &QAbstractButton::toggled,
            config, &EditorConfiguration::setShowWrapColumn);
    connect(m_tintMarginArea, &QAbstractButton::toggled,
            config, &EditorConfiguration::setTintMarginArea);
    connect(m_useIndenter, &QAbstractButton::toggled,
            config, &EditorConfiguration::setUseIndenter);
    connect(m_wrapColumn, &QSpinBox::valueChanged,
            config, &EditorConfiguration::setWrapColumn);

    connect(m_behaviorSettings, &TextEditor::BehaviorSettingsWidget::typingSettingsChanged,
            config, &EditorConfiguration::setTypingSettings);
    connect(m_behaviorSettings, &TextEditor::BehaviorSettingsWidget::storageSettingsChanged,
            config, &EditorConfiguration::setStorageSettings);
    connect(m_behaviorSettings, &TextEditor::BehaviorSettingsWidget::behaviorSettingsChanged,
            config, &EditorConfiguration::setBehaviorSettings);
    connect(m_behaviorSettings, &TextEditor::BehaviorSettingsWidget::extraEncodingSettingsChanged,
            config, &EditorConfiguration::setExtraEncodingSettings);
    connect(m_behaviorSettings, &TextEditor::BehaviorSettingsWidget::textCodecChanged,
            config, &EditorConfiguration::setTextCodec);
}

void EditorSettingsWidget::settingsToUi(const EditorConfiguration *config)
{
    m_showWrapColumn->setChecked(config->marginSettings().m_showMargin);
    m_tintMarginArea->setChecked(config->marginSettings().m_tintMarginArea);
    m_useIndenter->setChecked(config->marginSettings().m_useIndenter);
    m_wrapColumn->setValue(config->marginSettings().m_marginColumn);
    m_behaviorSettings->setCodeStyle(config->codeStyle());
    m_behaviorSettings->setAssignedCodec(config->textCodec());
    m_behaviorSettings->setAssignedTypingSettings(config->typingSettings());
    m_behaviorSettings->setAssignedStorageSettings(config->storageSettings());
    m_behaviorSettings->setAssignedBehaviorSettings(config->behaviorSettings());
    m_behaviorSettings->setAssignedExtraEncodingSettings(config->extraEncodingSettings());
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

} // ProjectExplorer::Internal
