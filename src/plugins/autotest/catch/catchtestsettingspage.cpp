/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "catchtestsettingspage.h"
#include "catchtestsettings.h"
#include "ui_catchtestsettingspage.h"
#include "../autotestconstants.h"

#include <coreplugin/icore.h>

namespace Autotest {
namespace Internal {

class CatchTestSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Autotest::Internal::CatchTestSettingsWidget)
public:
    explicit CatchTestSettingsWidget(CatchTestSettings *settings);
    void apply() override;
private:
    Ui::CatchTestSettingsPage m_ui;
    CatchTestSettings *m_settings;
};

CatchTestSettingsWidget::CatchTestSettingsWidget(CatchTestSettings *settings)
    : m_settings(settings)
{
    m_ui.setupUi(this);

    m_ui.abortSB->setEnabled(m_settings->abortAfterChecked);
    m_ui.samplesSB->setEnabled(m_settings->samplesChecked),
    m_ui.resamplesSB->setEnabled(m_settings->resamplesChecked);
    m_ui.confIntSB->setEnabled(m_settings->confidenceIntervalChecked);
    m_ui.warmupSB->setEnabled(m_settings->warmupChecked);

    connect(m_ui.abortCB, &QCheckBox::toggled, m_ui.abortSB, &QSpinBox::setEnabled);
    connect(m_ui.samplesCB, &QCheckBox::toggled, m_ui.samplesSB, &QSpinBox::setEnabled);
    connect(m_ui.resamplesCB, &QCheckBox::toggled, m_ui.resamplesSB, &QSpinBox::setEnabled);
    connect(m_ui.confIntCB, &QCheckBox::toggled, m_ui.confIntSB, &QDoubleSpinBox::setEnabled);
    connect(m_ui.warmupCB, &QCheckBox::toggled, m_ui.warmupSB, &QSpinBox::setEnabled);

    m_ui.showSuccessCB->setChecked(m_settings->showSuccess);
    m_ui.breakOnFailCB->setChecked(m_settings->breakOnFailure);
    m_ui.noThrowCB->setChecked(m_settings->noThrow);
    m_ui.visibleWhiteCB->setChecked(m_settings->visibleWhitespace);
    m_ui.warnOnEmpty->setChecked(m_settings->warnOnEmpty);
    m_ui.noAnalysisCB->setChecked(m_settings->noAnalysis);
    m_ui.abortCB->setChecked(m_settings->abortAfterChecked);
    m_ui.abortSB->setValue(m_settings->abortAfter);
    m_ui.samplesCB->setChecked(m_settings->samplesChecked);
    m_ui.samplesSB->setValue(m_settings->benchmarkSamples);
    m_ui.resamplesCB->setChecked(m_settings->resamplesChecked);
    m_ui.resamplesSB->setValue(m_settings->benchmarkResamples);
    m_ui.confIntCB->setChecked(m_settings->confidenceIntervalChecked);
    m_ui.confIntSB->setValue(m_settings->confidenceInterval);
    m_ui.warmupCB->setChecked(m_settings->warmupChecked);
    m_ui.warmupSB->setValue(m_settings->benchmarkWarmupTime);
}

void CatchTestSettingsWidget::apply()
{
    m_settings->showSuccess = m_ui.showSuccessCB->isChecked();
    m_settings->breakOnFailure = m_ui.breakOnFailCB->isChecked();
    m_settings->noThrow = m_ui.noThrowCB->isChecked();
    m_settings->visibleWhitespace = m_ui.visibleWhiteCB->isChecked();
    m_settings->warnOnEmpty = m_ui.warnOnEmpty->isChecked();
    m_settings->noAnalysis = m_ui.noAnalysisCB->isChecked();
    m_settings->abortAfterChecked = m_ui.abortCB->isChecked();
    m_settings->abortAfter = m_ui.abortSB->value();
    m_settings->samplesChecked = m_ui.samplesCB->isChecked();
    m_settings->benchmarkSamples = m_ui.samplesSB->value();
    m_settings->resamplesChecked = m_ui.resamplesCB->isChecked();
    m_settings->benchmarkResamples = m_ui.resamplesSB->value();
    m_settings->confidenceIntervalChecked = m_ui.confIntCB->isChecked();
    m_settings->confidenceInterval = m_ui.confIntSB->value();
    m_settings->warmupChecked = m_ui.warmupCB->isChecked();
    m_settings->benchmarkWarmupTime = m_ui.warmupSB->value();

    m_settings->toSettings(Core::ICore::settings());
}

CatchTestSettingsPage::CatchTestSettingsPage(CatchTestSettings *settings, Utils::Id settingsId)
{
    setId(settingsId);
    setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
    setDisplayName(QCoreApplication::translate("CatchTestFramework", "Catch Test"));
    setWidgetCreator([settings] { return new CatchTestSettingsWidget(settings); });
}

} // namespace Internal
} // namespace Autotest
