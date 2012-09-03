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

#include "codestylesettingspropertiespage.h"
#include "editorconfiguration.h"
#include "project.h"
#include <texteditor/texteditorsettings.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/codestyleeditor.h>

#include <QTextCodec>

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

    QMap<QString, ICodeStylePreferencesFactory *> factories
            = TextEditor::TextEditorSettings::instance()->codeStyleFactories();
    QMapIterator<QString, ICodeStylePreferencesFactory *> it(factories);
    while (it.hasNext()) {
        it.next();
        ICodeStylePreferencesFactory *factory = it.value();
        const QString languageId = factory->languageId();
        ICodeStylePreferences *codeStylePreferences = config->codeStyle(languageId);

        CodeStyleEditor *preview = new CodeStyleEditor(factory, codeStylePreferences, m_ui.stackedWidget);
        preview->clearMargins();
        m_ui.stackedWidget->addWidget(preview);
        m_ui.languageComboBox->addItem(factory->displayName());
    }

    connect(m_ui.languageComboBox, SIGNAL(currentIndexChanged(int)),
            m_ui.stackedWidget, SLOT(setCurrentIndex(int)));
}

