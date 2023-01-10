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

const char clangTidyExecutableKey[] = "ClangTidyExecutable";
const char clazyStandaloneExecutableKey[] = "ClazyStandaloneExecutable";

const char parallelJobsKey[] = "ParallelJobs";
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

void RunSettings::fromMap(const QVariantMap &map, const QString &prefix)
{
    m_diagnosticConfigId = Id::fromSetting(map.value(prefix + diagnosticConfigIdKey));
    m_parallelJobs = map.value(prefix + parallelJobsKey).toInt();
    m_buildBeforeAnalysis = map.value(prefix + buildBeforeAnalysisKey).toBool();
    m_analyzeOpenFiles = map.value(prefix + analyzeOpenFilesKey).toBool();
}

void RunSettings::toMap(QVariantMap &map, const QString &prefix) const
{
    map.insert(prefix + diagnosticConfigIdKey, m_diagnosticConfigId.toSetting());
    map.insert(prefix + parallelJobsKey, m_parallelJobs);
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
           && m_buildBeforeAnalysis == other.m_buildBeforeAnalysis
           && m_analyzeOpenFiles == other.m_analyzeOpenFiles;
}

ClangToolsSettings::ClangToolsSettings()
{
    readSettings();
}

ClangToolsSettings *ClangToolsSettings::instance()
{
    static ClangToolsSettings instance;
    return &instance;
}

static QVariantMap convertToMapFromVersionBefore410(QSettings *s)
{
    const char oldParallelJobsKey[] = "simultaneousProcesses";
    const char oldBuildBeforeAnalysisKey[] = "buildBeforeAnalysis";

    QVariantMap map;
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

    QSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::SETTINGS_ID);
    m_clangTidyExecutable = FilePath::fromSettings(s->value(clangTidyExecutableKey));
    m_clazyStandaloneExecutable = FilePath::fromSettings(s->value(clazyStandaloneExecutableKey));
    m_diagnosticConfigs.append(diagnosticConfigsFromSettings(s));

    QVariantMap map;
    if (!s->value(oldDiagnosticConfigIdKey).isNull()) {
        map = convertToMapFromVersionBefore410(s);
        write = true;
    } else {
        QVariantMap defaults;
        defaults.insert(diagnosticConfigIdKey, defaultDiagnosticId().toSetting());
        defaults.insert(parallelJobsKey, m_runSettings.parallelJobs());
        defaults.insert(buildBeforeAnalysisKey, m_runSettings.buildBeforeAnalysis());
        defaults.insert(analyzeOpenFilesKey, m_runSettings.analyzeOpenFiles());
        map = defaults;
        for (QVariantMap::ConstIterator it = defaults.constBegin(); it != defaults.constEnd(); ++it)
            map.insert(it.key(), s->value(it.key(), it.value()));
    }

    // Run settings
    m_runSettings.fromMap(map);

    s->endGroup();

    if (write)
        writeSettings();
}

void ClangToolsSettings::writeSettings()
{
    QSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::SETTINGS_ID);

    s->setValue(clangTidyExecutableKey, m_clangTidyExecutable.toSettings());
    s->setValue(clazyStandaloneExecutableKey, m_clazyStandaloneExecutable.toSettings());
    diagnosticConfigsToSettings(s, m_diagnosticConfigs);

    QVariantMap map;
    m_runSettings.toMap(map);
    for (QVariantMap::ConstIterator it = map.constBegin(); it != map.constEnd(); ++it)
        s->setValue(it.key(), it.value());

    s->endGroup();

    emit changed();
}

FilePath ClangToolsSettings::executable(ClangToolType tool) const
{
    return tool == ClangToolType::Tidy ? m_clangTidyExecutable : m_clazyStandaloneExecutable;
}

void ClangToolsSettings::setExecutable(ClangToolType tool, const FilePath &path)
{
    if (tool == ClangToolType::Tidy) {
        m_clangTidyExecutable = path;
        m_clangTidyVersion = {};
    } else {
        m_clazyStandaloneExecutable = path;
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
    return getVersionNumber(instance()->m_clangTidyVersion, Internal::clangTidyExecutable());
}

QVersionNumber ClangToolsSettings::clazyVersion()
{
    return ClazyStandaloneInfo::getInfo(Internal::clazyStandaloneExecutable()).version;
}

} // namespace Internal
} // namespace ClangTools
