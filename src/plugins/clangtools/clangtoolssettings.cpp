// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolssettings.h"

#include "clangtoolsconstants.h"
#include "clangtoolsdiagnostic.h"
#include "clangtoolsutils.h"
#include "executableinfo.h"

#include <coreplugin/icore.h>
#include <cppeditor/clangdiagnosticconfig.h>
#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/cppcodemodelsettings.h>
#include <cppeditor/cpptoolsreuse.h>

#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/store.h>

#include <QThread>

using namespace CppEditor;
using namespace Utils;

namespace ClangTools::Internal {

static Id defaultDiagnosticId()
{
    return ClangTools::Constants::DIAG_CONFIG_TIDY_AND_CLAZY;
}

Id RunSettings::safeDiagnosticConfigId() const
{
    if (!diagnosticConfigsModel().hasConfigWithId(diagnosticConfigId()))
        return defaultDiagnosticId();
    return diagnosticConfigId();
}

bool RunSettingsData::hasConfigFileForSourceFile(const FilePath &sourceFile) const
{
    if (!preferConfigFile)
        return false;
    return !sourceFile.searchHereAndInParents(".clang-tidy", DirFilterFlag::Files).isEmpty();
}

ClangToolsSettings *ClangToolsSettings::instance()
{
    static ClangToolsSettings instance;
    return &instance;
}

RunSettings::RunSettings(const Key &prefix)
{
    diagnosticConfigId.setDefaultValue(defaultDiagnosticId());

    parallelJobs.setSettingsKey(prefix + "ParallelJobs");
    // parallelJobs.setDefaultValue(-1);
    parallelJobs.setDefaultValue(qMax(0, QThread::idealThreadCount() / 2));

    preferConfigFile.setSettingsKey(prefix + "PreferConfigFile");
    preferConfigFile.setDefaultValue(true);

    buildBeforeAnalysis.setSettingsKey(prefix + "BuildBeforeAnalysis");
    buildBeforeAnalysis.setDefaultValue(true);

    analyzeOpenFiles.setSettingsKey(prefix + "AnalyzeOpenFiles");
    analyzeOpenFiles.setDefaultValue(true);
}

RunSettingsData RunSettings::data() const
{
    RunSettingsData d;
    d.diagnosticConfigId = diagnosticConfigId();
    d.parallelJobs = parallelJobs();
    d.preferConfigFile = preferConfigFile();
    d.buildBeforeAnalysis = buildBeforeAnalysis();
    d.analyzeOpenFiles = analyzeOpenFiles();
    return d;
}

bool RunSettings::hasConfigFileForSourceFile(const FilePath &sourceFile) const
{
    if (!preferConfigFile())
        return false;
    return !sourceFile.searchHereAndInParents(".clang-tidy", DirFilterFlag::Files).isEmpty();
}

ClangToolsSettings::ClangToolsSettings()
{
    registerAspect(&runSettings);

    setSettingsGroup(Constants::SETTINGS_ID);

    clangTidyExecutable.setSettingsKey("ClangTidyExecutable");
    clazyStandaloneExecutable.setSettingsKey("ClazyStandaloneExecutable");

    enableLowerClazyLevels.setSettingsKey("EnableLowerClazyLevels");
    enableLowerClazyLevels.setDefaultValue(true);

    readSettings();
}

void ClangToolsSettings::readSettings()
{
    AspectContainer::readSettings();

    // TODO: The remaining things should be ready for aspectification now.
    QtcSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::SETTINGS_ID);
    m_diagnosticConfigs.append(diagnosticConfigsFromSettings(s));

    // Run settings
    Store map;
    map.insert(diagnosticConfigIdKey,
               s->value(diagnosticConfigIdKey, defaultDiagnosticId().toSetting()));

    s->endGroup();
}

void ClangToolsSettings::writeSettings() const
{
    AspectContainer::writeSettings();

    QtcSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::SETTINGS_ID);

    diagnosticConfigsToSettings(s, m_diagnosticConfigs);

    Store map;
    for (Store::ConstIterator it = map.constBegin(); it != map.constEnd(); ++it)
        s->setValue(it.key(), it.value());

    s->endGroup();

    emit const_cast<ClangToolsSettings *>(this)->changed(); // FIXME: This is the wrong place
}

FilePath ClangToolsSettings::executable(ClangToolType tool) const
{
    return tool == ClangToolType::Tidy ? clangTidyExecutable() : clazyStandaloneExecutable();
}

void ClangToolsSettings::setExecutable(ClangToolType tool, const FilePath &path)
{
    if (tool == ClangToolType::Tidy) {
        clangTidyExecutable.setValue(path);
        m_clangTidyVersion = {};
    } else {
        clazyStandaloneExecutable.setValue(path);
        m_clazyVersion = {};
    }
}

static VersionAndSuffix getVersionNumber(VersionAndSuffix &version, const FilePath &toolFilePath)
{
    if (version.first.isNull() && !toolFilePath.isEmpty()) {
        const QString versionString = queryVersion(toolFilePath, QueryFailMode::Silent);
        qsizetype suffixIndex = versionString.size() - 1;
        version.first = QVersionNumber::fromString(versionString, &suffixIndex);
        version.second = versionString.mid(suffixIndex);
    }
    return version;
}

VersionAndSuffix ClangToolsSettings::clangTidyVersion()
{
    return getVersionNumber(instance()->m_clangTidyVersion,
                            Internal::toolExecutable(ClangToolType::Tidy));
}

QVersionNumber ClangToolsSettings::clazyVersion()
{
    return ClazyStandaloneInfo(Internal::toolExecutable(ClangToolType::Clazy)).version;
}

// ---- ClangToolsProjectSettings ----

const char SETTINGS_KEY_MAIN[] = "ClangTools";
const char SETTINGS_PREFIX_PROJECT[] = "ClangTools.";
const char SETTINGS_KEY_USE_GLOBAL_SETTINGS[] = "ClangTools.UseGlobalSettings";
const char SETTINGS_KEY_SELECTED_DIRS[] = "ClangTools.SelectedDirs";
const char SETTINGS_KEY_SELECTED_FILES[] = "ClangTools.SelectedFiles";
const char SETTINGS_KEY_SUPPRESSED_DIAGS[] = "ClangTools.SuppressedDiagnostics";
const char SETTINGS_KEY_SUPPRESSED_DIAGS_FILEPATH[] = "ClangTools.SuppressedDiagnosticFilePath";
const char SETTINGS_KEY_SUPPRESSED_DIAGS_MESSAGE[] = "ClangTools.SuppressedDiagnosticMessage";
const char SETTINGS_KEY_SUPPRESSED_DIAGS_UNIQIFIER[] = "ClangTools.SuppressedDiagnosticUniquifier";

ClangToolsProjectSettings::ClangToolsProjectSettings(ProjectExplorer::Project *project)
    : runSettings(SETTINGS_PREFIX_PROJECT), m_project(project)
{
    useGlobalSettings.setDefaultValue(true);

    load();

    connect(&useGlobalSettings, &BoolAspect::changed,
            this, [this] { emit changed(); });
    connect(this, &ClangToolsProjectSettings::suppressedDiagnosticsChanged,
            this, [this] { emit changed(); });
    connect(project, &ProjectExplorer::Project::settingsLoaded,
            this, &ClangToolsProjectSettings::load);
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings,
            this, &ClangToolsProjectSettings::store);
}

ClangToolsProjectSettings::~ClangToolsProjectSettings()
{
    store();
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
    map.insert(SETTINGS_PREFIX_PROJECT + Key(diagnosticConfigIdKey),
               p->namedSettings("ClangTools.DiagnosticConfig"));
    return map;
}

void ClangToolsProjectSettings::load()
{
    Store map = storeFromVariant(m_project->namedSettings(SETTINGS_KEY_MAIN));

    bool write = false;
    if (map.isEmpty()) {
        if (!m_project->namedSettings(SETTINGS_KEY_SELECTED_DIRS).isNull()) {
            map = convertToMapFromVersionBefore410(m_project);
            write = true;
        } else {
            return;
        }
    }

    useGlobalSettings.setValue(map.value(SETTINGS_KEY_USE_GLOBAL_SETTINGS).toBool());

    const QVariantList dirs = map.value(SETTINGS_KEY_SELECTED_DIRS).toList();
    m_selectedDirs = Utils::transform<QSet>(dirs, Utils::FilePath::fromSettings);

    const QVariantList files = map.value(SETTINGS_KEY_SELECTED_FILES).toList();
    m_selectedFiles = Utils::transform<QSet>(files, Utils::FilePath::fromSettings);

    const QVariantList list = map.value(SETTINGS_KEY_SUPPRESSED_DIAGS).toList();
    for (const QVariant &v : list) {
        const Store diag = storeFromVariant(v);
        const auto fp = Utils::FilePath::fromSettings(
            diag.value(SETTINGS_KEY_SUPPRESSED_DIAGS_FILEPATH));
        if (fp.isEmpty())
            continue;
        const QString message = diag.value(SETTINGS_KEY_SUPPRESSED_DIAGS_MESSAGE).toString();
        if (message.isEmpty())
            continue;
        const Utils::FilePath fullPath = m_project->projectDirectory().resolvePath(fp);
        if (!fullPath.exists())
            continue;
        const int uniquifier = diag.value(SETTINGS_KEY_SUPPRESSED_DIAGS_UNIQIFIER).toInt();
        m_suppressedDiagnostics << SuppressedDiagnostic(fp, message, uniquifier);
    }
    emit suppressedDiagnosticsChanged();

    runSettings.fromMap(map);

    if (write)
        store();
}

void ClangToolsProjectSettings::store()
{
    Store map;
    map.insert(SETTINGS_KEY_USE_GLOBAL_SETTINGS, useGlobalSettings());

    const QVariantList dirs = Utils::transform<QList>(m_selectedDirs, &Utils::FilePath::toSettings);
    map.insert(SETTINGS_KEY_SELECTED_DIRS, dirs);

    const QVariantList files
        = Utils::transform<QList>(m_selectedFiles, &Utils::FilePath::toSettings);
    map.insert(SETTINGS_KEY_SELECTED_FILES, files);

    QVariantList list;
    for (const SuppressedDiagnostic &diag : std::as_const(m_suppressedDiagnostics)) {
        Store diagMap;
        diagMap.insert(SETTINGS_KEY_SUPPRESSED_DIAGS_FILEPATH, diag.filePath.toSettings());
        diagMap.insert(SETTINGS_KEY_SUPPRESSED_DIAGS_MESSAGE, diag.description);
        diagMap.insert(SETTINGS_KEY_SUPPRESSED_DIAGS_UNIQIFIER, diag.uniquifier);
        list << variantFromStore(diagMap);
    }
    map.insert(SETTINGS_KEY_SUPPRESSED_DIAGS, list);

    runSettings.toMap(map);

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
    return v.value<std::shared_ptr<ClangToolsProjectSettings>>();
}

SuppressedDiagnostic::SuppressedDiagnostic(const Diagnostic &diag)
    : filePath(diag.location.targetFilePath)
    , description(diag.description)
    , uniquifier(diag.explainingSteps.count())
{
}

} // namespace ClangTools::Internal
