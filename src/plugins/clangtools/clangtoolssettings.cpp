// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolssettings.h"

#include "clangtoolsconstants.h"
#include "clangtoolsutils.h"
#include "executableinfo.h"

#include <coreplugin/icore.h>
#include <cppeditor/clangdiagnosticconfig.h>
#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/cppcodemodelsettings.h>
#include <cppeditor/cpptoolsreuse.h>

#include <utils/algorithm.h>

#include <QThread>

using namespace CppEditor;
using namespace Utils;

namespace ClangTools {
namespace Internal {


const char parallelJobsKey[] = "ParallelJobs";
const char preferConfigFileKey[] = "PreferConfigFile";
const char buildBeforeAnalysisKey[] = "BuildBeforeAnalysis";
const char analyzeOpenFilesKey[] = "AnalyzeOpenFiles";
const char oldDiagnosticConfigIdKey[] = "diagnosticConfigId";

static Id defaultDiagnosticId()
{
    return ClangTools::Constants::DIAG_CONFIG_TIDY_AND_CLAZY;
}

RunSettings::RunSettings()
    : m_diagnosticConfigId(defaultDiagnosticId())
    , m_parallelJobs(qMax(0, QThread::idealThreadCount() / 2))
{
}

void RunSettings::fromMap(const Store &map, const Key &prefix)
{
    m_diagnosticConfigId = Id::fromSetting(map.value(prefix + diagnosticConfigIdKey));
    m_parallelJobs = map.value(prefix + parallelJobsKey).toInt();
    m_preferConfigFile = map.value(prefix + preferConfigFileKey).toBool();
    m_buildBeforeAnalysis = map.value(prefix + buildBeforeAnalysisKey).toBool();
    m_analyzeOpenFiles = map.value(prefix + analyzeOpenFilesKey).toBool();
}

void RunSettings::toMap(Store &map, const Key &prefix) const
{
    map.insert(prefix + diagnosticConfigIdKey, m_diagnosticConfigId.toSetting());
    map.insert(prefix + parallelJobsKey, m_parallelJobs);
    map.insert(prefix + preferConfigFileKey, m_preferConfigFile);
    map.insert(prefix + buildBeforeAnalysisKey, m_buildBeforeAnalysis);
    map.insert(prefix + analyzeOpenFilesKey, m_analyzeOpenFiles);
}

Id RunSettings::diagnosticConfigId() const
{
    if (!diagnosticConfigsModel().hasConfigWithId(m_diagnosticConfigId))
        return defaultDiagnosticId();
    return m_diagnosticConfigId;
}

bool RunSettings::operator==(const RunSettings &other) const
{
    return m_diagnosticConfigId == other.m_diagnosticConfigId
           && m_parallelJobs == other.m_parallelJobs
           && m_preferConfigFile == other.m_preferConfigFile
           && m_buildBeforeAnalysis == other.m_buildBeforeAnalysis
           && m_analyzeOpenFiles == other.m_analyzeOpenFiles;
}

bool RunSettings::hasConfigFileForSourceFile(const Utils::FilePath &sourceFile) const
{
    if (!preferConfigFile())
        return false;
    for (FilePath parentDir = sourceFile.parentDir(); !parentDir.isEmpty();
         parentDir = parentDir.parentDir()) {
        if (parentDir.resolvePath(QLatin1String(".clang-tidy")).isReadableFile())
            return true;
    }
    return false;
}

ClangToolsSettings *ClangToolsSettings::instance()
{
    static ClangToolsSettings instance;
    return &instance;
}

ClangToolsSettings::ClangToolsSettings()
{
    setSettingsGroup(Constants::SETTINGS_ID);

    clangTidyExecutable.setSettingsKey("ClangTidyExecutable");

    clazyStandaloneExecutable.setSettingsKey("ClazyStandaloneExecutable");

    readSettings();
}

static Store convertToMapFromVersionBefore410(QtcSettings *s)
{
    const char oldParallelJobsKey[] = "simultaneousProcesses";
    const char oldBuildBeforeAnalysisKey[] = "buildBeforeAnalysis";

    Store map;
    map.insert(diagnosticConfigIdKey, s->value(oldDiagnosticConfigIdKey));
    map.insert(parallelJobsKey, s->value(oldParallelJobsKey));
    map.insert(buildBeforeAnalysisKey, s->value(oldBuildBeforeAnalysisKey));

    s->remove(oldDiagnosticConfigIdKey);
    s->remove(oldParallelJobsKey);
    s->remove(oldBuildBeforeAnalysisKey);

    return map;
}

ClangDiagnosticConfigs importDiagnosticConfigsFromCodeModel()
{
    const ClangDiagnosticConfigs configs = ClangdSettings::instance().customDiagnosticConfigs();

    ClangDiagnosticConfigs tidyClazyConfigs;
    ClangDiagnosticConfigs clangOnlyConfigs;
    std::tie(tidyClazyConfigs, clangOnlyConfigs)
        = Utils::partition(configs, [](const ClangDiagnosticConfig &config) {
              return !config.checks(ClangToolType::Clazy).isEmpty()
                     || (!config.checks(ClangToolType::Tidy).isEmpty()
                         && config.checks(ClangToolType::Tidy) != "-*");
          });
    return tidyClazyConfigs;
}

void ClangToolsSettings::readSettings()
{
    // Transfer tidy/clazy configs from code model
    bool write = false;
    ClangDiagnosticConfigs importedConfigs = importDiagnosticConfigsFromCodeModel();
    m_diagnosticConfigs.append(importedConfigs);
    if (!importedConfigs.isEmpty())
        write = true;

    AspectContainer::readSettings();

    QtcSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::SETTINGS_ID);
    m_diagnosticConfigs.append(diagnosticConfigsFromSettings(s));

    Store map;
    if (!s->value(oldDiagnosticConfigIdKey).isNull()) {
        map = convertToMapFromVersionBefore410(s);
        write = true;
    } else {
        Store defaults;
        defaults.insert(diagnosticConfigIdKey, defaultDiagnosticId().toSetting());
        defaults.insert(parallelJobsKey, m_runSettings.parallelJobs());
        defaults.insert(preferConfigFileKey, m_runSettings.preferConfigFile());
        defaults.insert(buildBeforeAnalysisKey, m_runSettings.buildBeforeAnalysis());
        defaults.insert(analyzeOpenFilesKey, m_runSettings.analyzeOpenFiles());
        map = defaults;
        for (Store::ConstIterator it = defaults.constBegin(); it != defaults.constEnd(); ++it)
            map.insert(it.key(), s->value(it.key(), it.value()));
    }

    // Run settings
    m_runSettings.fromMap(map);

    s->endGroup();

    if (write)
        writeSettings();
}

void ClangToolsSettings::writeSettings() const
{
    AspectContainer::writeSettings();

    QtcSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::SETTINGS_ID);

    diagnosticConfigsToSettings(s, m_diagnosticConfigs);

    Store map;
    m_runSettings.toMap(map);
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
        int suffixIndex = versionString.length() - 1;
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
    return ClazyStandaloneInfo::getInfo(Internal::toolExecutable(ClangToolType::Clazy)).version;
}

} // namespace Internal
} // namespace ClangTools
