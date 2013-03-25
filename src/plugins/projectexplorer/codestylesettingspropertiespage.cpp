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

#include "codestylesettingspropertiespage.h"
#include "editorconfiguration.h"
#include "project.h"
#include <texteditor/texteditorsettings.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/codestyleeditor.h>

using namespace TextEditor;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

QString CodeStyleSettingsPanelFactory::id() const
{
    return QLatin1String(CODESTYLESETTINGS_PANEL_ID);
}

QString CodeStyleSettingsPanelFactory::displayName() const
{
    return QCoreApplication::translate("CodeStyleSettingsPanelFactory", "Code Style");
}

int CodeStyleSettingsPanelFactory::priority() const
{
    return 40;
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
    panel->setIcon(QIcon(QLatin1String(":/projectexplorer/images/CodeStyleSettings.png")));
    panel->setDisplayName(QCoreApplication::translate("CodeStyleSettingsPanel", "Code Style"));
    return panel;
}

CodeStyleSettingsWidget::CodeStyleSettingsWidget(Project *project) : QWidget(), m_project(project)
{
    m_ui.setupUi(this);

    const EditorConfiguration *config = m_project->editorConfiguration();

    QMap<Core::Id, ICodeStylePreferencesFactory *> factories
            = TextEditor::TextEditorSettings::instance()->codeStyleFactories();
    QMapIterator<Core::Id, ICodeStylePreferencesFactory *> it(factories);
    while (it.hasNext()) {
        it.next();
        ICodeStylePreferencesFactory *factory = it.value();
        Core::Id languageId = factory->languageId();
        ICodeStylePreferences *codeStylePreferences = config->codeStyle(languageId);

        CodeStyleEditor *preview = new CodeStyleEditor(factory, codeStylePreferences, m_ui.stackedWidget);
        preview->clearMargins();
        m_ui.stackedWidget->addWidget(preview);
        m_ui.languageComboBox->addItem(factory->displayName());
    }

    connect(m_ui.languageComboBox, SIGNAL(currentIndexChanged(int)),
            m_ui.stackedWidget, SLOT(setCurrentIndex(int)));
}

