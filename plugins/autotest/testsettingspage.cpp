/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#include "autotestconstants.h"
#include "testsettingspage.h"
#include "testsettings.h"

#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>

namespace Autotest {
namespace Internal {

TestSettingsWidget::TestSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.callgrindRB->setEnabled(Utils::HostOsInfo::isAnyUnixHost()); // valgrind available on UNIX
    m_ui.perfRB->setEnabled(Utils::HostOsInfo::isLinuxHost()); // according to docs perf Linux only
}

void TestSettingsWidget::setSettings(const TestSettings &settings)
{
    m_ui.timeoutSpin->setValue(settings.timeout / 1000); // we store milliseconds
    m_ui.omitInternalMsgCB->setChecked(settings.omitInternalMssg);

    switch (settings.metrics) {
    case MetricsType::Walltime:
        m_ui.walltimeRB->setChecked(true);
        break;
    case MetricsType::TickCounter:
        m_ui.tickcounterRB->setChecked(true);
        break;
    case MetricsType::EventCounter:
        m_ui.eventCounterRB->setChecked(true);
        break;
    case MetricsType::CallGrind:
        m_ui.callgrindRB->setChecked(true);
        break;
    case MetricsType::Perf:
        m_ui.perfRB->setChecked(true);
        break;
    default:
        m_ui.walltimeRB->setChecked(true);
    }
}

TestSettings TestSettingsWidget::settings() const
{
    TestSettings result;
    result.timeout = m_ui.timeoutSpin->value() * 1000; // we display seconds
    result.omitInternalMssg = m_ui.omitInternalMsgCB->isChecked();

    if (m_ui.walltimeRB->isChecked())
        result.metrics = MetricsType::Walltime;
    else if (m_ui.tickcounterRB->isChecked())
        result.metrics = MetricsType::TickCounter;
    else if (m_ui.eventCounterRB->isChecked())
        result.metrics = MetricsType::EventCounter;
    else if (m_ui.callgrindRB->isChecked())
        result.metrics = MetricsType::CallGrind;
    else if (m_ui.perfRB->isChecked())
        result.metrics = MetricsType::Perf;

    return result;
}

TestSettingsPage::TestSettingsPage(const QSharedPointer<TestSettings> &settings)
    : m_settings(settings), m_widget(0)
{
    setId("A.General");
    setDisplayName(tr("General"));
    setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
    setDisplayCategory(tr("Test Settings"));
    setCategoryIcon(QLatin1String(":/images/autotest.png"));
}

TestSettingsPage::~TestSettingsPage()
{
}

QWidget *TestSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new TestSettingsWidget;
        m_widget->setSettings(*m_settings);
    }
    return m_widget;
}

void TestSettingsPage::apply()
{
    if (!m_widget) // page was not shown at all
        return;
    const TestSettings newSettings = m_widget->settings();
    if (newSettings != *m_settings) {
        *m_settings = newSettings;
        m_settings->toSettings(Core::ICore::settings());
    }
}

} // namespace Internal
} // namespace Autotest
