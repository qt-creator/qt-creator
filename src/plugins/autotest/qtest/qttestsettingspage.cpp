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

#include "../autotestconstants.h"
#include "qttestconstants.h"
#include "qttestsettingspage.h"
#include "qttestsettings.h"

#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>

namespace Autotest {
namespace Internal {

QtTestSettingsWidget::QtTestSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.callgrindRB->setEnabled(Utils::HostOsInfo::isAnyUnixHost()); // valgrind available on UNIX
    m_ui.perfRB->setEnabled(Utils::HostOsInfo::isLinuxHost()); // according to docs perf Linux only
}

void QtTestSettingsWidget::setSettings(const QtTestSettings &settings)
{
    m_ui.disableCrashhandlerCB->setChecked(settings.noCrashHandler);
    m_ui.useXMLOutputCB->setChecked(settings.useXMLOutput);
    m_ui.verboseBenchmarksCB->setChecked(settings.verboseBench);
    m_ui.logSignalsAndSlotsCB->setChecked(settings.logSignalsSlots);
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

QtTestSettings QtTestSettingsWidget::settings() const
{
    QtTestSettings result;

    result.noCrashHandler = m_ui.disableCrashhandlerCB->isChecked();
    result.useXMLOutput = m_ui.useXMLOutputCB->isChecked();
    result.verboseBench = m_ui.verboseBenchmarksCB->isChecked();
    result.logSignalsSlots = m_ui.logSignalsAndSlotsCB->isChecked();
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

QtTestSettingsPage::QtTestSettingsPage(QSharedPointer<IFrameworkSettings> settings,
                                       const ITestFramework *framework)
    : ITestSettingsPage(framework),
      m_settings(qSharedPointerCast<QtTestSettings>(settings)),
      m_widget(0)
{
    setDisplayName(QCoreApplication::translate("QtTestFramework",
                                               QtTest::Constants::FRAMEWORK_SETTINGS_CATEGORY));
}

QWidget *QtTestSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new QtTestSettingsWidget;
        m_widget->setSettings(*m_settings);
    }
    return m_widget;
}

void QtTestSettingsPage::apply()
{
    if (!m_widget) // page was not shown at all
        return;
    *m_settings = m_widget->settings();
    m_settings->toSettings(Core::ICore::settings());
}

} // namespace Internal
} // namespace Autotest
