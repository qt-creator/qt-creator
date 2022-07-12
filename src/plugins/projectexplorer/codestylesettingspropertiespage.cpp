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

#include <utils/layoutbuilder.h>

#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QStackedWidget>

using namespace TextEditor;

namespace ProjectExplorer::Internal {

CodeStyleSettingsWidget::CodeStyleSettingsWidget(Project *project)
{
    auto languageComboBox = new QComboBox(this);
    auto stackedWidget = new QStackedWidget(this);

    setUseGlobalSettingsCheckBoxVisible(false);
    setUseGlobalSettingsLabelVisible(false);

    const EditorConfiguration *config = project->editorConfiguration();

    for (ICodeStylePreferencesFactory *factory : TextEditorSettings::codeStyleFactories()) {
        Utils::Id languageId = factory->languageId();
        ICodeStylePreferences *codeStylePreferences = config->codeStyle(languageId);

        auto preview = factory->createCodeStyleEditor(codeStylePreferences, project, stackedWidget);
        if (preview && preview->layout())
            preview->layout()->setContentsMargins(QMargins());
        stackedWidget->addWidget(preview);
        languageComboBox->addItem(factory->displayName());
    }

    connect(languageComboBox, &QComboBox::currentIndexChanged,
            stackedWidget, &QStackedWidget::setCurrentIndex);

    using namespace Utils::Layouting;

    Column {
        Row { new QLabel(tr("Language:")), languageComboBox, Stretch() },
        stackedWidget
    }.attachTo(this, false);
}

} // ProjectExplorer::Internal
