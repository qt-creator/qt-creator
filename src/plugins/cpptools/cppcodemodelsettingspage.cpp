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

#include "cppcodemodelsettingspage.h"
#include "ui_cppcodemodelsettingspage.h"

#include "clangdiagnosticconfigswidget.h"
#include "cppmodelmanager.h"
#include "cpptoolsconstants.h"
#include "cpptoolsreuse.h"

#include <coreplugin/icore.h>
#include <utils/algorithm.h>

#include <QTextStream>

namespace CppTools {
namespace Internal {

class CppCodeModelSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(CppTools::Internal::CppCodeModelSettingsWidget)

public:
    CppCodeModelSettingsWidget(CppCodeModelSettings *s);
    ~CppCodeModelSettingsWidget() override;

private:
    void apply() final;

    void setupGeneralWidgets();
    void setupClangCodeModelWidgets();

    bool applyGeneralWidgetsToSettings() const;
    bool applyClangCodeModelWidgetsToSettings() const;

    Ui::CppCodeModelSettingsPage *m_ui = nullptr;
    CppCodeModelSettings *m_settings = nullptr;
};

CppCodeModelSettingsWidget::CppCodeModelSettingsWidget(CppCodeModelSettings *s)
    : m_ui(new Ui::CppCodeModelSettingsPage)
{
    m_ui->setupUi(this);

    m_settings = s;

    setupGeneralWidgets();
    setupClangCodeModelWidgets();
}

CppCodeModelSettingsWidget::~CppCodeModelSettingsWidget()
{
    delete m_ui;
}

void CppCodeModelSettingsWidget::apply()
{
    bool changed = false;

    changed |= applyGeneralWidgetsToSettings();
    changed |= applyClangCodeModelWidgetsToSettings();

    if (changed)
        m_settings->toSettings(Core::ICore::settings());
}

void CppCodeModelSettingsWidget::setupClangCodeModelWidgets()
{
    m_ui->clangDiagnosticConfigsSelectionWidget
        ->refresh(diagnosticConfigsModel(),
                  m_settings->clangDiagnosticConfigId(),
                  [](const CppTools::ClangDiagnosticConfigs &configs,
                     const Utils::Id &configToSelect) {
                      return new CppTools::ClangDiagnosticConfigsWidget(configs, configToSelect);
                  });

    const bool isClangActive = CppModelManager::instance()->isClangCodeModelActive();
    m_ui->clangCodeModelIsDisabledHint->setVisible(!isClangActive);
    m_ui->clangCodeModelIsEnabledHint->setVisible(isClangActive);
    for (int i = 0; i < m_ui->clangDiagnosticConfigsSelectionWidget->layout()->count(); ++i) {
        QWidget *widget = m_ui->clangDiagnosticConfigsSelectionWidget->layout()->itemAt(i)->widget();
        if (widget)
            widget->setEnabled(isClangActive);
    }
}

void CppCodeModelSettingsWidget::setupGeneralWidgets()
{
    m_ui->interpretAmbiguousHeadersAsCHeaders->setChecked(
                m_settings->interpretAmbigiousHeadersAsCHeaders());

    m_ui->skipIndexingBigFilesCheckBox->setChecked(m_settings->skipIndexingBigFiles());
    m_ui->bigFilesLimitSpinBox->setValue(m_settings->indexerFileSizeLimitInMb());

    const bool ignorePch = m_settings->pchUsage() == CppCodeModelSettings::PchUse_None;
    m_ui->ignorePCHCheckBox->setChecked(ignorePch);
}

bool CppCodeModelSettingsWidget::applyClangCodeModelWidgetsToSettings() const
{
    bool changed = false;

    const Utils::Id oldConfigId = m_settings->clangDiagnosticConfigId();
    const Utils::Id currentConfigId = m_ui->clangDiagnosticConfigsSelectionWidget->currentConfigId();
    if (oldConfigId != currentConfigId) {
        m_settings->setClangDiagnosticConfigId(currentConfigId);
        changed = true;
    }

    const ClangDiagnosticConfigs oldConfigs = m_settings->clangCustomDiagnosticConfigs();
    const ClangDiagnosticConfigs currentConfigs = m_ui->clangDiagnosticConfigsSelectionWidget
                                                      ->customConfigs();
    if (oldConfigs != currentConfigs) {
        m_settings->setClangCustomDiagnosticConfigs(currentConfigs);
        changed = true;
    }

    return changed;
}

bool CppCodeModelSettingsWidget::applyGeneralWidgetsToSettings() const
{
    bool settingsChanged = false;

    const bool newInterpretAmbiguousHeaderAsCHeaders
            = m_ui->interpretAmbiguousHeadersAsCHeaders->isChecked();
    if (m_settings->interpretAmbigiousHeadersAsCHeaders()
            != newInterpretAmbiguousHeaderAsCHeaders) {
        m_settings->setInterpretAmbigiousHeadersAsCHeaders(newInterpretAmbiguousHeaderAsCHeaders);
        settingsChanged = true;
    }

    const bool newSkipIndexingBigFiles = m_ui->skipIndexingBigFilesCheckBox->isChecked();
    if (m_settings->skipIndexingBigFiles() != newSkipIndexingBigFiles) {
        m_settings->setSkipIndexingBigFiles(newSkipIndexingBigFiles);
        settingsChanged = true;
    }
    const int newFileSizeLimit = m_ui->bigFilesLimitSpinBox->value();
    if (m_settings->indexerFileSizeLimitInMb() != newFileSizeLimit) {
        m_settings->setIndexerFileSizeLimitInMb(newFileSizeLimit);
        settingsChanged = true;
    }

    const bool newIgnorePch = m_ui->ignorePCHCheckBox->isChecked();
    const bool previousIgnorePch = m_settings->pchUsage() == CppCodeModelSettings::PchUse_None;
    if (newIgnorePch != previousIgnorePch) {
        const CppCodeModelSettings::PCHUsage pchUsage = m_ui->ignorePCHCheckBox->isChecked()
                ? CppCodeModelSettings::PchUse_None
                : CppCodeModelSettings::PchUse_BuildSystem;
        m_settings->setPCHUsage(pchUsage);
        settingsChanged = true;
    }

    return settingsChanged;
}

CppCodeModelSettingsPage::CppCodeModelSettingsPage(CppCodeModelSettings *settings)
{
    setId(Constants::CPP_CODE_MODEL_SETTINGS_ID);
    setDisplayName(CppCodeModelSettingsWidget::tr("Code Model"));
    setCategory(Constants::CPP_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("CppTools", "C++"));
    setCategoryIconPath(":/projectexplorer/images/settingscategory_cpp.png");
    setWidgetCreator([settings] { return new CppCodeModelSettingsWidget(settings); });
}

} // Internal
} // CppTools
