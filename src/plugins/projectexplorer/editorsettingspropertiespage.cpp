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

    m_restoreButton = new QPushButton(tr("Restore Global"));

    m_displaySettings = new QGroupBox(tr("Display Settings"));
    m_displaySettings->setEnabled(false);

    m_showWrapColumn = new QCheckBox(tr("Display right &margin at column:"));

    m_wrapColumn = new QSpinBox(m_displaySettings);
    m_wrapColumn->setEnabled(false);
    m_wrapColumn->setMaximum(999);

    m_useIndenter = new QCheckBox(tr("Use context-specific margin"));
    m_useIndenter->setToolTip(tr("If available, use a different margin. For example, "
                                 "the ColumnLimit from the ClangFormat plugin."));

    m_behaviorSettings = new TextEditor::BehaviorSettingsWidget(this);

    using namespace Utils::Layouting;

    Row {
        m_showWrapColumn,
        m_wrapColumn,
        m_useIndenter,
        Stretch()
    }.attachTo(m_displaySettings);

    Column {
        Row { m_restoreButton, Stretch() },
        m_displaySettings,
        m_behaviorSettings,
        Stretch(),
    }.attachTo(this, false);

    const EditorConfiguration *config = m_project->editorConfiguration();
    settingsToUi(config);

    globalSettingsActivated(config->useGlobalSettings());
    setUseGlobalSettings(config->useGlobalSettings());

    connect(m_showWrapColumn, &QCheckBox::toggled,
            m_wrapColumn, &QSpinBox::setEnabled);

    connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged,
            this, &EditorSettingsWidget::globalSettingsActivated);

    connect(m_restoreButton, &QAbstractButton::clicked,
            this, &EditorSettingsWidget::restoreDefaultValues);

    connect(m_showWrapColumn, &QAbstractButton::toggled,
            config, &EditorConfiguration::setShowWrapColumn);
    connect(m_useIndenter, &QAbstractButton::toggled,
            config, &EditorConfiguration::setUseIndenter);
    connect(m_wrapColumn, QOverload<int>::of(&QSpinBox::valueChanged),
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
