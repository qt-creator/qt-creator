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

#include "updateinfoplugin.h"
#include "updateinfotr.h"

#include <coreplugin/coreconstants.h>

#include <utils/qtcassert.h>
#include <utils/progressindicator.h>
#include <utils/layoutbuilder.h>

#include <QDate>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

namespace UpdateInfo {
namespace Internal {

class UpdateInfoSettingsPageWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(UpdateInfo::Internal::UpdateInfoSettingsPage)

public:
    UpdateInfoSettingsPageWidget(UpdateInfoPlugin *plugin)
        : m_plugin(plugin)
    {
        setWindowTitle(tr("Configure Filters"));

        m_updatesGroupBox = new QGroupBox(tr("Automatic Check for Updates"));
        m_updatesGroupBox->setCheckable(true);
        m_updatesGroupBox->setChecked(true);

        m_infoLabel = new QLabel(tr("Automatically runs a scheduled check for updates on "
                                    "a time interval basis. The automatic check for updates "
                                    "will be performed at the scheduled date, or the next "
                                    "startup following it."));
        m_infoLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        m_infoLabel->setWordWrap(true);

        m_checkIntervalComboBox = new QComboBox;
        m_nextCheckDateLabel = new QLabel;
        m_checkForNewQtVersions = new QCheckBox(tr("Check for new Qt versions"));

        using namespace Utils::Layouting;

        Column {
            m_infoLabel,
            Row {
                Form {
                    new QLabel(tr("Check interval basis:")), m_checkIntervalComboBox, Break(),
                    new QLabel(tr("Next check date:")), m_nextCheckDateLabel
                },
                Stretch()
            },
            m_checkForNewQtVersions
        }.attachTo(m_updatesGroupBox);

        m_lastCheckDateLabel = new QLabel;

        m_checkNowButton = new QPushButton(tr("Check Now"));

        m_messageLabel = new QLabel;

        Column {
            m_updatesGroupBox,
            Row {
                new QLabel(tr("Last check date:")),
                m_lastCheckDateLabel,
                Stretch(),
                Row {
                    m_messageLabel,
                    Stretch(),
                    m_checkNowButton
                }
            },
            Stretch()
        }.attachTo(this);

        m_checkIntervalComboBox->setCurrentIndex(-1);

        m_lastCheckDateLabel->setText(tr("Not checked yet"));

        m_checkIntervalComboBox->addItem(tr("Daily"), UpdateInfoPlugin::DailyCheck);
        m_checkIntervalComboBox->addItem(tr("Weekly"), UpdateInfoPlugin::WeeklyCheck);
        m_checkIntervalComboBox->addItem(tr("Monthly"), UpdateInfoPlugin::MonthlyCheck);
        UpdateInfoPlugin::CheckUpdateInterval interval = m_plugin->checkUpdateInterval();
        for (int i = 0; i < m_checkIntervalComboBox->count(); i++) {
            if (m_checkIntervalComboBox->itemData(i).toInt() == interval) {
                m_checkIntervalComboBox->setCurrentIndex(i);
                break;
            }
        }

        m_updatesGroupBox->setChecked(m_plugin->isAutomaticCheck());
        m_checkForNewQtVersions->setChecked(m_plugin->isCheckingForQtVersions());

        updateLastCheckDate();
        checkRunningChanged(m_plugin->isCheckForUpdatesRunning());

        connect(m_checkNowButton, &QPushButton::clicked,
                m_plugin, &UpdateInfoPlugin::startCheckForUpdates);
        connect(m_checkIntervalComboBox, &QComboBox::currentIndexChanged,
                this, &UpdateInfoSettingsPageWidget::updateNextCheckDate);
        connect(m_plugin, &UpdateInfoPlugin::lastCheckDateChanged,
                this, &UpdateInfoSettingsPageWidget::updateLastCheckDate);
        connect(m_plugin, &UpdateInfoPlugin::checkForUpdatesRunningChanged,
                this, &UpdateInfoSettingsPageWidget::checkRunningChanged);
        connect(m_plugin, &UpdateInfoPlugin::newUpdatesAvailable,
                this, &UpdateInfoSettingsPageWidget::newUpdatesAvailable);
    }

    void apply() final;

private:
    void newUpdatesAvailable(bool available);
    void checkRunningChanged(bool running);
    void updateLastCheckDate();
    void updateNextCheckDate();
    UpdateInfoPlugin::CheckUpdateInterval currentCheckInterval() const;

    QPointer<Utils::ProgressIndicator> m_progressIndicator;

    UpdateInfoPlugin *m_plugin;

    QGroupBox *m_updatesGroupBox;
    QLabel *m_infoLabel;
    QComboBox *m_checkIntervalComboBox;
    QLabel *m_nextCheckDateLabel;
    QCheckBox *m_checkForNewQtVersions;
    QLabel *m_lastCheckDateLabel;
    QPushButton *m_checkNowButton;
    QLabel *m_messageLabel;
};

UpdateInfoPlugin::CheckUpdateInterval UpdateInfoSettingsPageWidget::currentCheckInterval() const
{
    return static_cast<UpdateInfoPlugin::CheckUpdateInterval>
            (m_checkIntervalComboBox->itemData(m_checkIntervalComboBox->currentIndex()).toInt());
}

void UpdateInfoSettingsPageWidget::newUpdatesAvailable(bool available)
{
    const QString message = available
            ? tr("New updates are available.")
            : tr("No new updates are available.");
    m_messageLabel->setText(message);
}

void UpdateInfoSettingsPageWidget::checkRunningChanged(bool running)
{
    m_checkNowButton->setDisabled(running);

    if (running) {
        if (!m_progressIndicator) {
            m_progressIndicator = new Utils::ProgressIndicator(Utils::ProgressIndicatorSize::Large);
            m_progressIndicator->attachToWidget(this);
        }
        m_progressIndicator->show();
    } else {
        if (m_progressIndicator) {
            delete m_progressIndicator;
        }
    }

    const QString message = running
            ? tr("Checking for updates...") : QString();
    m_messageLabel->setText(message);
}

void UpdateInfoSettingsPageWidget::updateLastCheckDate()
{
    const QDate date = m_plugin->lastCheckDate();
    QString lastCheckDateString;
    if (date.isValid())
        lastCheckDateString = date.toString();
    else
        lastCheckDateString = tr("Not checked yet");

    m_lastCheckDateLabel->setText(lastCheckDateString);

    updateNextCheckDate();
}

void UpdateInfoSettingsPageWidget::updateNextCheckDate()
{
    QDate date = m_plugin->nextCheckDate(currentCheckInterval());
    if (!date.isValid() || date < QDate::currentDate())
        date = QDate::currentDate();

    m_nextCheckDateLabel->setText(date.toString());
}

void UpdateInfoSettingsPageWidget::apply()
{
    m_plugin->setCheckUpdateInterval(currentCheckInterval());
    m_plugin->setAutomaticCheck(m_updatesGroupBox->isChecked());
    m_plugin->setCheckingForQtVersions(m_checkForNewQtVersions->isChecked());
}

// SettingsPage

SettingsPage::SettingsPage(UpdateInfoPlugin *plugin)
{
    setId(FILTER_OPTIONS_PAGE_ID);
    setCategory(Core::Constants::SETTINGS_CATEGORY_CORE);
    setDisplayName(UpdateInfoSettingsPageWidget::tr("Update", "Update"));
    setWidgetCreator([plugin] { return new UpdateInfoSettingsPageWidget(plugin); });
}

} // Internal
} // UpdateInfoPlugin
