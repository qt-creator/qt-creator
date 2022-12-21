// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcodemodelsettings.h"

#include "clangdiagnosticconfigsmodel.h"
#include "cppeditorconstants.h"
#include "cpptoolsreuse.h"

#include <coreplugin/icore.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/settingsutils.h>

#include <QDateTime>
#include <QHash>
#include <QPair>
#include <QSettings>
#include <QStandardPaths>

using namespace Utils;

namespace CppEditor {

static Id initialClangDiagnosticConfigId()
{ return Constants::CPP_CLANG_DIAG_CONFIG_BUILDSYSTEM; }

static CppCodeModelSettings::PCHUsage initialPchUsage()
{ return CppCodeModelSettings::PchUse_BuildSystem; }

static QString enableLowerClazyLevelsKey()
{ return QLatin1String("enableLowerClazyLevels"); }

static QString pchUsageKey()
{ return QLatin1String(Constants::CPPEDITOR_MODEL_MANAGER_PCH_USAGE); }

static QString interpretAmbiguousHeadersAsCHeadersKey()
{ return QLatin1String(Constants::CPPEDITOR_INTERPRET_AMBIGIUOUS_HEADERS_AS_C_HEADERS); }

static QString skipIndexingBigFilesKey()
{ return QLatin1String(Constants::CPPEDITOR_SKIP_INDEXING_BIG_FILES); }

static QString ignoreFilesKey()
{ return QLatin1String(Constants::CPPEDITOR_IGNORE_FILES); }

static QString ignorePatternKey()
{ return QLatin1String(Constants::CPPEDITOR_IGNORE_PATTERN); }

static QString useBuiltinPreprocessorKey()
{ return QLatin1String(Constants::CPPEDITOR_USE_BUILTIN_PREPROCESSOR); }

static QString indexerFileSizeLimitKey()
{ return QLatin1String(Constants::CPPEDITOR_INDEXER_FILE_SIZE_LIMIT); }

static QString clangdSettingsKey() { return QLatin1String("ClangdSettings"); }
static QString useClangdKey() { return QLatin1String("UseClangdV7"); }
static QString clangdPathKey() { return QLatin1String("ClangdPath"); }
static QString clangdIndexingKey() { return QLatin1String("ClangdIndexing"); }
static QString clangdIndexingPriorityKey() { return QLatin1String("ClangdIndexingPriority"); }
static QString clangdHeaderInsertionKey() { return QLatin1String("ClangdHeaderInsertion"); }
static QString clangdThreadLimitKey() { return QLatin1String("ClangdThreadLimit"); }
static QString clangdDocumentThresholdKey() { return QLatin1String("ClangdDocumentThreshold"); }
static QString clangdSizeThresholdEnabledKey() { return QLatin1String("ClangdSizeThresholdEnabled"); }
static QString clangdSizeThresholdKey() { return QLatin1String("ClangdSizeThreshold"); }
static QString clangdUseGlobalSettingsKey() { return QLatin1String("useGlobalSettings"); }
static QString sessionsWithOneClangdKey() { return QLatin1String("SessionsWithOneClangd"); }
static QString diagnosticConfigIdKey() { return QLatin1String("diagnosticConfigId"); }
static QString checkedHardwareKey() { return QLatin1String("checkedHardware"); }
static QString completionResultsKey() { return QLatin1String("completionResults"); }

static FilePath g_defaultClangdFilePath;
static FilePath fallbackClangdFilePath()
{
    if (g_defaultClangdFilePath.exists())
        return g_defaultClangdFilePath;
    return Environment::systemEnvironment().searchInPath("clangd");
}

void CppCodeModelSettings::fromSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(Constants::CPPEDITOR_SETTINGSGROUP));

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

void CppCodeModelSettings::toSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(Constants::CPPEDITOR_SETTINGSGROUP));

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
    case IndexingPriority::Background: return tr("Background Priority");
    case IndexingPriority::Normal: return tr("Normal Priority");
    case IndexingPriority::Low: return tr("Low Priority");
    case IndexingPriority::Off: return tr("Off");
    }
    return {};
}

ClangdSettings &ClangdSettings::instance()
{
    static ClangdSettings settings;
    return settings;
}

ClangdSettings::ClangdSettings()
{
    loadSettings();
    const auto sessionMgr = ProjectExplorer::SessionManager::instance();
    connect(sessionMgr, &ProjectExplorer::SessionManager::sessionRemoved,
            this, [this](const QString &name) { m_data.sessionsWithOneClangd.removeOne(name); });
    connect(sessionMgr, &ProjectExplorer::SessionManager::sessionRenamed,
            this, [this](const QString &oldName, const QString &newName) {
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
    if (m_data.sessionsWithOneClangd.contains(ProjectExplorer::SessionManager::activeSession()))
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

static QVersionNumber getClangdVersion(const FilePath &clangdFilePath)
{
    Utils::QtcProcess clangdProc;
    clangdProc.setCommand({clangdFilePath, {"--version"}});
    clangdProc.start();
    if (!clangdProc.waitForFinished())
        return{};
    const QString output = clangdProc.allOutput();
    static const QString versionPrefix = "clangd version ";
    const int prefixOffset = output.indexOf(versionPrefix);
    if (prefixOffset == -1)
        return {};
    return QVersionNumber::fromString(output.mid(prefixOffset + versionPrefix.length()));
}

QVersionNumber ClangdSettings::clangdVersion(const FilePath &clangdFilePath)
{
    static QHash<Utils::FilePath, QPair<QDateTime, QVersionNumber>> versionCache;
    const QDateTime timeStamp = clangdFilePath.lastModified();
    const auto it = versionCache.find(clangdFilePath);
    if (it == versionCache.end()) {
        const QVersionNumber version = getClangdVersion(clangdFilePath);
        versionCache.insert(clangdFilePath, {timeStamp, version});
        return version;
    }
    if (it->first != timeStamp) {
        it->first = timeStamp;
        it->second = getClangdVersion(clangdFilePath);
    }
    return it->second;
}

static FilePath getClangHeadersPathFromClang(const FilePath &clangdFilePath)
{
    const FilePath clangFilePath = clangdFilePath.absolutePath().pathAppended("clang")
            .withExecutableSuffix();
    if (!clangFilePath.exists())
        return {};
    QtcProcess clang;
    clang.setCommand({clangFilePath, {"-print-resource-dir"}});
    clang.start();
    if (!clang.waitForFinished())
        return {};
    const FilePath resourceDir = FilePath::fromUserInput(QString::fromLocal8Bit(
            clang.readAllStandardOutput().trimmed()));
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

    const QVersionNumber version = ClangdSettings::clangdVersion(clangdFilePath);
    QTC_ASSERT(!version.isNull(), return {});
    static const QStringList libDirs{"lib", "lib64"};
    for (const QString &libDir : libDirs) {
        const FilePath includePath = clangdFilePath.absolutePath().parentDir().pathAppended(libDir)
                .pathAppended("clang").pathAppended(version.toString()).pathAppended("include");
        if (includePath.exists())
            return includePath;
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
    Utils::fromSettings(clangdSettingsKey(), {}, settings, &m_data);

    settings->beginGroup(QLatin1String(Constants::CPPEDITOR_SETTINGSGROUP));
    m_data.customDiagnosticConfigs = diagnosticConfigsFromSettings(settings);

    // Pre-8.0 compat
    static const QString oldKey("ClangDiagnosticConfig");
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
    Utils::toSettings(clangdSettingsKey(), {}, settings, &m_data);
    settings->beginGroup(QLatin1String(Constants::CPPEDITOR_SETTINGSGROUP));
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
    if (m_useGlobalSettings)
        return ClangdSettings::instance().data();
    ClangdSettings::Data data = m_customSettings;

    // This property is global by definition.
    data.sessionsWithOneClangd = ClangdSettings::instance().data().sessionsWithOneClangd;

    // This list exists only once.
    data.customDiagnosticConfigs = ClangdSettings::instance().data().customDiagnosticConfigs;

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
}

void ClangdProjectSettings::loadSettings()
{
    if (!m_project)
        return;
    const QVariantMap data = m_project->namedSettings(clangdSettingsKey()).toMap();
    m_useGlobalSettings = data.value(clangdUseGlobalSettingsKey(), true).toBool();
    if (!m_useGlobalSettings)
        m_customSettings.fromMap(data);
}

void ClangdProjectSettings::saveSettings()
{
    if (!m_project)
        return;
    QVariantMap data;
    if (!m_useGlobalSettings)
        data = m_customSettings.toMap();
    data.insert(clangdUseGlobalSettingsKey(), m_useGlobalSettings);
    m_project->setNamedSettings(clangdSettingsKey(), data);
}

QVariantMap ClangdSettings::Data::toMap() const
{
    QVariantMap map;
    map.insert(useClangdKey(), useClangd);
    map.insert(clangdPathKey(),
               executableFilePath != fallbackClangdFilePath() ? executableFilePath.toString()
                                                              : QString());
    map.insert(clangdIndexingKey(), indexingPriority != IndexingPriority::Off);
    map.insert(clangdIndexingPriorityKey(), int(indexingPriority));
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

void ClangdSettings::Data::fromMap(const QVariantMap &map)
{
    useClangd = map.value(useClangdKey(), true).toBool();
    executableFilePath = FilePath::fromString(map.value(clangdPathKey()).toString());
    indexingPriority = IndexingPriority(
        map.value(clangdIndexingPriorityKey(), int(this->indexingPriority)).toInt());
    const auto it = map.find(clangdIndexingKey());
    if (it != map.end() && !it->toBool())
        indexingPriority = IndexingPriority::Off;
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
