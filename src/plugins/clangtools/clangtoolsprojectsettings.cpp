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

#include "clangtoolsprojectsettings.h"
#include "clangtoolsdiagnostic.h"

#include <projectexplorer/session.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace ClangTools {
namespace Internal {

static const char SETTINGS_KEY_USE_GLOBAL_SETTINGS[] = "ClangTools.UseGlobalSettings";
static const char SETTINGS_KEY_DIAGNOSTIC_CONFIG[] = "ClangTools.DiagnosticConfig";
static const char SETTINGS_KEY_BUILD_BEFORE_ANALYSIS[] = "ClangTools.BuildBeforeAnalysis";
static const char SETTINGS_KEY_SELECTED_DIRS[] = "ClangTools.SelectedDirs";
static const char SETTINGS_KEY_SELECTED_FILES[] = "ClangTools.SelectedFiles";
static const char SETTINGS_KEY_SUPPRESSED_DIAGS[] = "ClangTools.SuppressedDiagnostics";
static const char SETTINGS_KEY_SUPPRESSED_DIAGS_FILEPATH[] = "ClangTools.SuppressedDiagnosticFilePath";
static const char SETTINGS_KEY_SUPPRESSED_DIAGS_MESSAGE[] = "ClangTools.SuppressedDiagnosticMessage";
static const char SETTINGS_KEY_SUPPRESSED_DIAGS_CONTEXTKIND[] = "ClangTools.SuppressedDiagnosticContextKind";
static const char SETTINGS_KEY_SUPPRESSED_DIAGS_CONTEXT[] = "ClangTools.SuppressedDiagnosticContext";
static const char SETTINGS_KEY_SUPPRESSED_DIAGS_UNIQIFIER[] = "ClangTools.SuppressedDiagnosticUniquifier";

ClangToolsProjectSettings::ClangToolsProjectSettings(ProjectExplorer::Project *project)
    : m_project(project)
{
    load();
    connect(project, &ProjectExplorer::Project::settingsLoaded,
            this, &ClangToolsProjectSettings::load);
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings, this,
            &ClangToolsProjectSettings::store);
}

ClangToolsProjectSettings::~ClangToolsProjectSettings()
{
    store();
}

void ClangToolsProjectSettings::addSuppressedDiagnostic(const SuppressedDiagnostic &diag)
{
    QTC_ASSERT(!m_suppressedDiagnostics.contains(diag), return);
    m_suppressedDiagnostics << diag;
    emit suppressedDiagnosticsChanged();
}

void ClangToolsProjectSettings::removeSuppressedDiagnostic(const SuppressedDiagnostic &diag)
{
    const bool wasPresent = m_suppressedDiagnostics.removeOne(diag);
    QTC_ASSERT(wasPresent, return);
    emit suppressedDiagnosticsChanged();
}

void ClangToolsProjectSettings::removeAllSuppressedDiagnostics()
{
    m_suppressedDiagnostics.clear();
    emit suppressedDiagnosticsChanged();
}

void ClangToolsProjectSettings::load()
{
    const QVariant useGlobalVariant =  m_project->namedSettings(SETTINGS_KEY_USE_GLOBAL_SETTINGS);
    m_useGlobalSettings = useGlobalVariant.isValid() ? useGlobalVariant.toBool() : true;
    m_diagnosticConfig = Core::Id::fromSetting(
        m_project->namedSettings(SETTINGS_KEY_DIAGNOSTIC_CONFIG));
    m_buildBeforeAnalysis = m_project->namedSettings(SETTINGS_KEY_BUILD_BEFORE_ANALYSIS).toBool();

    auto toFileName = [](const QString &s) { return Utils::FilePath::fromString(s); };

    const QStringList dirs = m_project->namedSettings(SETTINGS_KEY_SELECTED_DIRS).toStringList();
    m_selectedDirs = Utils::transform<QSet>(dirs, toFileName);

    const QStringList files = m_project->namedSettings(SETTINGS_KEY_SELECTED_FILES).toStringList();
    m_selectedFiles = Utils::transform<QSet>(files, toFileName);

    const QVariantList list = m_project->namedSettings(SETTINGS_KEY_SUPPRESSED_DIAGS).toList();
    foreach (const QVariant &v, list) {
        const QVariantMap diag = v.toMap();
        const QString fp = diag.value(SETTINGS_KEY_SUPPRESSED_DIAGS_FILEPATH).toString();
        if (fp.isEmpty())
            continue;
        const QString message = diag.value(SETTINGS_KEY_SUPPRESSED_DIAGS_MESSAGE).toString();
        if (message.isEmpty())
            continue;
        Utils::FilePath fullPath = Utils::FilePath::fromString(fp);
        if (fullPath.toFileInfo().isRelative())
            fullPath = m_project->projectDirectory().pathAppended(fp);
        if (!fullPath.exists())
            continue;
        const QString contextKind = diag.value(SETTINGS_KEY_SUPPRESSED_DIAGS_CONTEXTKIND).toString();
        const QString context = diag.value(SETTINGS_KEY_SUPPRESSED_DIAGS_CONTEXT).toString();
        const int uniquifier = diag.value(SETTINGS_KEY_SUPPRESSED_DIAGS_UNIQIFIER).toInt();
        m_suppressedDiagnostics << SuppressedDiagnostic(Utils::FilePath::fromString(fp), message,
                                                        contextKind, context, uniquifier);
    }
    emit suppressedDiagnosticsChanged();
}

void ClangToolsProjectSettings::store()
{
    m_project->setNamedSettings(SETTINGS_KEY_USE_GLOBAL_SETTINGS, m_useGlobalSettings);
    m_project->setNamedSettings(SETTINGS_KEY_DIAGNOSTIC_CONFIG, m_diagnosticConfig.toSetting());
    m_project->setNamedSettings(SETTINGS_KEY_BUILD_BEFORE_ANALYSIS, m_buildBeforeAnalysis);

    const QStringList dirs = Utils::transform(m_selectedDirs.toList(), &Utils::FilePath::toString);
    m_project->setNamedSettings(SETTINGS_KEY_SELECTED_DIRS, dirs);

    const QStringList files = Utils::transform(m_selectedFiles.toList(), &Utils::FilePath::toString);
    m_project->setNamedSettings(SETTINGS_KEY_SELECTED_FILES, files);

    QVariantList list;
    foreach (const SuppressedDiagnostic &diag, m_suppressedDiagnostics) {
        QVariantMap diagMap;
        diagMap.insert(SETTINGS_KEY_SUPPRESSED_DIAGS_FILEPATH, diag.filePath.toString());
        diagMap.insert(SETTINGS_KEY_SUPPRESSED_DIAGS_MESSAGE, diag.description);
        diagMap.insert(SETTINGS_KEY_SUPPRESSED_DIAGS_CONTEXTKIND, diag.contextKind);
        diagMap.insert(SETTINGS_KEY_SUPPRESSED_DIAGS_CONTEXT, diag.context);
        diagMap.insert(SETTINGS_KEY_SUPPRESSED_DIAGS_UNIQIFIER, diag.uniquifier);
        list << diagMap;
    }
    m_project->setNamedSettings(SETTINGS_KEY_SUPPRESSED_DIAGS, list);
}

bool ClangToolsProjectSettings::useGlobalSettings() const
{
    return m_useGlobalSettings;
}

void ClangToolsProjectSettings::setUseGlobalSettings(bool useGlobalSettings)
{
    m_useGlobalSettings = useGlobalSettings;
}

Core::Id ClangToolsProjectSettings::diagnosticConfig() const
{
    return m_diagnosticConfig;
}

void ClangToolsProjectSettings::setDiagnosticConfig(const Core::Id &diagnosticConfig)
{
    m_diagnosticConfig = diagnosticConfig;
}

bool ClangToolsProjectSettings::buildBeforeAnalysis() const
{
    return m_buildBeforeAnalysis;
}

void ClangToolsProjectSettings::setBuildBeforeAnalysis(bool build)
{
    m_buildBeforeAnalysis = build;
}

ClangToolsProjectSettingsManager::ClangToolsProjectSettingsManager()
{
    QObject::connect(ProjectExplorer::SessionManager::instance(),
                     &ProjectExplorer::SessionManager::aboutToRemoveProject,
                     &ClangToolsProjectSettingsManager::handleProjectToBeRemoved);
}

ClangToolsProjectSettings *ClangToolsProjectSettingsManager::getSettings(
    ProjectExplorer::Project *project)
{
    auto &settings = m_settings[project];
    if (!settings)
        settings.reset(new ClangToolsProjectSettings(project));
    return settings.data();
}

void ClangToolsProjectSettingsManager::handleProjectToBeRemoved(ProjectExplorer::Project *project)
{
    m_settings.remove(project);
}

ClangToolsProjectSettingsManager::SettingsMap ClangToolsProjectSettingsManager::m_settings;

SuppressedDiagnostic::SuppressedDiagnostic(const Diagnostic &diag)
    : filePath(Utils::FilePath::fromString(diag.location.filePath))
    , description(diag.description)
    , contextKind(diag.issueContextKind)
    , context(diag.issueContext)
    , uniquifier(diag.explainingSteps.count())
{
}

} // namespace Internal
} // namespace ClangTools
