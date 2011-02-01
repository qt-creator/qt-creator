/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "editorsettingspropertiespage.h"
#include "editorconfiguration.h"
#include "project.h"

#include <QtCore/QTextCodec>

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

bool EditorSettingsPanelFactory::supports(Project *project)
{
    Q_UNUSED(project);
    return true;
}

IPropertiesPanel *EditorSettingsPanelFactory::createPanel(Project *project)
{
    return new EditorSettingsPanel(project);
}

EditorSettingsPanel::EditorSettingsPanel(Project *project) :
    m_widget(new EditorSettingsWidget(project)),
    m_icon(":/projectexplorer/images/EditorSettings.png")
{
}

EditorSettingsPanel::~EditorSettingsPanel()
{
    delete m_widget;
}

QString EditorSettingsPanel::displayName() const
{
    return QCoreApplication::translate("EditorSettingsPanel", "Editor Settings");
}

QWidget *EditorSettingsPanel::widget() const
{
    return m_widget;
}

QIcon EditorSettingsPanel::icon() const
{
    return m_icon;
}

EditorSettingsWidget::EditorSettingsWidget(Project *project) : QWidget(), m_project(project)
{
    m_ui.setupUi(this);

    const EditorConfiguration *config = m_project->editorConfiguration();
    settingsToUi(config);

    setGlobalSettingsEnabled(config->useGlobalSettings());

    connect(m_ui.useGlobalCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(setGlobalSettingsEnabled(bool)));
    connect(m_ui.useGlobalCheckBox, SIGNAL(clicked(bool)),
            config, SLOT(setUseGlobalSettings(bool)));
    connect(m_ui.restoreButton, SIGNAL(clicked()), this, SLOT(restoreDefaultValues()));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(insertSpacesChanged(bool)),
            config, SLOT(setInsertSpaces(bool)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(autoInsertSpacesChanged(bool)),
            config, SLOT(setAutoInsertSpaces(bool)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(autoIndentChanged(bool)),
            config, SLOT(setAutoIndent(bool)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(smartBackSpaceChanged(bool)),
            config, SLOT(setSmartBackSpace(bool)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(tabSizeChanged(int)),
            config, SLOT(setTabSize(int)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(indentSizeChanged(int)),
            config, SLOT(setIndentSize(int)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(indentBlocksBehaviorChanged(int)),
            config, SLOT(setIndentBlocksBehavior(int)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(tabKeyBehaviorChanged(int)),
            config, SLOT(setTabKeyBehavior(int)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(continuationAlignBehaviorChanged(int)),
            config, SLOT(setContinuationAlignBehavior(int)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(cleanWhiteSpaceChanged(bool)),
            config, SLOT(setCleanWhiteSpace(bool)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(inEntireDocumentChanged(bool)),
            config, SLOT(setInEntireDocument(bool)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(addFinalNewLineChanged(bool)),
            config, SLOT(setAddFinalNewLine(bool)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(cleanIndentationChanged(bool)),
            config, SLOT(setCleanIndentation(bool)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(mouseNavigationChanged(bool)),
            config, SLOT(setMouseNavigation(bool)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(scrollWheelZoomingChanged(bool)),
            config, SLOT(setScrollWheelZooming(bool)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(utf8BomSettingsChanged(int)),
            config, SLOT(setUtf8BomSettings(int)));
    connect(m_ui.behaviorSettingsWidget, SIGNAL(textCodecChanged(QTextCodec*)),
            config, SLOT(setTextCodec(QTextCodec*)));
}

void EditorSettingsWidget::settingsToUi(const EditorConfiguration *config)
{
    m_ui.useGlobalCheckBox->setChecked(config->useGlobalSettings());
    m_ui.behaviorSettingsWidget->setAssignedCodec(config->textCodec());
    m_ui.behaviorSettingsWidget->setAssignedTabSettings(config->tabSettings());
    m_ui.behaviorSettingsWidget->setAssignedStorageSettings(config->storageSettings());
    m_ui.behaviorSettingsWidget->setAssignedBehaviorSettings(config->behaviorSettings());
    m_ui.behaviorSettingsWidget->setAssignedExtraEncodingSettings(config->extraEncodingSettings());
}

void EditorSettingsWidget::setGlobalSettingsEnabled(bool enabled)
{
    m_ui.behaviorSettingsWidget->setActive(!enabled);
    m_ui.restoreButton->setEnabled(!enabled);
}

void EditorSettingsWidget::restoreDefaultValues()
{
    EditorConfiguration *config = m_project->editorConfiguration();
    config->cloneGlobalSettings();
    settingsToUi(config);
}
