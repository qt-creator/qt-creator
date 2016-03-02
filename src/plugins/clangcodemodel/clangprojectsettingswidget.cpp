/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clangprojectsettingswidget.h"

#include "clangprojectsettings.h"

#include <coreplugin/icore.h>

#include <cpptools/clangdiagnosticconfig.h>
#include <cpptools/clangdiagnosticconfigswidget.h>
#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cpptoolsreuse.h>

static const char GLOBAL_PROXY_CONFIG_ID[] = "globalProxyConfig";

namespace ClangCodeModel {
namespace Internal {

static CppTools::ClangDiagnosticConfig
createConfigRepresentingGlobalSetting(const CppTools::ClangDiagnosticConfig &baseConfig)
{
    CppTools::ClangDiagnosticConfig config = baseConfig;
    config.setId(GLOBAL_PROXY_CONFIG_ID);

    QString displayName = config.displayName();
    if (config.isReadOnly())
        displayName = CppTools::ClangDiagnosticConfigsModel::displayNameWithBuiltinIndication(config);
    displayName = ClangProjectSettingsWidget::tr("Global setting (%1)").arg(displayName);

    config.setDisplayName(displayName);
    config.setIsReadOnly(true);

    return config;
}

static Core::Id globalConfigId(const CppTools::CppCodeModelSettings &settings,
                               const CppTools::ClangDiagnosticConfigsModel &model)
{
    const Core::Id configId = settings.clangDiagnosticConfigId();

    if (model.hasConfigWithId(configId))
        return configId;

    return model.at(0).id(); // Config saved in the settings was removed, fallback to first.
}

static CppTools::ClangDiagnosticConfigsModel
createConfigsModelWithGlobalProxyConfig(const CppTools::CppCodeModelSettings &settings)
{
    using namespace CppTools;
    ClangDiagnosticConfigsModel configsModel(settings.clangCustomDiagnosticConfigs());

    const Core::Id globalId = globalConfigId(settings, configsModel);
    const ClangDiagnosticConfig globalConfig = configsModel.configWithId(globalId);
    const ClangDiagnosticConfig globalProxy
            = createConfigRepresentingGlobalSetting(globalConfig);

    configsModel.prepend(globalProxy);

    return configsModel;
}

static Core::Id configIdForProject(const ClangProjectSettings &projectSettings)
{
    return projectSettings.useGlobalWarningConfig()
            ? Core::Id(GLOBAL_PROXY_CONFIG_ID)
            : projectSettings.warningConfigId();
}

ClangProjectSettingsWidget::ClangProjectSettingsWidget(ProjectExplorer::Project *project)
    : m_projectSettings(project)
{
    m_ui.setupUi(this);

    using namespace CppTools;

    m_diagnosticConfigWidget = new ClangDiagnosticConfigsWidget;
    m_diagnosticConfigWidget->setConfigWithUndecoratedDisplayName(Core::Id(GLOBAL_PROXY_CONFIG_ID));
    refreshDiagnosticConfigsWidgetFromSettings();
    connectToCppCodeModelSettingsChanged();
    connect(m_diagnosticConfigWidget.data(), &ClangDiagnosticConfigsWidget::currentConfigChanged,
            this, &ClangProjectSettingsWidget::onCurrentWarningConfigChanged);
    connect(m_diagnosticConfigWidget.data(), &ClangDiagnosticConfigsWidget::customConfigsChanged,
            this, &ClangProjectSettingsWidget::onCustomWarningConfigsChanged);

    m_ui.diagnosticConfigurationGroupBox->layout()->addWidget(m_diagnosticConfigWidget);
}

void ClangProjectSettingsWidget::onCurrentWarningConfigChanged(const Core::Id &currentConfigId)
{
    const bool useGlobalConfig = currentConfigId == Core::Id(GLOBAL_PROXY_CONFIG_ID);

    m_projectSettings.setUseGlobalWarningConfig(useGlobalConfig);
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

void ClangProjectSettingsWidget::refreshDiagnosticConfigsWidgetFromSettings()
{
    m_diagnosticConfigWidget->refresh(
                createConfigsModelWithGlobalProxyConfig(*CppTools::codeModelSettings()),
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
