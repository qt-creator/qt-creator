/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "clangtoolssettings.h"

#include "clangtoolsconstants.h"
#include "clangtoolsutils.h"

#include <coreplugin/icore.h>
#include <cppeditor/clangdiagnosticconfig.h>
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
    const ClangDiagnosticConfigs configs = codeModelSettings()->clangCustomDiagnosticConfigs();

    ClangDiagnosticConfigs tidyClazyConfigs;
    ClangDiagnosticConfigs clangOnlyConfigs;
    std::tie(tidyClazyConfigs, clangOnlyConfigs)
        = Utils::partition(configs, [](const ClangDiagnosticConfig &config) {
              return !config.clazyChecks().isEmpty()
                  || (!config.clangTidyChecks().isEmpty() && config.clangTidyChecks() != "-*");
          });

    if (!tidyClazyConfigs.isEmpty()) {
        codeModelSettings()->setClangCustomDiagnosticConfigs(clangOnlyConfigs);
        codeModelSettings()->toSettings(Core::ICore::settings());
    }

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
    m_clangTidyExecutable = FilePath::fromVariant(s->value(clangTidyExecutableKey));
    m_clazyStandaloneExecutable = FilePath::fromVariant(s->value(clazyStandaloneExecutableKey));
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

    s->setValue(clangTidyExecutableKey, m_clangTidyExecutable.toVariant());
    s->setValue(clazyStandaloneExecutableKey, m_clazyStandaloneExecutable.toVariant());
    diagnosticConfigsToSettings(s, m_diagnosticConfigs);

    QVariantMap map;
    m_runSettings.toMap(map);
    for (QVariantMap::ConstIterator it = map.constBegin(); it != map.constEnd(); ++it)
        s->setValue(it.key(), it.value());

    s->endGroup();

    emit changed();
}

void ClangToolsSettings::setClangTidyExecutable(const FilePath &path)
{
    m_clangTidyExecutable = path;
    m_clangTidyVersion = {};
}

void ClangToolsSettings::setClazyStandaloneExecutable(const FilePath &path)
{
    m_clazyStandaloneExecutable = path;
    m_clazyVersion = {};
}

static QVersionNumber getVersionNumber(QVersionNumber &version, const FilePath &toolFilePath)
{
    if (version.isNull() && !toolFilePath.isEmpty())
        version = QVersionNumber::fromString(queryVersion(toolFilePath, QueryFailMode::Silent));
    return version;
}

QVersionNumber ClangToolsSettings::clangTidyVersion()
{
    return getVersionNumber(instance()->m_clangTidyVersion, Internal::clangTidyExecutable());
}

QVersionNumber ClangToolsSettings::clazyVersion()
{
    return ClazyStandaloneInfo::getInfo(Internal::clazyStandaloneExecutable()).version;
}

} // namespace Internal
} // namespace ClangTools
