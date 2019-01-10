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

#include "clangdiagnosticconfigswidget.h"
#include "cppmodelmanager.h"
#include "cpptoolsconstants.h"
#include "cpptoolsreuse.h"
#include "ui_cppcodemodelsettingspage.h"

#include <clangtools/clangtoolsconstants.h> // for opening the respective options page
#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/utilsicons.h>

#include <QTextStream>

using namespace CppTools;
using namespace CppTools::Internal;

CppCodeModelSettingsWidget::CppCodeModelSettingsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::CppCodeModelSettingsPage)
{
    m_ui->setupUi(this);
    m_ui->expensiveChecksHintIcon->setPixmap(Utils::Icons::WARNING.pixmap());
    m_ui->expensiveChecksHintIcon->setVisible(false);
    m_ui->expensiveChecksHintText->setVisible(false);
    connect(m_ui->expensiveChecksHintText, &QLabel::linkActivated, [](const QString &){
        Core::ICore::showOptionsDialog(ClangTools::Constants::SETTINGS_PAGE_ID);
    });
}

CppCodeModelSettingsWidget::~CppCodeModelSettingsWidget()
{
    delete m_ui;
}

void CppCodeModelSettingsWidget::setSettings(const QSharedPointer<CppCodeModelSettings> &s)
{
    m_settings = s;

    setupGeneralWidgets();
    setupClangCodeModelWidgets();
}

void CppCodeModelSettingsWidget::applyToSettings() const
{
    bool changed = false;

    changed |= applyGeneralWidgetsToSettings();
    changed |= applyClangCodeModelWidgetsToSettings();

    if (changed)
        m_settings->toSettings(Core::ICore::settings());
}

static bool hasConfigExpensiveChecks(const Core::Id &configId)
{
    if (!configId.isValid())
        return false;

    const ClangDiagnosticConfig config
        = ClangDiagnosticConfigsModel(
              codeModelSettings()->clangCustomDiagnosticConfigs())
              .configWithId(configId);

    return !config.clazyChecks().isEmpty()
        || config.clangTidyMode() != ClangDiagnosticConfig::TidyMode::Disabled;
}

void CppCodeModelSettingsWidget::setupClangCodeModelWidgets()
{
    const bool isClangActive = CppModelManager::instance()->isClangCodeModelActive();

    m_ui->clangCodeModelIsDisabledHint->setVisible(!isClangActive);
    m_ui->clangCodeModelIsEnabledHint->setVisible(isClangActive);
    for (int i = 0; i < m_ui->clangDiagnosticConfigsSelectionWidget->layout()->count(); ++i) {
        QWidget *widget = m_ui->clangDiagnosticConfigsSelectionWidget->layout()->itemAt(i)->widget();
        if (widget)
            widget->setEnabled(isClangActive);
    }

    connect(m_settings.data(), &CppCodeModelSettings::changed,
            this, [this]() {
        m_ui->clangDiagnosticConfigsSelectionWidget->refresh(
                    m_ui->clangDiagnosticConfigsSelectionWidget->currentConfigId());
        if (applyClangCodeModelWidgetsToSettings())
            m_settings->toSettings(Core::ICore::settings());
    });

    const auto checkForExpensiveChecks = [this](const Core::Id &configId) {
        const bool visible = hasConfigExpensiveChecks(configId);
        m_ui->expensiveChecksHintIcon->setVisible(visible);
        m_ui->expensiveChecksHintText->setVisible(visible);
    };

    checkForExpensiveChecks(m_ui->clangDiagnosticConfigsSelectionWidget->currentConfigId());
    connect(m_ui->clangDiagnosticConfigsSelectionWidget,
            &ClangDiagnosticConfigsSelectionWidget::currentConfigChanged,
            checkForExpensiveChecks);
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
    const Core::Id oldConfigId = m_settings->clangDiagnosticConfigId();
    const Core::Id currentConfigId = m_ui->clangDiagnosticConfigsSelectionWidget->currentConfigId();
    if (oldConfigId != currentConfigId) {
        m_settings->setClangDiagnosticConfigId(currentConfigId);
        return true;
    }

    return false;
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

CppCodeModelSettingsPage::CppCodeModelSettingsPage(QSharedPointer<CppCodeModelSettings> &settings,
                                                   QObject *parent)
    : Core::IOptionsPage(parent)
    , m_settings(settings)
{
    setId(Constants::CPP_CODE_MODEL_SETTINGS_ID);
    setDisplayName(QCoreApplication::translate("CppTools",Constants::CPP_CODE_MODEL_SETTINGS_NAME));
    setCategory(Constants::CPP_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("CppTools", "C++"));
    setCategoryIcon(Utils::Icon({{":/cpptools/images/settingscategory_cpp.png",
                    Utils::Theme::PanelTextColorDark}}, Utils::Icon::Tint));
}

QWidget *CppCodeModelSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new CppCodeModelSettingsWidget;
        m_widget->setSettings(m_settings);
    }
    return m_widget;
}

void CppCodeModelSettingsPage::apply()
{
    if (m_widget)
        m_widget->applyToSettings();
}

void CppCodeModelSettingsPage::finish()
{
    delete m_widget;
}
