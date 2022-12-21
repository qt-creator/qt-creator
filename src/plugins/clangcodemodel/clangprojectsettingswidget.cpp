// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangprojectsettingswidget.h"

#include "clangmodelmanagersupport.h"
#include "clangprojectsettings.h"

#include <coreplugin/icore.h>

#include <cppeditor/clangdiagnosticconfig.h>
#include <cppeditor/clangdiagnosticconfigswidget.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppcodemodelsettings.h>
#include <cppeditor/cpptoolsreuse.h>

#include <utils/hostosinfo.h>

namespace ClangCodeModel {
namespace Internal {

static Utils::Id configIdForProject(ClangProjectSettings &projectSettings)
{
    if (projectSettings.useGlobalConfig())
        return CppEditor::codeModelSettings()->clangDiagnosticConfigId();
    return projectSettings.warningConfigId();
}

ClangProjectSettingsWidget::ClangProjectSettingsWidget(ProjectExplorer::Project *project)
    : m_projectSettings(ClangModelManagerSupport::instance()->projectSettings(project))
{
    m_ui.setupUi(this);
    setGlobalSettingsId(CppEditor::Constants::CPP_CODE_MODEL_SETTINGS_ID);

    using namespace CppEditor;

    m_ui.delayedTemplateParseCheckBox->setVisible(Utils::HostOsInfo::isWindowsHost());

    connect(m_ui.clangDiagnosticConfigsSelectionWidget,
            &ClangDiagnosticConfigsSelectionWidget::changed,
            this,
            [this]() {
                // Save project's config id
                const Utils::Id currentConfigId = m_ui.clangDiagnosticConfigsSelectionWidget
                                                     ->currentConfigId();
                m_projectSettings.setWarningConfigId(currentConfigId);

                // Save global custom configs
                const ClangDiagnosticConfigs configs = m_ui.clangDiagnosticConfigsSelectionWidget
                                                           ->customConfigs();
                CppEditor::codeModelSettings()->setClangCustomDiagnosticConfigs(configs);
                CppEditor::codeModelSettings()->toSettings(Core::ICore::settings());
            });

    connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged,
            this, &ClangProjectSettingsWidget::onGlobalCustomChanged);
    connect(m_ui.delayedTemplateParseCheckBox, &QCheckBox::toggled,
            this, &ClangProjectSettingsWidget::onDelayedTemplateParseClicked);
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings,
            this, &ClangProjectSettingsWidget::onAboutToSaveProjectSettings);

    connect(&m_projectSettings, &ClangProjectSettings::changed,
            this, &ClangProjectSettingsWidget::syncWidgets);
    connect(CppEditor::codeModelSettings(), &CppEditor::CppCodeModelSettings::changed,
            this, &ClangProjectSettingsWidget::syncOtherWidgetsToComboBox);

    syncWidgets();
}

void ClangProjectSettingsWidget::onDelayedTemplateParseClicked(bool checked)
{
    // Don't save it when we reset the global config in code
    if (m_projectSettings.useGlobalConfig())
        return;

    const QLatin1String extraFlag{checked ? ClangProjectSettings::DelayedTemplateParsing
                                          : ClangProjectSettings::NoDelayedTemplateParsing};
    QStringList options = m_projectSettings.commandLineOptions();
    options.removeAll(QLatin1String{ClangProjectSettings::DelayedTemplateParsing});
    options.removeAll(QLatin1String{ClangProjectSettings::NoDelayedTemplateParsing});
    options.append(extraFlag);
    m_projectSettings.setCommandLineOptions(options);
}

void ClangProjectSettingsWidget::onGlobalCustomChanged(bool useGlobalSettings)
{
    m_projectSettings.setUseGlobalConfig(useGlobalSettings);
    syncOtherWidgetsToComboBox();
}

void ClangProjectSettingsWidget::onAboutToSaveProjectSettings()
{
    CppEditor::codeModelSettings()->toSettings(Core::ICore::settings());
}

void ClangProjectSettingsWidget::syncWidgets()
{
    setUseGlobalSettings(m_projectSettings.useGlobalConfig());
    syncOtherWidgetsToComboBox();
}

void ClangProjectSettingsWidget::syncOtherWidgetsToComboBox()
{
    const QStringList options = m_projectSettings.commandLineOptions();
    m_ui.delayedTemplateParseCheckBox->setChecked(
        options.contains(QLatin1String{ClangProjectSettings::DelayedTemplateParsing}));

    const bool isCustom = !m_projectSettings.useGlobalConfig();
    m_ui.delayedTemplateParseCheckBox->setEnabled(isCustom);

    for (int i = 0; i < m_ui.clangDiagnosticConfigsSelectionWidget->layout()->count(); ++i) {
        QWidget *widget = m_ui.clangDiagnosticConfigsSelectionWidget->layout()->itemAt(i)->widget();
        if (widget)
            widget->setEnabled(isCustom);
    }

    m_ui.clangDiagnosticConfigsSelectionWidget
        ->refresh(CppEditor::diagnosticConfigsModel(),
                  configIdForProject(m_projectSettings),
                  [](const CppEditor::ClangDiagnosticConfigs &configs,
                     const Utils::Id &configToSelect) {
                      return new CppEditor::ClangDiagnosticConfigsWidget(configs, configToSelect);
                  });
}

} // namespace Internal
} // namespace ClangCodeModel
