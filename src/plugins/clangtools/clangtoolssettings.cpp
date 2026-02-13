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
    return !sourceFile.searchHereAndInParents(".clang-tidy", QDir::Files).isEmpty();
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
    return !sourceFile.searchHereAndInParents(".clang-tidy", QDir::Files).isEmpty();
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

} // namespace Internal
} // namespace ClangTools
