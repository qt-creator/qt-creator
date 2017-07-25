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

#include "settingspage.h"

#include <coreplugin/coreconstants.h>
#include <utils/qtcassert.h>
#include <utils/progressindicator.h>

#include <QDate>

using namespace UpdateInfo;
using namespace UpdateInfo::Internal;

namespace {

static const char FILTER_OPTIONS_PAGE_ID[] = "Update";
static const char FILTER_OPTIONS_PAGE[] = QT_TRANSLATE_NOOP("Update", "Update");

}

SettingsPage::SettingsPage(UpdateInfoPlugin *plugin)
    : m_widget(0)
    , m_plugin(plugin)
{
    setId(FILTER_OPTIONS_PAGE_ID);
    setCategory(Core::Constants::SETTINGS_CATEGORY_CORE);
    setCategoryIcon(Utils::Icon(Core::Constants::SETTINGS_CATEGORY_CORE_ICON));
    setDisplayName(QCoreApplication::translate("Update", FILTER_OPTIONS_PAGE));
    setDisplayCategory(QCoreApplication::translate("Core", Core::Constants::SETTINGS_TR_CATEGORY_CORE));
}

QWidget *SettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new QWidget;
        m_ui.setupUi(m_widget);
        m_ui.m_checkIntervalComboBox->addItem(tr("Daily"), UpdateInfoPlugin::DailyCheck);
        m_ui.m_checkIntervalComboBox->addItem(tr("Weekly"), UpdateInfoPlugin::WeeklyCheck);
        m_ui.m_checkIntervalComboBox->addItem(tr("Monthly"), UpdateInfoPlugin::MonthlyCheck);
        UpdateInfoPlugin::CheckUpdateInterval interval = m_plugin->checkUpdateInterval();
        for (int i = 0; i < m_ui.m_checkIntervalComboBox->count(); i++) {
            if (m_ui.m_checkIntervalComboBox->itemData(i).toInt() == interval) {
                m_ui.m_checkIntervalComboBox->setCurrentIndex(i);
                break;
            }
        }

        m_ui.m_updatesGroupBox->setChecked(m_plugin->isAutomaticCheck());

        updateLastCheckDate();
        checkRunningChanged(m_plugin->isCheckForUpdatesRunning());

        connect(m_ui.m_checkNowButton, &QPushButton::clicked,
                m_plugin, &UpdateInfoPlugin::startCheckForUpdates);
        connect(m_ui.m_checkIntervalComboBox,
                static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, &SettingsPage::updateNextCheckDate);
        connect(m_plugin, &UpdateInfoPlugin::lastCheckDateChanged,
                this, &SettingsPage::updateLastCheckDate);
        connect(m_plugin, &UpdateInfoPlugin::checkForUpdatesRunningChanged,
                this, &SettingsPage::checkRunningChanged);
        connect(m_plugin, &UpdateInfoPlugin::newUpdatesAvailable,
                this, &SettingsPage::newUpdatesAvailable);
    }
    return m_widget;
}

UpdateInfoPlugin::CheckUpdateInterval SettingsPage::currentCheckInterval() const
{
    QTC_ASSERT(m_widget, return UpdateInfoPlugin::WeeklyCheck);

    return static_cast<UpdateInfoPlugin::CheckUpdateInterval>
            (m_ui.m_checkIntervalComboBox->itemData(m_ui.m_checkIntervalComboBox->currentIndex()).toInt());
}

void SettingsPage::newUpdatesAvailable(bool available)
{
    if (!m_widget)
        return;

    const QString message = available
            ? tr("New updates are available.")
            : tr("No new updates are available.");
    m_ui.m_messageLabel->setText(message);
}

void SettingsPage::checkRunningChanged(bool running)
{
    if (!m_widget)
        return;

    m_ui.m_checkNowButton->setDisabled(running);

    if (running) {
        if (!m_progressIndicator) {
            m_progressIndicator = new Utils::ProgressIndicator(Utils::ProgressIndicatorSize::Large);
            m_progressIndicator->attachToWidget(m_widget);
        }
        m_progressIndicator->show();
    } else {
        if (m_progressIndicator) {
            delete m_progressIndicator;
        }
    }

    const QString message = running
            ? tr("Checking for updates...") : QString();
    m_ui.m_messageLabel->setText(message);
}

void SettingsPage::updateLastCheckDate()
{
    if (!m_widget)
        return;

    const QDate date = m_plugin->lastCheckDate();
    QString lastCheckDateString;
    if (date.isValid())
        lastCheckDateString = date.toString();
    else
        lastCheckDateString = tr("Not checked yet");

    m_ui.m_lastCheckDateLabel->setText(lastCheckDateString);

    updateNextCheckDate();
}

void SettingsPage::updateNextCheckDate()
{
    if (!m_widget)
        return;

    QDate date = m_plugin->nextCheckDate(currentCheckInterval());
    if (!date.isValid() || date < QDate::currentDate())
        date = QDate::currentDate();

    m_ui.m_nextCheckDateLabel->setText(date.toString());
}

void SettingsPage::apply()
{
    if (!m_widget)
        return;

    m_plugin->setCheckUpdateInterval(currentCheckInterval());
    m_plugin->setAutomaticCheck(m_ui.m_updatesGroupBox->isChecked());
}

void SettingsPage::finish()
{
    delete m_widget;
}
