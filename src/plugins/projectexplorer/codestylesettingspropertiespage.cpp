/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "codestylesettingspropertiespage.h"
#include "editorconfiguration.h"
#include "project.h"
#include <texteditor/codestylepreferencesmanager.h>
#include <texteditor/icodestylepreferencesfactory.h>

#include <QtCore/QTextCodec>

using namespace TextEditor;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

QString CodeStyleSettingsPanelFactory::id() const
{
    return QLatin1String(CODESTYLESETTINGS_PANEL_ID);
}

QString CodeStyleSettingsPanelFactory::displayName() const
{
    return QCoreApplication::translate("CodeStyleSettingsPanelFactory", "Code Style Settings");
}

bool CodeStyleSettingsPanelFactory::supports(Project *project)
{
    Q_UNUSED(project);
    return true;
}

PropertiesPanel *CodeStyleSettingsPanelFactory::createPanel(Project *project)
{
    PropertiesPanel *panel = new PropertiesPanel;
    panel->setWidget(new CodeStyleSettingsWidget(project));
    panel->setIcon(QIcon(":/projectexplorer/images/CodeStyleSettings.png"));
    panel->setDisplayName(QCoreApplication::translate("CodeStyleSettingsPanel", "Code Style Settings"));
    return panel;
}

CodeStyleSettingsWidget::CodeStyleSettingsWidget(Project *project) : QWidget(), m_project(project)
{
    m_ui.setupUi(this);

    const EditorConfiguration *config = m_project->editorConfiguration();
    CodeStylePreferencesManager *manager =
            CodeStylePreferencesManager::instance();

    QList<ICodeStylePreferencesFactory *> factories = manager->factories();
    for (int i = 0; i < factories.count(); i++) {
        ICodeStylePreferencesFactory *factory = factories.at(i);
        const QString languageId = factory->languageId();
        TabPreferences *tabPreferences = config->tabPreferences(languageId);
        IFallbackPreferences *codeStylePreferences = config->codeStylePreferences(languageId);

        QWidget *widget = factory->createEditor(codeStylePreferences, tabPreferences, m_ui.stackedWidget);
        m_ui.stackedWidget->addWidget(widget);
        m_ui.languageComboBox->addItem(factory->displayName());
    }

    connect(m_ui.languageComboBox, SIGNAL(currentIndexChanged(int)),
            m_ui.stackedWidget, SLOT(setCurrentIndex(int)));
}


