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

#include "codestylesettingspropertiespage.h"
#include "editorconfiguration.h"
#include "project.h"
#include <texteditor/texteditorsettings.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/codestyleeditor.h>

using namespace TextEditor;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

CodeStyleSettingsWidget::CodeStyleSettingsWidget(Project *project) : QWidget(), m_project(project)
{
    m_ui.setupUi(this);

    const EditorConfiguration *config = m_project->editorConfiguration();

    QMap<Core::Id, ICodeStylePreferencesFactory *> factories
            = TextEditorSettings::codeStyleFactories();
    QMapIterator<Core::Id, ICodeStylePreferencesFactory *> it(factories);
    while (it.hasNext()) {
        it.next();
        ICodeStylePreferencesFactory *factory = it.value();
        Core::Id languageId = factory->languageId();
        ICodeStylePreferences *codeStylePreferences = config->codeStyle(languageId);

        auto preview = new CodeStyleEditor(factory, codeStylePreferences, m_ui.stackedWidget);
        preview->clearMargins();
        m_ui.stackedWidget->addWidget(preview);
        m_ui.languageComboBox->addItem(factory->displayName());
    }

    connect(m_ui.languageComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            m_ui.stackedWidget, &QStackedWidget::setCurrentIndex);
}

