/****************************************************************************
**
** Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
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

#include "cppquickfixprojectsettings.h"
#include "cppeditorconstants.h"
#include <coreplugin/icore.h>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QtDebug>

namespace CppEditor {
namespace Internal {
using namespace Constants;

static const char SETTINGS_FILE_NAME[] = ".cppQuickFix";
static const char USE_GLOBAL_SETTINGS[] = "UseGlobalSettings";

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
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings, [this] {
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
    const QString key = "CppQuickFixProjectsSettings";
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
            msgBox.setText(tr("Quick Fix settings are saved in a file. Existing settings file "
                              "\"%1\" found. Should this file be used or a "
                              "new one be created?")
                               .arg(m_settingsFile.toString()));
            QPushButton *cancel = msgBox.addButton(QMessageBox::Cancel);
            cancel->setToolTip(tr("Switch Back to Global Settings"));
            QPushButton *useExisting = msgBox.addButton(tr("Use Existing"), QMessageBox::AcceptRole);
            useExisting->setToolTip(m_settingsFile.toString());
            QPushButton *createNew = msgBox.addButton(tr("Create New"), QMessageBox::ActionRole);
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

    QSettings settings(m_settingsFile.toString(), QSettings::IniFormat);
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
    QSettings settings(m_settingsFile.toString(), QSettings::IniFormat);
    if (settings.status() == QSettings::NoError) {
        m_ownSettings.loadSettingsFrom(&settings);
        return;
    }
    m_settingsFile.clear();
}

} // namespace Internal
} // namespace CppEditor
