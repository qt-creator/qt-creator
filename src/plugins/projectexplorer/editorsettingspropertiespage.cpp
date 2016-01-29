/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "editorsettingspropertiespage.h"
#include "editorconfiguration.h"
#include "project.h"
#include <texteditor/marginsettings.h>

#include <QTextCodec>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

EditorSettingsWidget::EditorSettingsWidget(Project *project) : QWidget(), m_project(project)
{
    m_ui.setupUi(this);

    const EditorConfiguration *config = m_project->editorConfiguration();
    settingsToUi(config);

    globalSettingsActivated(config->useGlobalSettings() ? 0 : 1);


    connect(m_ui.globalSelector, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, &EditorSettingsWidget::globalSettingsActivated);
    connect(m_ui.restoreButton, &QAbstractButton::clicked,
            this, &EditorSettingsWidget::restoreDefaultValues);

    connect(m_ui.showWrapColumn, &QAbstractButton::toggled,
            config, &EditorConfiguration::setShowWrapColumn);
    connect(m_ui.wrapColumn, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            config, &EditorConfiguration::setWrapColumn);

    connect(m_ui.behaviorSettingsWidget, &TextEditor::BehaviorSettingsWidget::typingSettingsChanged,
            config, &EditorConfiguration::setTypingSettings);
    connect(m_ui.behaviorSettingsWidget, &TextEditor::BehaviorSettingsWidget::storageSettingsChanged,
            config, &EditorConfiguration::setStorageSettings);
    connect(m_ui.behaviorSettingsWidget, &TextEditor::BehaviorSettingsWidget::behaviorSettingsChanged,
            config, &EditorConfiguration::setBehaviorSettings);
    connect(m_ui.behaviorSettingsWidget, &TextEditor::BehaviorSettingsWidget::extraEncodingSettingsChanged,
            config, &EditorConfiguration::setExtraEncodingSettings);
    connect(m_ui.behaviorSettingsWidget, &TextEditor::BehaviorSettingsWidget::textCodecChanged,
            config, &EditorConfiguration::setTextCodec);
}

void EditorSettingsWidget::settingsToUi(const EditorConfiguration *config)
{
    m_ui.showWrapColumn->setChecked(config->marginSettings().m_showMargin);
    m_ui.wrapColumn->setValue(config->marginSettings().m_marginColumn);
    m_ui.behaviorSettingsWidget->setCodeStyle(config->codeStyle());
    m_ui.globalSelector->setCurrentIndex(config->useGlobalSettings() ? 0 : 1);
    m_ui.behaviorSettingsWidget->setAssignedCodec(config->textCodec());
    m_ui.behaviorSettingsWidget->setAssignedTypingSettings(config->typingSettings());
    m_ui.behaviorSettingsWidget->setAssignedStorageSettings(config->storageSettings());
    m_ui.behaviorSettingsWidget->setAssignedBehaviorSettings(config->behaviorSettings());
    m_ui.behaviorSettingsWidget->setAssignedExtraEncodingSettings(config->extraEncodingSettings());
}

void EditorSettingsWidget::globalSettingsActivated(int index)
{
    const bool useGlobal = !index;
    m_ui.displaySettings->setEnabled(!useGlobal);
    m_ui.behaviorSettingsWidget->setActive(!useGlobal);
    m_ui.restoreButton->setEnabled(!useGlobal);
    EditorConfiguration *config = m_project->editorConfiguration();
    config->setUseGlobalSettings(useGlobal);
}

void EditorSettingsWidget::restoreDefaultValues()
{
    EditorConfiguration *config = m_project->editorConfiguration();
    config->cloneGlobalSettings();
    settingsToUi(config);
}
