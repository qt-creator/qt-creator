/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "sshsettingspage.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <ssh/sshsettings.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>

#include <QCheckBox>
#include <QCoreApplication>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QString>

using namespace QSsh;
using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class SshSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    SshSettingsWidget();
    void saveSettings();

private:
    void setupConnectionSharingCheckBox();
    void setupConnectionSharingSpinBox();
    void setupSshPathChooser();
    void setupSftpPathChooser();
    void setupAskpassPathChooser();
    void setupKeygenPathChooser();
    void setupPathChooser(PathChooser &chooser, const FileName &initialPath, bool &changedFlag);
    void updateCheckboxEnabled();
    void updateSpinboxEnabled();

    QCheckBox m_connectionSharingCheckBox;
    QSpinBox m_connectionSharingSpinBox;
    PathChooser m_sshChooser;
    PathChooser m_sftpChooser;
    PathChooser m_askpassChooser;
    PathChooser m_keygenChooser;
    bool m_sshPathChanged = false;
    bool m_sftpPathChanged = false;
    bool m_askpassPathChanged = false;
    bool m_keygenPathChanged = false;
};

SshSettingsPage::SshSettingsPage(QObject *parent) : Core::IOptionsPage(parent)
{
    setId(Constants::SSH_SETTINGS_PAGE_ID);
    setDisplayName(tr("SSH"));
    setCategory(Constants::DEVICE_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer", "SSH"));
    setCategoryIcon(Utils::Icon({{":/projectexplorer/images/settingscategory_devices.png",
                    Utils::Theme::PanelTextColorDark}}, Utils::Icon::Tint));
}

QWidget *SshSettingsPage::widget()
{
    if (!m_widget)
        m_widget = new SshSettingsWidget;
    return m_widget;
}

void SshSettingsPage::apply()
{
    m_widget->saveSettings();
}

void SshSettingsPage::finish()
{
    delete m_widget;
}


SshSettingsWidget::SshSettingsWidget()
{
    setupConnectionSharingCheckBox();
    setupConnectionSharingSpinBox();
    setupSshPathChooser();
    setupSftpPathChooser();
    setupAskpassPathChooser();
    setupKeygenPathChooser();
    auto * const layout = new QFormLayout(this);
    layout->addRow(tr("Enable connection sharing:"), &m_connectionSharingCheckBox);
    layout->addRow(tr("Connection sharing timeout:"), &m_connectionSharingSpinBox);
    layout->addRow(tr("Path to ssh executable:"), &m_sshChooser);
    layout->addRow(tr("Path to sftp executable:"), &m_sftpChooser);
    layout->addRow(tr("Path to ssh-askpass executable:"), &m_askpassChooser);
    layout->addRow(tr("Path to ssh-keygen executable:"), &m_keygenChooser);
    updateCheckboxEnabled();
    updateSpinboxEnabled();
}

void SshSettingsWidget::saveSettings()
{
    SshSettings::setConnectionSharingEnabled(m_connectionSharingCheckBox.isChecked());
    SshSettings::setConnectionSharingTimeout(m_connectionSharingSpinBox.value());
    if (m_sshPathChanged)
        SshSettings::setSshFilePath(m_sshChooser.fileName());
    if (m_sftpPathChanged)
        SshSettings::setSftpFilePath(m_sftpChooser.fileName());
    if (m_askpassPathChanged)
        SshSettings::setAskpassFilePath(m_askpassChooser.fileName());
    if (m_keygenPathChanged)
        SshSettings::setKeygenFilePath(m_keygenChooser.fileName());
    SshSettings::storeSettings(Core::ICore::settings());
}

void SshSettingsWidget::setupConnectionSharingCheckBox()
{
    m_connectionSharingCheckBox.setChecked(SshSettings::connectionSharingEnabled());
    connect(&m_connectionSharingCheckBox, &QCheckBox::toggled,
            this, &SshSettingsWidget::updateSpinboxEnabled);
}

void SshSettingsWidget::setupConnectionSharingSpinBox()
{
    m_connectionSharingSpinBox.setMinimum(1);
    m_connectionSharingSpinBox.setValue(SshSettings::connectionSharingTimeout());
    m_connectionSharingSpinBox.setSuffix(tr(" minutes"));
}

void SshSettingsWidget::setupSshPathChooser()
{
    setupPathChooser(m_sshChooser, SshSettings::sshFilePath(), m_sshPathChanged);
}

void SshSettingsWidget::setupSftpPathChooser()
{
    setupPathChooser(m_sftpChooser, SshSettings::sftpFilePath(), m_sftpPathChanged);
}

void SshSettingsWidget::setupAskpassPathChooser()
{
    setupPathChooser(m_askpassChooser, SshSettings::askpassFilePath(), m_askpassPathChanged);
}

void SshSettingsWidget::setupKeygenPathChooser()
{
    setupPathChooser(m_keygenChooser, SshSettings::keygenFilePath(), m_keygenPathChanged);
}

void SshSettingsWidget::setupPathChooser(PathChooser &chooser, const FileName &initialPath,
                                         bool &changedFlag)
{
    chooser.setExpectedKind(PathChooser::ExistingCommand);
    chooser.setFileName(initialPath);
    connect(&chooser, &PathChooser::pathChanged, [&changedFlag] { changedFlag = true; });
}

void SshSettingsWidget::updateCheckboxEnabled()
{
    if (!Utils::HostOsInfo::isWindowsHost())
        return;
    m_connectionSharingCheckBox.setEnabled(false);
    static_cast<QFormLayout *>(layout())->labelForField(&m_connectionSharingCheckBox)
            ->setEnabled(false);
}

void SshSettingsWidget::updateSpinboxEnabled()
{
    m_connectionSharingSpinBox.setEnabled(m_connectionSharingCheckBox.isChecked());
    static_cast<QFormLayout *>(layout())->labelForField(&m_connectionSharingSpinBox)
            ->setEnabled(m_connectionSharingCheckBox.isChecked());
}

} // namespace Internal
} // namespace ProjectExplorer

#include <sshsettingspage.moc>
