// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbssettings.h"

#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagertr.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/qtcsettings.h>

#include <QCheckBox>
#include <QFormLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QPushButton>

using namespace Utils;

namespace QbsProjectManager::Internal {

const char QBS_EXE_KEY[] = "QbsProjectManager/QbsExecutable";
const char QBS_DEFAULT_INSTALL_DIR_KEY[] = "QbsProjectManager/DefaultInstallDir";
const char USE_CREATOR_SETTINGS_KEY[] = "QbsProjectManager/useCreatorDir";

static QString getQbsVersion(const FilePath &qbsExe)
{
    if (qbsExe.isEmpty() || !qbsExe.exists())
        return {};
    Process qbsProc;
    qbsProc.setCommand({qbsExe, {"--version"}});
    qbsProc.start();
    if (!qbsProc.waitForFinished(5000) || qbsProc.exitCode() != 0)
        return {};
    return QString::fromLocal8Bit(qbsProc.readAllRawStandardOutput()).trimmed();
}

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
    FilePath candidate = instance().m_settings.qbsExecutableFilePath;
    if (!candidate.exists())
        candidate = defaultQbsExecutableFilePath();
    return candidate;
}

FilePath QbsSettings::defaultQbsExecutableFilePath()
{
    const QString fileName = HostOsInfo::withExecutableSuffix("qbs");
    FilePath candidate = FilePath::fromString(QCoreApplication::applicationDirPath())
                             .pathAppended(fileName);
    if (!candidate.exists())
        candidate = Environment::systemEnvironment().searchInPath(fileName);
    return candidate;
}

FilePath QbsSettings::qbsConfigFilePath()
{
    const FilePath qbsExe = qbsExecutableFilePath();
    if (!qbsExe.isExecutableFile())
        return {};
    const FilePath qbsConfig = qbsExe.absolutePath().pathAppended("qbs-config")
            .withExecutableSuffix();
    if (!qbsConfig.isExecutableFile())
        return {};
    return qbsConfig;
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
    return useCreatorSettingsDirForQbs() ? Core::ICore::userResourcePath().toString() : QString();
}

QVersionNumber QbsSettings::qbsVersion()
{
    if (instance().m_settings.qbsVersion.isNull())
        instance().m_settings.qbsVersion = QVersionNumber::fromString(
                    getQbsVersion(qbsExecutableFilePath()));
    return instance().m_settings.qbsVersion;
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
    QtcSettings * const s = Core::ICore::settings();
    m_settings.qbsExecutableFilePath = FilePath::fromString(s->value(QBS_EXE_KEY).toString());
    m_settings.defaultInstallDirTemplate = s->value(
                QBS_DEFAULT_INSTALL_DIR_KEY,
                "%{CurrentBuild:QbsBuildRoot}/install-root").toString();
    m_settings.useCreatorSettings = s->value(USE_CREATOR_SETTINGS_KEY, true).toBool();
}

void QbsSettings::storeSettings() const
{
    QtcSettings * const s = Core::ICore::settings();
    s->setValueWithDefault(QBS_EXE_KEY, m_settings.qbsExecutableFilePath.toString(),
                           defaultQbsExecutableFilePath().toString());
    s->setValue(QBS_DEFAULT_INSTALL_DIR_KEY, m_settings.defaultInstallDirTemplate);
    s->setValue(USE_CREATOR_SETTINGS_KEY, m_settings.useCreatorSettings);
}

class QbsSettingsPageWidget : public Core::IOptionsPageWidget
{
public:
    QbsSettingsPageWidget()
    {
        m_qbsExePathChooser.setExpectedKind(PathChooser::ExistingCommand);
        m_qbsExePathChooser.setFilePath(QbsSettings::qbsExecutableFilePath());
        m_resetQbsExeButton.setText(Tr::tr("Reset"));
        m_defaultInstallDirLineEdit.setText(QbsSettings::defaultInstallDirTemplate());
        m_versionLabel.setText(getQbsVersionString());
        //: %1 == "Qt Creator" or "Qt Design Studio"
        m_settingsDirCheckBox.setText(Tr::tr("Use %1 settings directory for Qbs")
                                          .arg(QGuiApplication::applicationDisplayName()));
        m_settingsDirCheckBox.setChecked(QbsSettings::useCreatorSettingsDirForQbs());

        const auto layout = new QFormLayout(this);
        layout->addRow(&m_settingsDirCheckBox);
        const auto qbsExeLayout = new QHBoxLayout;
        qbsExeLayout->addWidget(&m_qbsExePathChooser);
        qbsExeLayout->addWidget(&m_resetQbsExeButton);
        layout->addRow(Tr::tr("Path to qbs executable:"), qbsExeLayout);
        layout->addRow(Tr::tr("Default installation directory:"), &m_defaultInstallDirLineEdit);
        layout->addRow(Tr::tr("Qbs version:"), &m_versionLabel);

        connect(&m_qbsExePathChooser, &PathChooser::textChanged, this, [this] {
            m_versionLabel.setText(getQbsVersionString());
        });
        connect(&m_resetQbsExeButton, &QPushButton::clicked, this, [this] {
            m_qbsExePathChooser.setFilePath(QbsSettings::defaultQbsExecutableFilePath());
        });
    }

    void apply() final
    {
        QbsSettingsData settings = QbsSettings::rawSettingsData();
        if (m_qbsExePathChooser.filePath() != QbsSettings::qbsExecutableFilePath())
            settings.qbsExecutableFilePath = m_qbsExePathChooser.filePath();
        settings.defaultInstallDirTemplate = m_defaultInstallDirLineEdit.text();
        settings.useCreatorSettings = m_settingsDirCheckBox.isChecked();
        settings.qbsVersion = {};
        QbsSettings::setSettingsData(settings);
    }

private:
    QString getQbsVersionString()
    {
        const QString version = getQbsVersion(m_qbsExePathChooser.filePath());
        return version.isEmpty() ? Tr::tr("Failed to retrieve version.") : version;
    }

    PathChooser m_qbsExePathChooser;
    QPushButton m_resetQbsExeButton;
    QLabel m_versionLabel;
    QCheckBox m_settingsDirCheckBox;
    FancyLineEdit m_defaultInstallDirLineEdit;
};

QbsSettingsPage::QbsSettingsPage()
{
    setId("A.QbsProjectManager.QbsSettings");
    setDisplayName(Tr::tr("General"));
    setCategory(Constants::QBS_SETTINGS_CATEGORY);
    setDisplayCategory(Tr::tr(Constants::QBS_SETTINGS_TR_CATEGORY));
    setCategoryIconPath(":/qbsprojectmanager/images/settingscategory_qbsprojectmanager.png");
    setWidgetCreator([] { return new QbsSettingsPageWidget; });
}

} // QbsProjectManager::Internal
