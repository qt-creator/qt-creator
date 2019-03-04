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

#include "qmakesettings.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/hostosinfo.h>

#include <QCheckBox>
#include <QCoreApplication>
#include <QVBoxLayout>

namespace QmakeProjectManager {
namespace Internal {

const char BUILD_DIR_WARNING_KEY[] = "QmakeProjectManager/WarnAgainstUnalignedBuildDir";

static bool operator==(const QmakeSettingsData &s1, const QmakeSettingsData &s2)
{
    return s1.warnAgainstUnalignedBuildDir == s2.warnAgainstUnalignedBuildDir;
}
static bool operator!=(const QmakeSettingsData &s1, const QmakeSettingsData &s2)
{
    return !(s1 == s2);
}

bool QmakeSettings::warnAgainstUnalignedBuildDir()
{
    return instance().m_settings.warnAgainstUnalignedBuildDir;
}

QmakeSettings &QmakeSettings::instance()
{
    static QmakeSettings theSettings;
    return theSettings;
}

void QmakeSettings::setSettingsData(const QmakeSettingsData &settings)
{
    if (instance().m_settings != settings) {
        instance().m_settings = settings;
        instance().storeSettings();
        emit instance().settingsChanged();
    }
}

QmakeSettings::QmakeSettings()
{
    loadSettings();
}

void QmakeSettings::loadSettings()
{
    m_settings.warnAgainstUnalignedBuildDir = Core::ICore::settings()->value(
                BUILD_DIR_WARNING_KEY, Utils::HostOsInfo::isWindowsHost()).toBool();
}

void QmakeSettings::storeSettings() const
{
    Core::ICore::settings()->setValue(BUILD_DIR_WARNING_KEY, warnAgainstUnalignedBuildDir());
}

class QmakeSettingsPage::SettingsWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(QmakeProjectManager::Internal::QmakeSettingsPage)
public:
    SettingsWidget()
    {
        m_warnAgainstUnalignedBuildDirCheckbox.setText(tr("Warn if a project's source and "
            "build directories are not at the same level"));
        m_warnAgainstUnalignedBuildDirCheckbox.setToolTip(tr("Qmake has subtle bugs that "
            "can trigger if source and build directory are not at the same level."));
        m_warnAgainstUnalignedBuildDirCheckbox.setChecked(
                    QmakeSettings::warnAgainstUnalignedBuildDir());
        const auto layout = new QVBoxLayout(this);
        layout->addWidget(&m_warnAgainstUnalignedBuildDirCheckbox);
        layout->addStretch(1);
    }

    void apply()
    {
        QmakeSettingsData settings;
        settings.warnAgainstUnalignedBuildDir = m_warnAgainstUnalignedBuildDirCheckbox.isChecked();
        QmakeSettings::setSettingsData(settings);
    }

private:
    QCheckBox m_warnAgainstUnalignedBuildDirCheckbox;
};

QmakeSettingsPage::QmakeSettingsPage()
{
    setId("K.QmakeProjectManager.QmakeSettings");
    setDisplayName(tr("Qmake"));
    setCategory(ProjectExplorer::Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
}

QWidget *QmakeSettingsPage::widget()
{
    if (!m_widget)
        m_widget = new SettingsWidget;
    return m_widget;

}

void QmakeSettingsPage::apply()
{
    if (m_widget)
        m_widget->apply();
}

void QmakeSettingsPage::finish()
{
    delete m_widget;
}

} // namespace Internal
} // namespace QmakeProjectManager
