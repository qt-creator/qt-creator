// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolsprojectsettings.h"
#include "clangtoolsdiagnostic.h"

#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/store.h>

using namespace Utils;

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
    connect(this, &ClangToolsProjectSettings::suppressedDiagnosticsChanged,
            this, &ClangToolsProjectSettings::changed);
    connect(project, &ProjectExplorer::Project::settingsLoaded,
            this, &ClangToolsProjectSettings::load);
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings, this,
            &ClangToolsProjectSettings::store);
}

ClangToolsProjectSettings::~ClangToolsProjectSettings()
{
    store();
}

void ClangToolsProjectSettings::setUseGlobalSettings(bool useGlobalSettings)
{
    if (m_useGlobalSettings == useGlobalSettings)
        return;
    m_useGlobalSettings = useGlobalSettings;
    emit changed();
}

void ClangToolsProjectSettings::setRunSettings(const RunSettings &settings)
{
    if (m_runSettings == settings)
        return;
    m_runSettings = settings;
    emit changed();
}

void ClangToolsProjectSettings::setSelectedDirs(const QSet<Utils::FilePath> &value)
{
    if (m_selectedDirs == value)
        return;
    m_selectedDirs = value;
    emit changed();
}

void ClangToolsProjectSettings::setSelectedFiles(const QSet<Utils::FilePath> &value)
{
    if (m_selectedFiles == value)
        return;
    m_selectedFiles = value;
    emit changed();
}

void ClangToolsProjectSettings::addSuppressedDiagnostics(const SuppressedDiagnosticsList &diags)
{
    m_suppressedDiagnostics << diags;
    emit suppressedDiagnosticsChanged();
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

static Store convertToMapFromVersionBefore410(ProjectExplorer::Project *p)
{
    // These keys haven't changed.
    const Key keys[] = {
        SETTINGS_KEY_SELECTED_DIRS,
        SETTINGS_KEY_SELECTED_FILES,
        SETTINGS_KEY_SUPPRESSED_DIAGS,
        SETTINGS_KEY_USE_GLOBAL_SETTINGS,
        "ClangTools.BuildBeforeAnalysis",
    };

    Store map;
    for (const Key &key : keys)
        map.insert(key, p->namedSettings(key));

    map.insert(SETTINGS_PREFIX + Key(diagnosticConfigIdKey),
               p->namedSettings("ClangTools.DiagnosticConfig"));

    return map;
}

void ClangToolsProjectSettings::load()
{
    // Load map
    Store map = storeFromVariant(m_project->namedSettings(SETTINGS_KEY_MAIN));

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
    for (const QVariant &v : list) {
        const Store diag = storeFromVariant(v);
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
    Store map;
    map.insert(SETTINGS_KEY_USE_GLOBAL_SETTINGS, m_useGlobalSettings);

    const QStringList dirs = Utils::transform<QList>(m_selectedDirs, &Utils::FilePath::toString);
    map.insert(SETTINGS_KEY_SELECTED_DIRS, dirs);

    const QStringList files = Utils::transform<QList>(m_selectedFiles, &Utils::FilePath::toString);
    map.insert(SETTINGS_KEY_SELECTED_FILES, files);

    QVariantList list;
    for (const SuppressedDiagnostic &diag : std::as_const(m_suppressedDiagnostics)) {
        Store diagMap;
        diagMap.insert(SETTINGS_KEY_SUPPRESSED_DIAGS_FILEPATH, diag.filePath.toString());
        diagMap.insert(SETTINGS_KEY_SUPPRESSED_DIAGS_MESSAGE, diag.description);
        diagMap.insert(SETTINGS_KEY_SUPPRESSED_DIAGS_UNIQIFIER, diag.uniquifier);
        list << variantFromStore(diagMap);
    }
    map.insert(SETTINGS_KEY_SUPPRESSED_DIAGS, list);

    m_runSettings.toMap(map, SETTINGS_PREFIX);

    m_project->setNamedSettings(SETTINGS_KEY_MAIN, variantFromStore(map));
}

ClangToolsProjectSettings::ClangToolsProjectSettingsPtr
    ClangToolsProjectSettings::getSettings(ProjectExplorer::Project *project)
{
    const Key key = "ClangToolsProjectSettings";
    QVariant v = project->extraData(key);
    if (v.isNull()) {
        v = QVariant::fromValue(
                     ClangToolsProjectSettingsPtr{new ClangToolsProjectSettings(project)});
        project->setExtraData(key, v);
    }
    return v.value<QSharedPointer<ClangToolsProjectSettings>>();
}

SuppressedDiagnostic::SuppressedDiagnostic(const Diagnostic &diag)
    : filePath(diag.location.filePath)
    , description(diag.description)
    , uniquifier(diag.explainingSteps.count())
{
}

} // namespace Internal
} // namespace ClangTools
