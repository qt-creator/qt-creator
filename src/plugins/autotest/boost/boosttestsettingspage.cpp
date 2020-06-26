/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "boosttestsettingspage.h"
#include "boosttestconstants.h"
#include "boosttestsettings.h"
#include "../testframeworkmanager.h"
#include "ui_boosttestsettingspage.h"

#include <coreplugin/icore.h>

namespace Autotest {
namespace Internal {

class BoostTestSettingsWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Autotest::Internal::BoostTestSettingsWidget)

public:
    explicit BoostTestSettingsWidget(BoostTestSettings *settings);

    void apply() final;

    void setSettings(const BoostTestSettings &settings);
    BoostTestSettings settings() const;

private:
    void fillComboBoxes();
    Ui::BoostSettingsPage m_ui;
    BoostTestSettings *m_settings;
};

BoostTestSettingsWidget::BoostTestSettingsWidget(BoostTestSettings *settings)
    : m_settings(settings)
{
    m_ui.setupUi(this);
    fillComboBoxes();
    connect(m_ui.randomizeCB, &QCheckBox::toggled, m_ui.seedSB, &QSpinBox::setEnabled);

    m_ui.logFormatCB->setCurrentIndex(int(m_settings->logLevel));
    m_ui.reportLevelCB->setCurrentIndex(int(m_settings->reportLevel));
    m_ui.randomizeCB->setChecked(m_settings->randomize);
    m_ui.seedSB->setValue(m_settings->seed);
    m_ui.systemErrorCB->setChecked(m_settings->systemErrors);
    m_ui.fpExceptions->setChecked(m_settings->fpExceptions);
    m_ui.memoryLeakCB->setChecked(m_settings->memLeaks);
}

void BoostTestSettingsWidget::apply()
{
    m_settings->logLevel = LogLevel(m_ui.logFormatCB->currentData().toInt());
    m_settings->reportLevel = ReportLevel(m_ui.reportLevelCB->currentData().toInt());
    m_settings->randomize = m_ui.randomizeCB->isChecked();
    m_settings->seed = m_ui.seedSB->value();
    m_settings->systemErrors = m_ui.systemErrorCB->isChecked();
    m_settings->fpExceptions = m_ui.fpExceptions->isChecked();
    m_settings->memLeaks = m_ui.memoryLeakCB->isChecked();

    m_settings->toSettings(Core::ICore::settings());
}

void BoostTestSettingsWidget::fillComboBoxes()
{
    m_ui.logFormatCB->addItem("All", QVariant::fromValue(LogLevel::All));
    m_ui.logFormatCB->addItem("Success", QVariant::fromValue(LogLevel::Success));
    m_ui.logFormatCB->addItem("Test Suite", QVariant::fromValue(LogLevel::TestSuite));
    m_ui.logFormatCB->addItem("Unit Scope", QVariant::fromValue(LogLevel::UnitScope));
    m_ui.logFormatCB->addItem("Message", QVariant::fromValue(LogLevel::Message));
    m_ui.logFormatCB->addItem("Warning", QVariant::fromValue(LogLevel::Warning));
    m_ui.logFormatCB->addItem("Error", QVariant::fromValue(LogLevel::Error));
    m_ui.logFormatCB->addItem("C++ Exception", QVariant::fromValue(LogLevel::CppException));
    m_ui.logFormatCB->addItem("System Error", QVariant::fromValue(LogLevel::SystemError));
    m_ui.logFormatCB->addItem("Fatal Error", QVariant::fromValue(LogLevel::FatalError));
    m_ui.logFormatCB->addItem("Nothing", QVariant::fromValue(LogLevel::Nothing));

    m_ui.reportLevelCB->addItem("Confirm", QVariant::fromValue(ReportLevel::Confirm));
    m_ui.reportLevelCB->addItem("Short", QVariant::fromValue(ReportLevel::Short));
    m_ui.reportLevelCB->addItem("Detailed", QVariant::fromValue(ReportLevel::Detailed));
    m_ui.reportLevelCB->addItem("No", QVariant::fromValue(ReportLevel::No));
}

BoostTestSettingsPage::BoostTestSettingsPage(BoostTestSettings *settings, Utils::Id settingsId)
{
    setId(settingsId);
    setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
    setDisplayName(QCoreApplication::translate("BoostTestFramework",
                                               BoostTest::Constants::FRAMEWORK_SETTINGS_CATEGORY));
    setWidgetCreator([settings] { return new BoostTestSettingsWidget(settings); });
}

} // Internal
} // Autotest
