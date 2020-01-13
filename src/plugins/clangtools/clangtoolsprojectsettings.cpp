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

static const char SETTINGS_KEY_MAIN[] = "ClangTools";
static const char SETTINGS_PREFIX[] = "ClangTools.";
static const char SETTINGS_KEY_USE_GLOBAL_SETTINGS[] = "ClangTools.UseGlobalSettings";
static const char SETTINGS_KEY_SELECTED_DIRS[] = "ClangTools.SelectedDirs";
static const char SETTINGS_KEY_SELECTED_FILES[] = "ClangTools.SelectedFiles";
static const char SETTINGS_KEY_SUPPRESSED_DIAGS[] = "ClangTools.SuppressedDiagnostics";
static const char SETTINGS_KEY_SUPPRESSED_DIAGS_FILEPATH[] = "ClangTools.SuppressedDiagnosticFilePath";
static const char SETTINGS_KEY_SUPPRESSED_DIAGS_MESSAGE[] = "ClangTools.SuppressedDiagnosticMessage";
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

static QVariantMap convertToMapFromVersionBefore410(ProjectExplorer::Project *p)
{
    // These keys haven't changed.
    const QStringList keys = {
        SETTINGS_KEY_SELECTED_DIRS,
        SETTINGS_KEY_SELECTED_FILES,
        SETTINGS_KEY_SUPPRESSED_DIAGS,
        SETTINGS_KEY_USE_GLOBAL_SETTINGS,
        "ClangTools.BuildBeforeAnalysis",
    };

    QVariantMap map;
    for (const QString &key : keys)
        map.insert(key, p->namedSettings(key));

    map.insert(SETTINGS_PREFIX + QString(diagnosticConfigIdKey),
               p->namedSettings("ClangTools.DiagnosticConfig"));

    return map;
}

void ClangToolsProjectSettings::load()
{
    // Load map
    QVariantMap map = m_project->namedSettings(SETTINGS_KEY_MAIN).toMap();

    bool write = false;
    if (map.isEmpty()) {
        if (!m_project->namedSettings(SETTINGS_KEY_SELECTED_DIRS).isNull()) {
            map = convertToMapFromVersionBefore410(m_project);
            write = true;
        } else {
            return; // Use defaults
        }
    }

    // Read map
    m_useGlobalSettings = map.value(SETTINGS_KEY_USE_GLOBAL_SETTINGS).toBool();

    auto toFileName = [](const QString &s) { return Utils::FilePath::fromString(s); };
    const QStringList dirs = map.value(SETTINGS_KEY_SELECTED_DIRS).toStringList();
    m_selectedDirs = Utils::transform<QSet>(dirs, toFileName);

    const QStringList files = map.value(SETTINGS_KEY_SELECTED_FILES).toStringList();
    m_selectedFiles = Utils::transform<QSet>(files, toFileName);

    const QVariantList list = map.value(SETTINGS_KEY_SUPPRESSED_DIAGS).toList();
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
        const int uniquifier = diag.value(SETTINGS_KEY_SUPPRESSED_DIAGS_UNIQIFIER).toInt();
        m_suppressedDiagnostics << SuppressedDiagnostic(Utils::FilePath::fromString(fp),
                                                        message,
                                                        uniquifier);
    }
    emit suppressedDiagnosticsChanged();

    m_runSettings.fromMap(map, SETTINGS_PREFIX);

    if (write)
        store(); // Store new settings format
}

void ClangToolsProjectSettings::store()
{
    QVariantMap map;
    map.insert(SETTINGS_KEY_USE_GLOBAL_SETTINGS, m_useGlobalSettings);

    const QStringList dirs = Utils::transform<QList>(m_selectedDirs, &Utils::FilePath::toString);
    map.insert(SETTINGS_KEY_SELECTED_DIRS, dirs);

    const QStringList files = Utils::transform<QList>(m_selectedFiles, &Utils::FilePath::toString);
    map.insert(SETTINGS_KEY_SELECTED_FILES, files);

    QVariantList list;
    for (const SuppressedDiagnostic &diag : m_suppressedDiagnostics) {
        QVariantMap diagMap;
        diagMap.insert(SETTINGS_KEY_SUPPRESSED_DIAGS_FILEPATH, diag.filePath.toString());
        diagMap.insert(SETTINGS_KEY_SUPPRESSED_DIAGS_MESSAGE, diag.description);
        diagMap.insert(SETTINGS_KEY_SUPPRESSED_DIAGS_UNIQIFIER, diag.uniquifier);
        list << diagMap;
    }
    map.insert(SETTINGS_KEY_SUPPRESSED_DIAGS, list);

    m_runSettings.toMap(map, SETTINGS_PREFIX);

    m_project->setNamedSettings(SETTINGS_KEY_MAIN, map);
}

ClangToolsProjectSettings::ClangToolsProjectSettingsPtr
    ClangToolsProjectSettings::getSettings(ProjectExplorer::Project *project)
{
    const QString key = "ClangToolsProjectSettings";
    QVariant v = project->extraData(key);
    if (v.isNull()) {
        v = QVariant::fromValue(
                     ClangToolsProjectSettingsPtr{new ClangToolsProjectSettings(project)});
        project->setExtraData(key, v);
    }
    return v.value<QSharedPointer<ClangToolsProjectSettings>>();
}

SuppressedDiagnostic::SuppressedDiagnostic(const Diagnostic &diag)
    : filePath(Utils::FilePath::fromString(diag.location.filePath))
    , description(diag.description)
    , uniquifier(diag.explainingSteps.count())
{
}

} // namespace Internal
} // namespace ClangTools
