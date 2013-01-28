/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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
    return QCoreApplication::translate("EditorSettingsPanelFactory", "Editor");
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
    panel->setDisplayName(QCoreApplication::translate("EditorSettingsPanel", "Editor"));
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
