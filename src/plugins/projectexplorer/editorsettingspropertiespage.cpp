/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "editorsettingspropertiespage.h"
#include "editorconfiguration.h"
#include "project.h"

#include <QTextCodec>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

QString EditorSettingsPanelFactory::id() const
{
    return QLatin1String(EDITORSETTINGS_PANEL_ID);
}

QString EditorSettingsPanelFactory::displayName() const
{
    return QCoreApplication::translate("EditorSettingsPanelFactory", "Editor Settings");
}

int EditorSettingsPanelFactory::priority() const
{
    return 30;
}

bool EditorSettingsPanelFactory::supports(Project *project)
{
    Q_UNUSED(project);
    return true;
}

PropertiesPanel *EditorSettingsPanelFactory::createPanel(Project *project)
{
    PropertiesPanel *panel = new PropertiesPanel;
    panel->setDisplayName(QCoreApplication::translate("EditorSettingsPanel", "Editor Settings"));
    panel->setWidget(new EditorSettingsWidget(project)),
    panel->setIcon(QIcon(QLatin1String(":/projectexplorer/images/EditorSettings.png")));
    return panel;
}

EditorSettingsWidget::EditorSettingsWidget(Project *project) : QWidget(), m_project(project)
{
    m_ui.setupUi(this);

    const EditorConfiguration *config = m_project->editorConfiguration();
    settingsToUi(config);

    globalSettingsActivated(config->useGlobalSettings() ? 0 : 1);

    connect(m_ui.globalSelector, SIGNAL(activated(int)),
            this, SLOT(globalSettingsActivated(int)));
    connect(m_ui.restoreButton, SIGNAL(clicked()), this, SLOT(restoreDefaultValues()));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(typingSettingsChanged(TextEditor::TypingSettings)),
            config, SLOT(setTypingSettings(TextEditor::TypingSettings)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(storageSettingsChanged(TextEditor::StorageSettings)),
            config, SLOT(setStorageSettings(TextEditor::StorageSettings)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(behaviorSettingsChanged(TextEditor::BehaviorSettings)),
            config, SLOT(setBehaviorSettings(TextEditor::BehaviorSettings)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(extraEncodingSettingsChanged(TextEditor::ExtraEncodingSettings)),
            config, SLOT(setExtraEncodingSettings(TextEditor::ExtraEncodingSettings)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(textCodecChanged(QTextCodec*)),
            config, SLOT(setTextCodec(QTextCodec*)));
}

void EditorSettingsWidget::settingsToUi(const EditorConfiguration *config)
{
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
