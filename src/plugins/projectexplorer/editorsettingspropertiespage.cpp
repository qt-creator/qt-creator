/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

    connect(m_ui.globalSelector, SIGNAL(activated(int)),
            this, SLOT(globalSettingsActivated(int)));
    connect(m_ui.restoreButton, SIGNAL(clicked()), this, SLOT(restoreDefaultValues()));

    connect(m_ui.showWrapColumn, SIGNAL(toggled(bool)), config, SLOT(setShowWrapColumn(bool)));
    connect(m_ui.wrapColumn, SIGNAL(valueChanged(int)), config, SLOT(setWrapColumn(int)));

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
