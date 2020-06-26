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

#include "gtestconstants.h"
#include "gtestsettingspage.h"
#include "gtestsettings.h"
#include "gtest_utils.h"
#include "../autotestconstants.h"
#include "../testframeworkmanager.h"

#include "ui_gtestsettingspage.h"
#include <coreplugin/icore.h>

namespace Autotest {
namespace Internal {

static bool validateFilter(Utils::FancyLineEdit *edit, QString * /*error*/)
{
    return edit && GTestUtils::isValidGTestFilter(edit->text());
}

class GTestSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Autotest::Internal::GTestSettingsWidget)

public:
    explicit GTestSettingsWidget(GTestSettings *settings);

private:
    void apply() final;

    Ui::GTestSettingsPage m_ui;
    QString m_currentGTestFilter;
    GTestSettings *m_settings;
};

GTestSettingsWidget::GTestSettingsWidget(GTestSettings *settings)
    : m_settings(settings)
{
    m_ui.setupUi(this);
    m_ui.filterLineEdit->setValidationFunction(&validateFilter);
    m_ui.filterLineEdit->setEnabled(m_ui.groupModeCombo->currentIndex() == 1);

    connect(m_ui.groupModeCombo, &QComboBox::currentTextChanged,
            this, [this] () {
        m_ui.filterLineEdit->setEnabled(m_ui.groupModeCombo->currentIndex() == 1);
    });
    connect(m_ui.repeatGTestsCB, &QCheckBox::toggled, m_ui.repetitionSpin, &QSpinBox::setEnabled);
    connect(m_ui.shuffleGTestsCB, &QCheckBox::toggled, m_ui.seedSpin, &QSpinBox::setEnabled);

    m_ui.runDisabledGTestsCB->setChecked(m_settings->runDisabled);
    m_ui.repeatGTestsCB->setChecked(m_settings->repeat);
    m_ui.shuffleGTestsCB->setChecked(m_settings->shuffle);
    m_ui.repetitionSpin->setValue(m_settings->iterations);
    m_ui.seedSpin->setValue(m_settings->seed);
    m_ui.breakOnFailureCB->setChecked(m_settings->breakOnFailure);
    m_ui.throwOnFailureCB->setChecked(m_settings->throwOnFailure);
    m_ui.groupModeCombo->setCurrentIndex(m_settings->groupMode - 1); // there's None for internal use
    m_ui.filterLineEdit->setText(m_settings->gtestFilter);
    m_currentGTestFilter = m_settings->gtestFilter; // store it temporarily (if edit is invalid)
}

void GTestSettingsWidget::apply()
{
    GTest::Constants::GroupMode oldGroupMode = m_settings->groupMode;
    const QString oldFilter = m_settings->gtestFilter;

    m_settings->runDisabled = m_ui.runDisabledGTestsCB->isChecked();
    m_settings->repeat = m_ui.repeatGTestsCB->isChecked();
    m_settings->shuffle = m_ui.shuffleGTestsCB->isChecked();
    m_settings->iterations = m_ui.repetitionSpin->value();
    m_settings->seed = m_ui.seedSpin->value();
    m_settings->breakOnFailure = m_ui.breakOnFailureCB->isChecked();
    m_settings->throwOnFailure = m_ui.throwOnFailureCB->isChecked();
    m_settings->groupMode = static_cast<GTest::Constants::GroupMode>(
                m_ui.groupModeCombo->currentIndex() + 1);
    if (m_ui.filterLineEdit->isValid())
        m_settings->gtestFilter = m_ui.filterLineEdit->text();
    else
        m_settings->gtestFilter = m_currentGTestFilter;

    m_settings->toSettings(Core::ICore::settings());
    if (m_settings->groupMode == oldGroupMode && oldFilter == m_settings->gtestFilter)
        return;

    auto id = Utils::Id(Constants::FRAMEWORK_PREFIX).withSuffix(GTest::Constants::FRAMEWORK_NAME);
    TestTreeModel::instance()->rebuild({id});
}

GTestSettingsPage::GTestSettingsPage(GTestSettings *settings, Utils::Id settingsId)
{
    setId(settingsId);
    setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
    setDisplayName(QCoreApplication::translate("GTestFramework",
                                               GTest::Constants::FRAMEWORK_SETTINGS_CATEGORY));
    setWidgetCreator([settings] { return new GTestSettingsWidget(settings); });
}

} // namespace Internal
} // namespace Autotest
