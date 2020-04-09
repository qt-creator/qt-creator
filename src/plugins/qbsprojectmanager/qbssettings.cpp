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

#include "qbssettings.h"

#include "qbsprojectmanagerconstants.h"

#include <app/app_version.h>
#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>

#include <QCoreApplication>
#include <QCheckBox>
#include <QFormLayout>
#include <QLabel>
#include <QProcess>

using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

const char QBS_EXE_KEY[] = "QbsProjectManager/QbsExecutable";
const char QBS_DEFAULT_INSTALL_DIR_KEY[] = "QbsProjectManager/DefaultInstallDir";
const char USE_CREATOR_SETTINGS_KEY[] = "QbsProjectManager/useCreatorDir";

static bool operator==(const QbsSettingsData &s1, const QbsSettingsData &s2)
{
    return s1.qbsExecutableFilePath == s2.qbsExecutableFilePath
            && s1.defaultInstallDirTemplate == s2.defaultInstallDirTemplate
            && s1.useCreatorSettings == s2.useCreatorSettings;
}
static bool operator!=(const QbsSettingsData &s1, const QbsSettingsData &s2)
{
    return !(s1 == s2);
}

FilePath QbsSettings::qbsExecutableFilePath()
{
    const QString fileName = HostOsInfo::withExecutableSuffix("qbs");
    FilePath candidate = instance().m_settings.qbsExecutableFilePath;
    if (!candidate.exists()) {
        candidate = FilePath::fromString(QCoreApplication::applicationDirPath())
                .pathAppended(fileName);
    }
    if (!candidate.exists())
        candidate = Environment::systemEnvironment().searchInPath(fileName);
    return candidate;
}

QString QbsSettings::defaultInstallDirTemplate()
{
    return instance().m_settings.defaultInstallDirTemplate;
}

bool QbsSettings::useCreatorSettingsDirForQbs()
{
    return instance().m_settings.useCreatorSettings;
}

QString QbsSettings::qbsSettingsBaseDir()
{
    return useCreatorSettingsDirForQbs() ? Core::ICore::userResourcePath() : QString();
}

QbsSettings &QbsSettings::instance()
{
    static QbsSettings theSettings;
    return theSettings;
}

void QbsSettings::setSettingsData(const QbsSettingsData &settings)
{
    if (instance().m_settings != settings) {
        instance().m_settings = settings;
        instance().storeSettings();
        emit instance().settingsChanged();
    }
}

QbsSettingsData QbsSettings::rawSettingsData()
{
    return instance().m_settings;
}

QbsSettings::QbsSettings()
{
    loadSettings();
}

void QbsSettings::loadSettings()
{
    QSettings * const s = Core::ICore::settings();
    m_settings.qbsExecutableFilePath = FilePath::fromString(s->value(QBS_EXE_KEY).toString());
    m_settings.defaultInstallDirTemplate = s->value(
                QBS_DEFAULT_INSTALL_DIR_KEY,
                "%{CurrentBuild:QbsBuildRoot}/install-root").toString();
    m_settings.useCreatorSettings = s->value(USE_CREATOR_SETTINGS_KEY, true).toBool();
}

void QbsSettings::storeSettings() const
{
    QSettings * const s = Core::ICore::settings();
    s->setValue(QBS_EXE_KEY, m_settings.qbsExecutableFilePath.toString());
    s->setValue(QBS_DEFAULT_INSTALL_DIR_KEY, m_settings.defaultInstallDirTemplate);
    s->setValue(USE_CREATOR_SETTINGS_KEY, m_settings.useCreatorSettings);
}

class QbsSettingsPage::SettingsWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(QbsProjectManager::Internal::QbsSettingsPage)
public:
    SettingsWidget()
    {
        m_qbsExePathChooser.setExpectedKind(PathChooser::ExistingCommand);
        m_qbsExePathChooser.setFilePath(QbsSettings::qbsExecutableFilePath());
        m_defaultInstallDirLineEdit.setText(QbsSettings::defaultInstallDirTemplate());
        m_versionLabel.setText(getQbsVersion());
        m_settingsDirCheckBox.setText(tr("Use %1 settings directory for Qbs")
                                      .arg(Core::Constants::IDE_DISPLAY_NAME));
        m_settingsDirCheckBox.setChecked(QbsSettings::useCreatorSettingsDirForQbs());

        const auto layout = new QFormLayout(this);
        layout->addRow(&m_settingsDirCheckBox);
        layout->addRow(tr("Path to qbs executable:"), &m_qbsExePathChooser);
        layout->addRow(tr("Default installation directory:"), &m_defaultInstallDirLineEdit);
        layout->addRow(tr("Qbs version:"), &m_versionLabel);
    }

    void apply()
    {
        QbsSettingsData settings = QbsSettings::rawSettingsData();
        if (m_qbsExePathChooser.filePath() != QbsSettings::qbsExecutableFilePath())
            settings.qbsExecutableFilePath = m_qbsExePathChooser.filePath();
        settings.defaultInstallDirTemplate = m_defaultInstallDirLineEdit.text();
        settings.useCreatorSettings = m_settingsDirCheckBox.isChecked();
        QbsSettings::setSettingsData(settings);
    }

private:
    static QString getQbsVersion()
    {
        const FilePath qbsExe = QbsSettings::qbsExecutableFilePath();
        if (qbsExe.isEmpty() || !qbsExe.exists())
            return tr("Failed to retrieve version.");
        QProcess qbsProc;
        qbsProc.start(qbsExe.toString(), {"--version"});
        if (!qbsProc.waitForStarted(3000) || !qbsProc.waitForFinished(5000)
                || qbsProc.exitCode() != 0) {
            return tr("Failed to retrieve version.");
        }
        return QString::fromLocal8Bit(qbsProc.readAllStandardOutput()).trimmed();
    }

    PathChooser m_qbsExePathChooser;
    QLabel m_versionLabel;
    QCheckBox m_settingsDirCheckBox;
    FancyLineEdit m_defaultInstallDirLineEdit;
};

QbsSettingsPage::QbsSettingsPage()
{
    setId("A.QbsProjectManager.QbsSettings");
    setDisplayName(tr("General"));
    setCategory(Constants::QBS_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("QbsProjectManager",
                                                   Constants::QBS_SETTINGS_TR_CATEGORY));
    setCategoryIconPath(":/qbsprojectmanager/images/settingscategory_qbsprojectmanager.png");
}

QWidget *QbsSettingsPage::widget()
{
    if (!m_widget)
        m_widget = new SettingsWidget;
    return m_widget;

}

void QbsSettingsPage::apply()
{
    if (m_widget)
        m_widget->apply();
}

void QbsSettingsPage::finish()
{
    delete m_widget;
}

} // namespace Internal
} // namespace QbsProjectManager
