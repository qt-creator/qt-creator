// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codestylesettingspropertiespage.h"

#include "editorconfiguration.h"
#include "project.h"
#include "projectexplorertr.h"

#include <cppeditor/cppeditorconstants.h>

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

    setGlobalSettingsId(CppEditor::Constants::CPP_CODE_STYLE_SETTINGS_ID);
    setUseGlobalSettingsCheckBoxVisible(false);

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

    using namespace Layouting;

    Column {
        Row { new QLabel(Tr::tr("Language:")), languageComboBox, st },
        stackedWidget,
        noMargin
    }.attachTo(this);
}

} // ProjectExplorer::Internal
