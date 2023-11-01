// Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppquickfixprojectsettings.h"

#include "cppeditorconstants.h"
#include "cppeditortr.h"

#include <coreplugin/icore.h>

#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QtDebug>

using namespace Utils;

namespace CppEditor {
namespace Internal {

using namespace Constants;

const char SETTINGS_FILE_NAME[] = ".cppQuickFix";
const char USE_GLOBAL_SETTINGS[] = "UseGlobalSettings";

CppQuickFixProjectsSettings::CppQuickFixProjectsSettings(ProjectExplorer::Project *project)
{
    m_project = project;
    const auto settings = m_project->namedSettings(QUICK_FIX_SETTINGS_ID).toMap();
    // if no option is saved try to load settings from a file
    m_useGlobalSettings = settings.value(USE_GLOBAL_SETTINGS, false).toBool();
    if (!m_useGlobalSettings) {
        m_settingsFile = searchForCppQuickFixSettingsFile();
        if (!m_settingsFile.isEmpty()) {
            loadOwnSettingsFromFile();
            m_useGlobalSettings = false;
        } else {
            m_useGlobalSettings = true;
        }
    }
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings, this, [this] {
        auto settings = m_project->namedSettings(QUICK_FIX_SETTINGS_ID).toMap();
        settings.insert(USE_GLOBAL_SETTINGS, m_useGlobalSettings);
        m_project->setNamedSettings(QUICK_FIX_SETTINGS_ID, settings);
    });
}

CppQuickFixSettings *CppQuickFixProjectsSettings::getSettings()
{
    if (m_useGlobalSettings)
        return CppQuickFixSettings::instance();

    return &m_ownSettings;
}

bool CppQuickFixProjectsSettings::isUsingGlobalSettings() const
{
    return m_useGlobalSettings;
}

const Utils::FilePath &CppQuickFixProjectsSettings::filePathOfSettingsFile() const
{
    return m_settingsFile;
}

CppQuickFixProjectsSettings::CppQuickFixProjectsSettingsPtr CppQuickFixProjectsSettings::getSettings(
    ProjectExplorer::Project *project)
{
    const Key key = "CppQuickFixProjectsSettings";
    QVariant v = project->extraData(key);
    if (v.isNull()) {
        v = QVariant::fromValue(
            CppQuickFixProjectsSettingsPtr{new CppQuickFixProjectsSettings(project)});
        project->setExtraData(key, v);
    }
    return v.value<QSharedPointer<CppQuickFixProjectsSettings>>();
}

CppQuickFixSettings *CppQuickFixProjectsSettings::getQuickFixSettings(ProjectExplorer::Project *project)
{
    if (project)
        return getSettings(project)->getSettings();
    return CppQuickFixSettings::instance();
}

Utils::FilePath CppQuickFixProjectsSettings::searchForCppQuickFixSettingsFile()
{
    auto cur = m_project->projectDirectory();
    while (!cur.isEmpty()) {
        const auto path = cur / SETTINGS_FILE_NAME;
        if (path.exists())
            return path;

        cur = cur.parentDir();
    }
    return cur;
}

void CppQuickFixProjectsSettings::useGlobalSettings()
{
    m_useGlobalSettings = true;
}

bool CppQuickFixProjectsSettings::useCustomSettings()
{
    if (m_settingsFile.isEmpty()) {
        m_settingsFile = searchForCppQuickFixSettingsFile();
        const Utils::FilePath defaultLocation = m_project->projectDirectory() / SETTINGS_FILE_NAME;
        if (m_settingsFile.isEmpty()) {
            m_settingsFile = defaultLocation;
        } else if (m_settingsFile != defaultLocation) {
            QMessageBox msgBox(Core::ICore::dialogParent());
            msgBox.setText(Tr::tr("Quick Fix settings are saved in a file. Existing settings file "
                                  "\"%1\" found. Should this file be used or a "
                                  "new one be created?")
                               .arg(m_settingsFile.toString()));
            QPushButton *cancel = msgBox.addButton(QMessageBox::Cancel);
            cancel->setToolTip(Tr::tr("Switch Back to Global Settings"));
            QPushButton *useExisting = msgBox.addButton(Tr::tr("Use Existing"), QMessageBox::AcceptRole);
            useExisting->setToolTip(m_settingsFile.toString());
            QPushButton *createNew = msgBox.addButton(Tr::tr("Create New"), QMessageBox::ActionRole);
            createNew->setToolTip(defaultLocation.toString());
            msgBox.exec();
            if (msgBox.clickedButton() == createNew) {
                m_settingsFile = defaultLocation;
            } else if (msgBox.clickedButton() != useExisting) {
                m_settingsFile.clear();
                return false;
            }
        }

        resetOwnSettingsToGlobal();
    }
    if (m_settingsFile.exists())
        loadOwnSettingsFromFile();

    m_useGlobalSettings = false;
    return true;
}

void CppQuickFixProjectsSettings::resetOwnSettingsToGlobal()
{
    m_ownSettings = *CppQuickFixSettings::instance();
}

bool CppQuickFixProjectsSettings::saveOwnSettings()
{
    if (m_settingsFile.isEmpty())
        return false;

    QtcSettings settings(m_settingsFile.toString(), QSettings::IniFormat);
    if (settings.status() == QSettings::NoError) {
        m_ownSettings.saveSettingsTo(&settings);
        settings.sync();
        return settings.status() == QSettings::NoError;
    }
    m_settingsFile.clear();
    return false;
}

void CppQuickFixProjectsSettings::loadOwnSettingsFromFile()
{
    QtcSettings settings(m_settingsFile.toString(), QSettings::IniFormat);
    if (settings.status() == QSettings::NoError) {
        m_ownSettings.loadSettingsFrom(&settings);
        return;
    }
    m_settingsFile.clear();
}

} // namespace Internal
} // namespace CppEditor
