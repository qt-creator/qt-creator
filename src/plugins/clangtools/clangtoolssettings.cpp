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
#include <cpptools/clangdiagnosticconfig.h>
#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cpptoolsreuse.h>

#include <utils/algorithm.h>

#include <QThread>

static const char clangTidyExecutableKey[] = "ClangTidyExecutable";
static const char clazyStandaloneExecutableKey[] = "ClazyStandaloneExecutable";

static const char parallelJobsKey[] = "ParallelJobs";
static const char buildBeforeAnalysisKey[] = "BuildBeforeAnalysis";

static const char oldDiagnosticConfigIdKey[] = "diagnosticConfigId";

using namespace CppTools;

namespace ClangTools {
namespace Internal {

static Utils::Id defaultDiagnosticId()
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
    m_diagnosticConfigId = Utils::Id::fromSetting(map.value(prefix + diagnosticConfigIdKey));
    m_parallelJobs = map.value(prefix + parallelJobsKey).toInt();
    m_buildBeforeAnalysis = map.value(prefix + buildBeforeAnalysisKey).toBool();
}

void RunSettings::toMap(QVariantMap &map, const QString &prefix) const
{
    map.insert(prefix + diagnosticConfigIdKey, m_diagnosticConfigId.toSetting());
    map.insert(prefix + parallelJobsKey, m_parallelJobs);
    map.insert(prefix + buildBeforeAnalysisKey, m_buildBeforeAnalysis);
}

Utils::Id RunSettings::diagnosticConfigId() const
{
    if (!diagnosticConfigsModel().hasConfigWithId(m_diagnosticConfigId))
        return defaultDiagnosticId();
    return m_diagnosticConfigId;
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
    m_clangTidyExecutable = s->value(clangTidyExecutableKey).toString();
    m_clazyStandaloneExecutable = s->value(clazyStandaloneExecutableKey).toString();
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

    s->setValue(clangTidyExecutableKey, m_clangTidyExecutable);
    s->setValue(clazyStandaloneExecutableKey, m_clazyStandaloneExecutable);
    diagnosticConfigsToSettings(s, m_diagnosticConfigs);

    QVariantMap map;
    m_runSettings.toMap(map);
    for (QVariantMap::ConstIterator it = map.constBegin(); it != map.constEnd(); ++it)
        s->setValue(it.key(), it.value());

    s->endGroup();

    emit changed();
}

} // namespace Internal
} // namespace ClangTools
