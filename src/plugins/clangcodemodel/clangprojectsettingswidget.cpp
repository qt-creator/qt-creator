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

#include "clangprojectsettingswidget.h"

#include "clangprojectsettings.h"

#include <coreplugin/icore.h>

#include <cpptools/clangdiagnosticconfig.h>
#include <cpptools/clangdiagnosticconfigswidget.h>
#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cpptoolsreuse.h>

#include <utils/hostosinfo.h>

namespace ClangCodeModel {
namespace Internal {

static Core::Id configIdForProject(ClangProjectSettings &projectSettings)
{
    if (projectSettings.useGlobalConfig())
        return CppTools::codeModelSettings()->clangDiagnosticConfigId();
    Core::Id configId = projectSettings.warningConfigId();
    if (!configId.isValid()) {
        configId = CppTools::codeModelSettings()->clangDiagnosticConfigId();
        projectSettings.setWarningConfigId(configId);
    }
    return configId;
}

ClangProjectSettingsWidget::ClangProjectSettingsWidget(ProjectExplorer::Project *project)
    : m_projectSettings(project)
{
    m_ui.setupUi(this);

    using namespace CppTools;

    m_diagnosticConfigWidget = new ClangDiagnosticConfigsWidget;
    refreshDiagnosticConfigsWidgetFromSettings();

    m_ui.generalConfigurationGroupBox->setVisible(Utils::HostOsInfo::isWindowsHost());

    m_ui.clangSettings->setCurrentIndex(m_projectSettings.useGlobalConfig() ? 0 : 1);
    syncOtherWidgetsToComboBox();

    connectToCppCodeModelSettingsChanged();
    connect(m_diagnosticConfigWidget.data(), &ClangDiagnosticConfigsWidget::currentConfigChanged,
            this, &ClangProjectSettingsWidget::onCurrentWarningConfigChanged);
    connect(m_diagnosticConfigWidget.data(), &ClangDiagnosticConfigsWidget::customConfigsChanged,
            this, &ClangProjectSettingsWidget::onCustomWarningConfigsChanged);
    connect(m_ui.delayedTemplateParse, &QCheckBox::toggled,
            this, &ClangProjectSettingsWidget::onDelayedTemplateParseClicked);
    connect(m_ui.clangSettings,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ClangProjectSettingsWidget::onClangSettingsChanged);

    m_ui.diagnosticConfigurationGroupBox->layout()->addWidget(m_diagnosticConfigWidget);
}

void ClangProjectSettingsWidget::onCurrentWarningConfigChanged(const Core::Id &currentConfigId)
{
    // Don't save it when we reset the global config in code
    if (m_projectSettings.useGlobalConfig())
        return;
    m_projectSettings.setWarningConfigId(currentConfigId);
    m_projectSettings.store();
}

void ClangProjectSettingsWidget::onCustomWarningConfigsChanged(
        const CppTools::ClangDiagnosticConfigs &customConfigs)
{
    disconnectFromCppCodeModelSettingsChanged();

    const QSharedPointer<CppTools::CppCodeModelSettings> codeModelSettings
            = CppTools::codeModelSettings();
    codeModelSettings->setClangCustomDiagnosticConfigs(customConfigs);
    codeModelSettings->toSettings(Core::ICore::settings());

    connectToCppCodeModelSettingsChanged();
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
    m_projectSettings.store();
}

void ClangProjectSettingsWidget::onClangSettingsChanged(int index)
{
    m_projectSettings.setUseGlobalConfig(index == 0 ? true : false);
    m_projectSettings.store();
    syncOtherWidgetsToComboBox();
}

void ClangProjectSettingsWidget::syncOtherWidgetsToComboBox()
{
    const QStringList options = m_projectSettings.commandLineOptions();
    m_ui.delayedTemplateParse->setChecked(
                options.contains(QLatin1String{ClangProjectSettings::DelayedTemplateParsing}));

    const bool isCustom = !m_projectSettings.useGlobalConfig();
    m_ui.generalConfigurationGroupBox->setEnabled(isCustom);
    m_ui.diagnosticConfigurationGroupBox->setEnabled(isCustom);

    refreshDiagnosticConfigsWidgetFromSettings();
}

void ClangProjectSettingsWidget::refreshDiagnosticConfigsWidgetFromSettings()
{
    CppTools::ClangDiagnosticConfigsModel configsModel(
                CppTools::codeModelSettings()->clangCustomDiagnosticConfigs());
    m_diagnosticConfigWidget->refresh(configsModel,
                                      configIdForProject(m_projectSettings));
}

void ClangProjectSettingsWidget::connectToCppCodeModelSettingsChanged()
{
    connect(CppTools::codeModelSettings().data(), &CppTools::CppCodeModelSettings::changed,
            this, &ClangProjectSettingsWidget::refreshDiagnosticConfigsWidgetFromSettings);
}

void ClangProjectSettingsWidget::disconnectFromCppCodeModelSettingsChanged()
{
    disconnect(CppTools::codeModelSettings().data(), &CppTools::CppCodeModelSettings::changed,
               this, &ClangProjectSettingsWidget::refreshDiagnosticConfigsWidgetFromSettings);
}

} // namespace Internal
} // namespace ClangCodeModel
