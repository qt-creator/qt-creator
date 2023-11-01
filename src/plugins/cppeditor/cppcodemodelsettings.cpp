// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcodemodelsettings.h"

#include "clangdiagnosticconfigsmodel.h"
#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cpptoolsreuse.h"

#include <coreplugin/icore.h>
#include <coreplugin/session.h>

#include <projectexplorer/project.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QDateTime>
#include <QHash>
#include <QPair>
#include <QSettings>
#include <QStandardPaths>

using namespace Utils;

namespace CppEditor {

static Id initialClangDiagnosticConfigId() { return Constants::CPP_CLANG_DIAG_CONFIG_BUILDSYSTEM; }
static CppCodeModelSettings::PCHUsage initialPchUsage()
    { return CppCodeModelSettings::PchUse_BuildSystem; }
static Key enableLowerClazyLevelsKey() { return "enableLowerClazyLevels"; }
static Key pchUsageKey() { return Constants::CPPEDITOR_MODEL_MANAGER_PCH_USAGE; }
static Key interpretAmbiguousHeadersAsCHeadersKey()
    { return Constants::CPPEDITOR_INTERPRET_AMBIGIUOUS_HEADERS_AS_C_HEADERS; }
static Key skipIndexingBigFilesKey() { return Constants::CPPEDITOR_SKIP_INDEXING_BIG_FILES; }
static Key ignoreFilesKey() { return Constants::CPPEDITOR_IGNORE_FILES; }
static Key ignorePatternKey() { return Constants::CPPEDITOR_IGNORE_PATTERN; }
static Key useBuiltinPreprocessorKey() { return Constants::CPPEDITOR_USE_BUILTIN_PREPROCESSOR; }
static Key indexerFileSizeLimitKey() { return Constants::CPPEDITOR_INDEXER_FILE_SIZE_LIMIT; }

static Key clangdSettingsKey() { return "ClangdSettings"; }
static Key useClangdKey() { return "UseClangdV7"; }
static Key clangdPathKey() { return "ClangdPath"; }
static Key clangdIndexingKey() { return "ClangdIndexing"; }
static Key clangdIndexingPriorityKey() { return "ClangdIndexingPriority"; }
static Key clangdHeaderSourceSwitchModeKey() { return "ClangdHeaderSourceSwitchMode"; }
static Key clangdCompletionRankingModelKey() { return "ClangdCompletionRankingModel"; }
static Key clangdHeaderInsertionKey() { return "ClangdHeaderInsertion"; }
static Key clangdThreadLimitKey() { return "ClangdThreadLimit"; }
static Key clangdDocumentThresholdKey() { return "ClangdDocumentThreshold"; }
static Key clangdSizeThresholdEnabledKey() { return "ClangdSizeThresholdEnabled"; }
static Key clangdSizeThresholdKey() { return "ClangdSizeThreshold"; }
static Key clangdUseGlobalSettingsKey() { return "useGlobalSettings"; }
static Key clangdblockIndexingSettingsKey() { return "blockIndexing"; }
static Key sessionsWithOneClangdKey() { return "SessionsWithOneClangd"; }
static Key diagnosticConfigIdKey() { return "diagnosticConfigId"; }
static Key checkedHardwareKey() { return "checkedHardware"; }
static Key completionResultsKey() { return "completionResults"; }

static FilePath g_defaultClangdFilePath;
static FilePath fallbackClangdFilePath()
{
    if (g_defaultClangdFilePath.exists())
        return g_defaultClangdFilePath;
    return Environment::systemEnvironment().searchInPath("clangd");
}

void CppCodeModelSettings::fromSettings(QtcSettings *s)
{
    s->beginGroup(Constants::CPPEDITOR_SETTINGSGROUP);

    setEnableLowerClazyLevels(s->value(enableLowerClazyLevelsKey(), true).toBool());

    const QVariant pchUsageVariant = s->value(pchUsageKey(), initialPchUsage());
    setPCHUsage(static_cast<PCHUsage>(pchUsageVariant.toInt()));

    const QVariant interpretAmbiguousHeadersAsCHeaders
            = s->value(interpretAmbiguousHeadersAsCHeadersKey(), false);
    setInterpretAmbigiousHeadersAsCHeaders(interpretAmbiguousHeadersAsCHeaders.toBool());

    const QVariant skipIndexingBigFiles = s->value(skipIndexingBigFilesKey(), true);
    setSkipIndexingBigFiles(skipIndexingBigFiles.toBool());

    const QVariant ignoreFiles = s->value(ignoreFilesKey(), false);
    setIgnoreFiles(ignoreFiles.toBool());

    const QVariant ignorePattern = s->value(ignorePatternKey(), "");
    setIgnorePattern(ignorePattern.toString());

    setUseBuiltinPreprocessor(s->value(useBuiltinPreprocessorKey(), true).toBool());

    const QVariant indexerFileSizeLimit = s->value(indexerFileSizeLimitKey(), 5);
    setIndexerFileSizeLimitInMb(indexerFileSizeLimit.toInt());

    s->endGroup();

    emit changed();
}

void CppCodeModelSettings::toSettings(QtcSettings *s)
{
    s->beginGroup(Constants::CPPEDITOR_SETTINGSGROUP);

    s->setValue(enableLowerClazyLevelsKey(), enableLowerClazyLevels());
    s->setValue(pchUsageKey(), pchUsage());

    s->setValue(interpretAmbiguousHeadersAsCHeadersKey(), interpretAmbigiousHeadersAsCHeaders());
    s->setValue(skipIndexingBigFilesKey(), skipIndexingBigFiles());
    s->setValue(ignoreFilesKey(), ignoreFiles());
    s->setValue(ignorePatternKey(), QVariant(ignorePattern()));
    s->setValue(useBuiltinPreprocessorKey(), useBuiltinPreprocessor());
    s->setValue(indexerFileSizeLimitKey(), indexerFileSizeLimitInMb());

    s->endGroup();

    emit changed();
}

CppCodeModelSettings::PCHUsage CppCodeModelSettings::pchUsage() const
{
    return m_pchUsage;
}

void CppCodeModelSettings::setPCHUsage(CppCodeModelSettings::PCHUsage pchUsage)
{
    m_pchUsage = pchUsage;
}

bool CppCodeModelSettings::interpretAmbigiousHeadersAsCHeaders() const
{
    return m_interpretAmbigiousHeadersAsCHeaders;
}

void CppCodeModelSettings::setInterpretAmbigiousHeadersAsCHeaders(bool yesno)
{
    m_interpretAmbigiousHeadersAsCHeaders = yesno;
}

bool CppCodeModelSettings::skipIndexingBigFiles() const
{
    return m_skipIndexingBigFiles;
}

void CppCodeModelSettings::setSkipIndexingBigFiles(bool yesno)
{
    m_skipIndexingBigFiles = yesno;
}

int CppCodeModelSettings::indexerFileSizeLimitInMb() const
{
    return m_indexerFileSizeLimitInMB;
}

void CppCodeModelSettings::setIndexerFileSizeLimitInMb(int sizeInMB)
{
    m_indexerFileSizeLimitInMB = sizeInMB;
}

bool CppCodeModelSettings::ignoreFiles() const
{
   return m_ignoreFiles;
}

void CppCodeModelSettings::setIgnoreFiles(bool ignoreFiles)
{
    m_ignoreFiles = ignoreFiles;
}

QString CppCodeModelSettings::ignorePattern() const
{
   return m_ignorePattern;
}

void CppCodeModelSettings::setIgnorePattern(const QString& ignorePattern)
{
    m_ignorePattern = ignorePattern;
}


bool CppCodeModelSettings::enableLowerClazyLevels() const
{
    return m_enableLowerClazyLevels;
}

void CppCodeModelSettings::setEnableLowerClazyLevels(bool yesno)
{
    m_enableLowerClazyLevels = yesno;
}


QString ClangdSettings::priorityToString(const IndexingPriority &priority)
{
    switch (priority) {
    case IndexingPriority::Background: return "background";
    case IndexingPriority::Normal: return "normal";
    case IndexingPriority::Low: return "low";
    case IndexingPriority::Off: return {};
    }
    return {};
}

QString ClangdSettings::priorityToDisplayString(const IndexingPriority &priority)
{
    switch (priority) {
    case IndexingPriority::Background: return Tr::tr("Background Priority");
    case IndexingPriority::Normal: return Tr::tr("Normal Priority");
    case IndexingPriority::Low: return Tr::tr("Low Priority");
    case IndexingPriority::Off: return Tr::tr("Off");
    }
    return {};
}

QString ClangdSettings::headerSourceSwitchModeToDisplayString(HeaderSourceSwitchMode mode)
{
    switch (mode) {
    case HeaderSourceSwitchMode::BuiltinOnly: return Tr::tr("Use Built-in Only");
    case HeaderSourceSwitchMode::ClangdOnly: return Tr::tr("Use Clangd Only");
    case HeaderSourceSwitchMode::Both: return Tr::tr("Try Both");
    }
    return {};
}

QString ClangdSettings::rankingModelToCmdLineString(CompletionRankingModel model)
{
    switch (model) {
    case CompletionRankingModel::Default: break;
    case CompletionRankingModel::DecisionForest: return "decision_forest";
    case CompletionRankingModel::Heuristics: return "heuristics";
    }
    QTC_ASSERT(false, return {});
}

QString ClangdSettings::rankingModelToDisplayString(CompletionRankingModel model)
{
    switch (model) {
    case CompletionRankingModel::Default: return Tr::tr("Default");
    case CompletionRankingModel::DecisionForest: return Tr::tr("Decision Forest");
    case CompletionRankingModel::Heuristics: return Tr::tr("Heuristics");
    }
    QTC_ASSERT(false, return {});
}

ClangdSettings &ClangdSettings::instance()
{
    static ClangdSettings settings;
    return settings;
}

ClangdSettings::ClangdSettings()
{
    loadSettings();
    const auto sessionMgr = Core::SessionManager::instance();
    connect(sessionMgr, &Core::SessionManager::sessionRemoved, this, [this](const QString &name) {
        m_data.sessionsWithOneClangd.removeOne(name);
    });
    connect(sessionMgr,
            &Core::SessionManager::sessionRenamed,
            this,
            [this](const QString &oldName, const QString &newName) {
                const auto index = m_data.sessionsWithOneClangd.indexOf(oldName);
                if (index != -1)
                    m_data.sessionsWithOneClangd[index] = newName;
            });
}

bool ClangdSettings::useClangd() const
{
    return m_data.useClangd && clangdVersion() >= QVersionNumber(14);
}

void ClangdSettings::setUseClangd(bool use) { instance().m_data.useClangd = use; }

void ClangdSettings::setUseClangdAndSave(bool use)
{
    setUseClangd(use);
    instance().saveSettings();
}

bool ClangdSettings::hardwareFulfillsRequirements()
{
    instance().m_data.haveCheckedHardwareReqirements = true;
    instance().saveSettings();
    const quint64 minRam = quint64(12) * 1024 * 1024 * 1024;
    const std::optional<quint64> totalRam = Utils::HostOsInfo::totalMemoryInstalledInBytes();
    return !totalRam || *totalRam >= minRam;
}

bool ClangdSettings::haveCheckedHardwareRequirements()
{
    return instance().data().haveCheckedHardwareReqirements;
}

void ClangdSettings::setDefaultClangdPath(const FilePath &filePath)
{
    g_defaultClangdFilePath = filePath;
}

void ClangdSettings::setCustomDiagnosticConfigs(const ClangDiagnosticConfigs &configs)
{
    if (instance().customDiagnosticConfigs() == configs)
        return;
    instance().m_data.customDiagnosticConfigs = configs;
    instance().saveSettings();
}

FilePath ClangdSettings::clangdFilePath() const
{
    if (!m_data.executableFilePath.isEmpty())
        return m_data.executableFilePath;
    return fallbackClangdFilePath();
}

bool ClangdSettings::sizeIsOkay(const Utils::FilePath &fp) const
{
    return !sizeThresholdEnabled() || sizeThresholdInKb() * 1024 >= fp.fileSize();
}

ClangDiagnosticConfigs ClangdSettings::customDiagnosticConfigs() const
{
    return m_data.customDiagnosticConfigs;
}

Id ClangdSettings::diagnosticConfigId() const
{
    if (!diagnosticConfigsModel().hasConfigWithId(m_data.diagnosticConfigId))
        return initialClangDiagnosticConfigId();
    return m_data.diagnosticConfigId;
}

ClangDiagnosticConfig ClangdSettings::diagnosticConfig() const
{
    return diagnosticConfigsModel(customDiagnosticConfigs()).configWithId(diagnosticConfigId());
}

ClangdSettings::Granularity ClangdSettings::granularity() const
{
    if (m_data.sessionsWithOneClangd.contains(Core::SessionManager::activeSession()))
        return Granularity::Session;
    return Granularity::Project;
}

void ClangdSettings::setData(const Data &data)
{
    if (this == &instance() && data != m_data) {
        m_data = data;
        saveSettings();
        emit changed();
    }
}

static FilePath getClangHeadersPathFromClang(const FilePath &clangdFilePath)
{
    const FilePath clangFilePath = clangdFilePath.absolutePath().pathAppended("clang")
            .withExecutableSuffix();
    if (!clangFilePath.exists())
        return {};
    Process clang;
    clang.setCommand({clangFilePath, {"-print-resource-dir"}});
    clang.start();
    if (!clang.waitForFinished())
        return {};
    const FilePath resourceDir = FilePath::fromUserInput(QString::fromLocal8Bit(
            clang.readAllRawStandardOutput().trimmed()));
    if (resourceDir.isEmpty() || !resourceDir.exists())
        return {};
    const FilePath includeDir = resourceDir.pathAppended("include");
    if (!includeDir.exists())
        return {};
    return includeDir;
}

static FilePath getClangHeadersPath(const FilePath &clangdFilePath)
{
    const FilePath headersPath = getClangHeadersPathFromClang(clangdFilePath);
    if (!headersPath.isEmpty())
        return headersPath;

    const QVersionNumber version = Utils::clangdVersion(clangdFilePath);
    QTC_ASSERT(!version.isNull(), return {});
    static const QStringList libDirs{"lib", "lib64"};
    const QStringList versionStrings{QString::number(version.majorVersion()), version.toString()};
    for (const QString &libDir : libDirs) {
        for (const QString &versionString : versionStrings) {
            const FilePath includePath = clangdFilePath.absolutePath().parentDir()
                                             .pathAppended(libDir).pathAppended("clang")
                                             .pathAppended(versionString).pathAppended("include");
            if (includePath.exists())
                return includePath;
        }
    }
    QTC_CHECK(false);
    return {};
}

FilePath ClangdSettings::clangdIncludePath() const
{
    QTC_ASSERT(useClangd(), return {});
    FilePath clangdPath = clangdFilePath();
    QTC_ASSERT(!clangdPath.isEmpty() && clangdPath.exists(), return {});
    static QHash<FilePath, FilePath> headersPathCache;
    const auto it = headersPathCache.constFind(clangdPath);
    if (it != headersPathCache.constEnd())
        return *it;
    const FilePath headersPath = getClangHeadersPath(clangdPath);
    if (!headersPath.isEmpty())
        headersPathCache.insert(clangdPath, headersPath);
    return headersPath;
}

FilePath ClangdSettings::clangdUserConfigFilePath()
{
    return FilePath::fromString(
                QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
            / "clangd/config.yaml";
}

void ClangdSettings::loadSettings()
{
    const auto settings = Core::ICore::settings();

    m_data.fromMap(Utils::storeFromSettings(clangdSettingsKey(), settings));

    settings->beginGroup(Constants::CPPEDITOR_SETTINGSGROUP);
    m_data.customDiagnosticConfigs = diagnosticConfigsFromSettings(settings);

    // Pre-8.0 compat
    static const Key oldKey("ClangDiagnosticConfig");
    const QVariant configId = settings->value(oldKey);
    if (configId.isValid()) {
        m_data.diagnosticConfigId = Id::fromSetting(configId);
        settings->setValue(oldKey, {});
    }

    settings->endGroup();
}

void ClangdSettings::saveSettings()
{
    const auto settings = Core::ICore::settings();
    Utils::storeToSettings(clangdSettingsKey(), settings, m_data.toMap());
    settings->beginGroup(Constants::CPPEDITOR_SETTINGSGROUP);
    diagnosticConfigsToSettings(settings, m_data.customDiagnosticConfigs);
    settings->endGroup();
}

#ifdef WITH_TESTS
void ClangdSettings::setClangdFilePath(const FilePath &filePath)
{
    instance().m_data.executableFilePath = filePath;
}
#endif

ClangdProjectSettings::ClangdProjectSettings(ProjectExplorer::Project *project) : m_project(project)
{
    loadSettings();
}

ClangdSettings::Data ClangdProjectSettings::settings() const
{
    const ClangdSettings::Data globalData = ClangdSettings::instance().data();
    ClangdSettings::Data data = globalData;
    if (!m_useGlobalSettings) {
        data = m_customSettings;
    // This property is global by definition.
    data.sessionsWithOneClangd = ClangdSettings::instance().data().sessionsWithOneClangd;

    // This list exists only once.
    data.customDiagnosticConfigs = ClangdSettings::instance().data().customDiagnosticConfigs;
}
    if (m_blockIndexing)
        data.indexingPriority = ClangdSettings::IndexingPriority::Off;
    return data;
}

void ClangdProjectSettings::setSettings(const ClangdSettings::Data &data)
{
    m_customSettings = data;
    saveSettings();
    ClangdSettings::setCustomDiagnosticConfigs(data.customDiagnosticConfigs);
    emit ClangdSettings::instance().changed();
}

void ClangdProjectSettings::setUseGlobalSettings(bool useGlobal)
{
    m_useGlobalSettings = useGlobal;
    saveSettings();
    emit ClangdSettings::instance().changed();
}

void ClangdProjectSettings::setDiagnosticConfigId(Utils::Id configId)
{
    m_customSettings.diagnosticConfigId = configId;
    saveSettings();
    emit ClangdSettings::instance().changed();
}

void ClangdProjectSettings::blockIndexing()
{
    if (m_blockIndexing)
        return;
    m_blockIndexing = true;
    saveSettings();
    emit ClangdSettings::instance().changed();
}

void ClangdProjectSettings::unblockIndexing()
{
    if (!m_blockIndexing)
        return;
    m_blockIndexing = false;
    saveSettings();
    // Do not emit changed here since that would restart clients with blocked indexing
}

void ClangdProjectSettings::loadSettings()
{
    if (!m_project)
        return;
    const Store data = storeFromVariant(m_project->namedSettings(clangdSettingsKey()));
    m_useGlobalSettings = data.value(clangdUseGlobalSettingsKey(), true).toBool();
    m_blockIndexing = data.value(clangdblockIndexingSettingsKey(), false).toBool();
    if (!m_useGlobalSettings)
        m_customSettings.fromMap(data);
}

void ClangdProjectSettings::saveSettings()
{
    if (!m_project)
        return;
    Store data;
    if (!m_useGlobalSettings)
        data = m_customSettings.toMap();
    data.insert(clangdUseGlobalSettingsKey(), m_useGlobalSettings);
    data.insert(clangdblockIndexingSettingsKey(), m_blockIndexing);
    m_project->setNamedSettings(clangdSettingsKey(), variantFromStore(data));
}

Store ClangdSettings::Data::toMap() const
{
    Store map;
    map.insert(useClangdKey(), useClangd);
    map.insert(clangdPathKey(),
               executableFilePath != fallbackClangdFilePath() ? executableFilePath.toString()
                                                              : QString());
    map.insert(clangdIndexingKey(), indexingPriority != IndexingPriority::Off);
    map.insert(clangdIndexingPriorityKey(), int(indexingPriority));
    map.insert(clangdHeaderSourceSwitchModeKey(), int(headerSourceSwitchMode));
    map.insert(clangdCompletionRankingModelKey(), int(completionRankingModel));
    map.insert(clangdHeaderInsertionKey(), autoIncludeHeaders);
    map.insert(clangdThreadLimitKey(), workerThreadLimit);
    map.insert(clangdDocumentThresholdKey(), documentUpdateThreshold);
    map.insert(clangdSizeThresholdEnabledKey(), sizeThresholdEnabled);
    map.insert(clangdSizeThresholdKey(), sizeThresholdInKb);
    map.insert(sessionsWithOneClangdKey(), sessionsWithOneClangd);
    map.insert(diagnosticConfigIdKey(), diagnosticConfigId.toSetting());
    map.insert(checkedHardwareKey(), true);
    map.insert(completionResultsKey(), completionResults);
    return map;
}

void ClangdSettings::Data::fromMap(const Store &map)
{
    useClangd = map.value(useClangdKey(), true).toBool();
    executableFilePath = FilePath::fromString(map.value(clangdPathKey()).toString());
    indexingPriority = IndexingPriority(
        map.value(clangdIndexingPriorityKey(), int(this->indexingPriority)).toInt());
    const auto it = map.find(clangdIndexingKey());
    if (it != map.end() && !it->toBool())
        indexingPriority = IndexingPriority::Off;
    headerSourceSwitchMode = HeaderSourceSwitchMode(map.value(clangdHeaderSourceSwitchModeKey(),
                                                              int(headerSourceSwitchMode)).toInt());
    completionRankingModel = CompletionRankingModel(map.value(clangdCompletionRankingModelKey(),
                                                              int(completionRankingModel)).toInt());
    autoIncludeHeaders = map.value(clangdHeaderInsertionKey(), false).toBool();
    workerThreadLimit = map.value(clangdThreadLimitKey(), 0).toInt();
    documentUpdateThreshold = map.value(clangdDocumentThresholdKey(), 500).toInt();
    sizeThresholdEnabled = map.value(clangdSizeThresholdEnabledKey(), false).toBool();
    sizeThresholdInKb = map.value(clangdSizeThresholdKey(), 1024).toLongLong();
    sessionsWithOneClangd = map.value(sessionsWithOneClangdKey()).toStringList();
    diagnosticConfigId = Id::fromSetting(map.value(diagnosticConfigIdKey(),
                                                   initialClangDiagnosticConfigId().toSetting()));
    haveCheckedHardwareReqirements = map.value(checkedHardwareKey(), false).toBool();
    completionResults = map.value(completionResultsKey(), defaultCompletionResults()).toInt();
}

int ClangdSettings::Data::defaultCompletionResults()
{
    // Default clangd --limit-results value is 100
    bool ok = false;
    const int userValue = qtcEnvironmentVariableIntValue("QTC_CLANGD_COMPLETION_RESULTS", &ok);
    return ok ? userValue : 100;
}

} // namespace CppEditor
