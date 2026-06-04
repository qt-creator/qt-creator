// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codestylesettingspropertiespage.h"

#include "editorconfiguration.h"
#include "project.h"
#include "projectexplorertr.h"
#include "projectpanelfactory.h"

#include <cppeditor/cppeditorconstants.h>

#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/codestyleeditor.h>

#include <utils/layoutbuilder.h>

#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QStackedWidget>

using namespace TextEditor;

namespace ProjectExplorer::Internal {

class CodeStyleSettingsWidget final : public QWidget
{
public:
    explicit CodeStyleSettingsWidget(Project *project)
    {
        const EditorConfiguration *config = project->editorConfiguration();

        for (ICodeStylePreferencesFactory *factory : codeStyleFactories()) {
            Utils::Id languageId = factory->languageId();
            ICodeStylePreferences *codeStylePreferences = config->codeStyle(languageId);
            CodeStyleEditor *preview = factory->createProjectEditor(
                project->projectFilePath(), codeStylePreferences);
            if (preview && preview->layout())
                preview->layout()->setContentsMargins(QMargins());
            m_stackedWidget.addWidget(preview);
            m_languageComboBox.addItem(factory->displayName());
        }

        connect(&m_languageComboBox, &QComboBox::currentIndexChanged,
                &m_stackedWidget, &QStackedWidget::setCurrentIndex);

        using namespace Layouting;

        Column {
            createGlobalSettingsLink(CppEditor::Constants::CPP_CODE_STYLE_SETTINGS_ID),
            createHr(),
            Row { new QLabel(Tr::tr("Language:")), &m_languageComboBox, st },
            &m_stackedWidget,
            noMargin
        }.attachTo(this);
    }

private:
    QComboBox m_languageComboBox;
    QStackedWidget m_stackedWidget;
};

class CodeStyleProjectPanelFactory final : public ProjectPanelFactory
{
public:
    CodeStyleProjectPanelFactory()
    {
        setPriority(40);
        setDisplayName(Tr::tr("Code Style"));
        setCreateWidgetFunction([](Project *project) { return new CodeStyleSettingsWidget(project); });
    }
};

void setupCodeStyleProjectPanel()
{
    static CodeStyleProjectPanelFactory theCodeStyleProjectPanelFactory;
}

} // ProjectExplorer::Internal
