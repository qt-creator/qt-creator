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

#include <coreplugin/icore.h>

namespace Autotest {
namespace Internal {

static bool validateFilter(Utils::FancyLineEdit *edit, QString * /*error*/)
{
    return edit && GTestUtils::isValidGTestFilter(edit->text());
}

GTestSettingsWidget::GTestSettingsWidget()
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
}

void GTestSettingsWidget::setSettings(const GTestSettings &settings)
{
    m_ui.runDisabledGTestsCB->setChecked(settings.runDisabled);
    m_ui.repeatGTestsCB->setChecked(settings.repeat);
    m_ui.shuffleGTestsCB->setChecked(settings.shuffle);
    m_ui.repetitionSpin->setValue(settings.iterations);
    m_ui.seedSpin->setValue(settings.seed);
    m_ui.breakOnFailureCB->setChecked(settings.breakOnFailure);
    m_ui.throwOnFailureCB->setChecked(settings.throwOnFailure);
    m_ui.groupModeCombo->setCurrentIndex(settings.groupMode - 1); // there's None for internal use
    m_ui.filterLineEdit->setText(settings.gtestFilter);
    m_currentGTestFilter = settings.gtestFilter; // store it temporarily (if edit is invalid)
}

GTestSettings GTestSettingsWidget::settings() const
{
    GTestSettings result;
    result.runDisabled = m_ui.runDisabledGTestsCB->isChecked();
    result.repeat = m_ui.repeatGTestsCB->isChecked();
    result.shuffle = m_ui.shuffleGTestsCB->isChecked();
    result.iterations = m_ui.repetitionSpin->value();
    result.seed = m_ui.seedSpin->value();
    result.breakOnFailure = m_ui.breakOnFailureCB->isChecked();
    result.throwOnFailure = m_ui.throwOnFailureCB->isChecked();
    result.groupMode = static_cast<GTest::Constants::GroupMode>(
                m_ui.groupModeCombo->currentIndex() + 1);
    if (m_ui.filterLineEdit->isValid())
        result.gtestFilter = m_ui.filterLineEdit->text();
    else
        result.gtestFilter = m_currentGTestFilter;
    return result;
}

GTestSettingsPage::GTestSettingsPage(QSharedPointer<IFrameworkSettings> settings,
                                     const ITestFramework *framework)
    : ITestSettingsPage(framework),
      m_settings(qSharedPointerCast<GTestSettings>(settings))
{
    setDisplayName(QCoreApplication::translate("GTestFramework",
                                               GTest::Constants::FRAMEWORK_SETTINGS_CATEGORY));
}

QWidget *GTestSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new GTestSettingsWidget;
        m_widget->setSettings(*m_settings);
    }
    return m_widget;
}

void GTestSettingsPage::apply()
{
    if (!m_widget) // page was not shown at all
        return;

    GTest::Constants::GroupMode oldGroupMode = m_settings->groupMode;
    const QString oldFilter = m_settings->gtestFilter;
    *m_settings = m_widget->settings();
    m_settings->toSettings(Core::ICore::settings());
    if (m_settings->groupMode == oldGroupMode && oldFilter == m_settings->gtestFilter)
        return;

    auto id = Core::Id(Constants::FRAMEWORK_PREFIX).withSuffix(GTest::Constants::FRAMEWORK_NAME);
    TestTreeModel::instance()->rebuild({id});
}

} // namespace Internal
} // namespace Autotest
